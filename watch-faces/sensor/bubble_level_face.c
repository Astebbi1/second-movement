/*
 * MIT License
 *
 * Copyright (c) 2026 Stebbs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "bubble_level_face.h"
#include "watch.h"
#include "lis2dw.h"

// Accelerometer polling rate (must match movement_set_accelerometer_background_rate call)
#define TICK_FREQ          8

// Raw accelerometer units considered "level" (±200 ≈ ±11° at 1g=1800 counts).
// Raise this to make the level zone wider; lower it for stricter level detection.
#define LEVEL_THRESHOLD    200

// Raw units at full-scale tilt mapped to display edge (positions 0 or 5).
#define TILT_SCALE         1800

// Short single pip played once per second while level.
static const int8_t _level_beep[] = {
    BUZZER_NOTE_B5, 2,  // ~30 ms
    0
};

// Read and average available FIFO samples; update smoothed readings.
static void _read_accel(bubble_level_state_t *state) {
    lis2dw_fifo_t fifo;
    lis2dw_read_fifo(&fifo, 10);
    if (fifo.count == 0) return;

    float x = 0, y = 0;
    for (uint8_t i = 0; i < fifo.count; i++) {
        x += fifo.readings[i].x;
        y += fifo.readings[i].y;
    }
    int16_t new_x = (int16_t)(x / fifo.count);
    int16_t new_y = (int16_t)(y / fifo.count);
    lis2dw_clear_fifo();

    // Exponential moving average: smoothed = (3*old + new) / 4
    state->smoothed_x = (int16_t)((state->smoothed_x * 3 + new_x) / 4);
    state->smoothed_y = (int16_t)((state->smoothed_y * 3 + new_y) / 4);
}

static void _update_display(bubble_level_state_t *state) {
    // Select active axis
    int16_t reading = state->use_y_axis ? state->smoothed_y : state->smoothed_x;

    // Clamp to ±TILT_SCALE then map to position 0–5.
    // Formula: pos = (reading + TILT_SCALE) * 6 / (2 * TILT_SCALE)
    // At reading=0 → pos=3 (center); at ±TILT_SCALE → pos=0/6 (clamped to 5).
    int32_t clamped = reading;
    if (clamped < -TILT_SCALE) clamped = -TILT_SCALE;
    if (clamped >  TILT_SCALE) clamped =  TILT_SCALE;
    uint8_t pos = (uint8_t)((clamped + TILT_SCALE) * 6 / (2 * TILT_SCALE));
    if (pos > 5) pos = 5;

    bool is_level = (reading > -LEVEL_THRESHOLD && reading < LEVEL_THRESHOLD);

    // Build 6-char bottom row: dashes for rail, bubble char at pos.
    char rail[7];
    memset(rail, '-', 6);
    rail[6] = '\0';

    if (is_level) {
        // Flash bubble between '0' (ring, no middle bar) and '8' (all segments)
        rail[pos] = (state->tick_count % 4 < 2) ? '8' : '0';
    } else {
        rail[pos] = '0';
    }

    watch_display_text(WATCH_POSITION_BOTTOM, rail);

    // Indicators and feedback when level
    if (is_level) {
        // Keep LED green
        if (!state->led_enabled) {
            watch_enable_leds();
            state->led_enabled = true;
        }
        watch_set_led_color_rgb(0, 255, 0);

        // Flash SIGNAL indicator in sync with bubble
        if (state->tick_count % 4 < 2) {
            watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        } else {
            watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
        }

        // Beep once per second (every TICK_FREQ ticks)
        if (state->tick_count % TICK_FREQ == 0) {
            watch_buzzer_play_sequence(_level_beep, NULL);
        }
    } else {
        // Turn off LED
        if (state->led_enabled) {
            watch_set_led_off();
            watch_disable_leds();
            state->led_enabled = false;
        }
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
    }

    state->tick_count++;
}

void bubble_level_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(bubble_level_state_t));
        memset(*context_ptr, 0, sizeof(bubble_level_state_t));
    }
}

void bubble_level_face_activate(void *context) {
    bubble_level_state_t *state = (bubble_level_state_t *)context;
    state->tick_count = 0;
    state->led_enabled = false;

    // Start accelerometer at 12.5 Hz background rate, drain any stale FIFO data.
    movement_set_accelerometer_background_rate(LIS2DW_DATA_RATE_12_5_HZ);
    lis2dw_enable_fifo();
    lis2dw_clear_fifo();

    movement_request_tick_frequency(TICK_FREQ);
}

bool bubble_level_face_loop(movement_event_t event, void *context) {
    bubble_level_state_t *state = (bubble_level_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_clear_colon();
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LEVEL", "LvL");
            watch_display_text(WATCH_POSITION_BOTTOM, "------");
            break;

        case EVENT_TICK:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LEVEL", "LvL");
            _read_accel(state);
            _update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Toggle between X and Y axis for different holding orientations
            state->use_y_axis = !state->use_y_axis;
            // Brief visual confirmation: show which axis is now active
            watch_display_text(WATCH_POSITION_BOTTOM, state->use_y_axis ? "  Y   " : "  X   ");
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Allow LED button as normal (LED brief illuminate via movement)
            movement_illuminate_led();
            break;

        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            // Low energy: show static label, no accelerometer drain
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LEVEL", "LvL");
            watch_display_text(WATCH_POSITION_BOTTOM, "--LvL-");
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void bubble_level_face_resign(void *context) {
    bubble_level_state_t *state = (bubble_level_state_t *)context;

    // Clean up accelerometer FIFO
    lis2dw_clear_fifo();
    lis2dw_disable_fifo();

    // Ensure LED is off when leaving the face
    if (state->led_enabled) {
        watch_set_led_off();
        watch_disable_leds();
        state->led_enabled = false;
    }

    // Clear any indicators we set
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);

    movement_request_tick_frequency(1);
}
