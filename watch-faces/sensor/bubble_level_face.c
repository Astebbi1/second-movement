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

/* Map a raw reading to a 0-5 position for the 1D 6-char rail. */
static uint8_t _to_6pos(int16_t reading) {
    int32_t c = reading;
    if (c < -TILT_SCALE) c = -TILT_SCALE;
    if (c >  TILT_SCALE) c =  TILT_SCALE;
    uint8_t pos = (uint8_t)((c + TILT_SCALE) * 6 / (2 * TILT_SCALE));
    if (pos > 5) pos = 5;
    return pos;
}

/* Map a raw reading to a 0-2 position for a 3-char half-rail (2D mode). */
static uint8_t _to_3pos(int16_t reading) {
    if (reading < -LEVEL_THRESHOLD) return 0;
    if (reading >  LEVEL_THRESHOLD) return 2;
    return 1;
}

static void _update_display(bubble_level_state_t *state) {
    bool flash = (state->tick_count % 4 < 2);
    char bot[7] = "------";

    bool x_level = (state->smoothed_x > -LEVEL_THRESHOLD && state->smoothed_x < LEVEL_THRESHOLD);
    bool y_level = (state->smoothed_y > -LEVEL_THRESHOLD && state->smoothed_y < LEVEL_THRESHOLD);

    if (state->mode == BUBBLE_MODE_2D) {
        /* Left half (chars 0-2): Y axis (front/back tilt).
         * Right half (chars 3-5): X axis (left/right tilt).
         * Bubble at center of each half = both axes level. */
        uint8_t yp = _to_3pos(state->smoothed_y);
        uint8_t xp = _to_3pos(state->smoothed_x);
        bot[yp]     = (y_level && flash) ? '8' : '0';
        bot[3 + xp] = (x_level && flash) ? '8' : '0';
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "2dLvL", "2d");
    } else {
        /* 1D mode: single bubble across 6-char rail. */
        int16_t reading = (state->mode == BUBBLE_MODE_Y) ? state->smoothed_y : state->smoothed_x;
        bool is_level   = (state->mode == BUBBLE_MODE_Y) ? y_level : x_level;
        uint8_t pos = _to_6pos(reading);
        bot[pos] = (is_level && flash) ? '8' : '0';
        const char *top_1d = (state->mode == BUBBLE_MODE_Y) ? "LvL Y" : "LvL X";
        const char *top_fb = (state->mode == BUBBLE_MODE_Y) ? "LvY"   : "LvX";
        watch_display_text_with_fallback(WATCH_POSITION_TOP, top_1d, top_fb);
        x_level = y_level = is_level;  /* reuse combined check below */
    }

    watch_display_text(WATCH_POSITION_BOTTOM, bot);

    bool both_level = x_level && y_level;

    if (both_level) {
        if (!state->led_enabled) {
            watch_enable_leds();
            state->led_enabled = true;
        }
        watch_set_led_color_rgb(0, 255, 0);
        if (flash) {
            watch_set_indicator(WATCH_INDICATOR_SIGNAL);
        } else {
            watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
        }
        if (state->tick_count % TICK_FREQ == 0) {
            watch_buzzer_play_sequence((int8_t *)_level_beep, NULL);
        }
    } else {
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
        ((bubble_level_state_t *)*context_ptr)->mode = BUBBLE_MODE_Y;
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
            watch_display_text(WATCH_POSITION_BOTTOM, "------");
            break;

        case EVENT_TICK:
            _read_accel(state);
            _update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            /* Cycle X → Y → 2D → X */
            state->mode = (bubble_mode_t)((state->mode + 1) % 3);
            switch (state->mode) {
                case BUBBLE_MODE_X:  watch_display_text(WATCH_POSITION_BOTTOM, "  X   "); break;
                case BUBBLE_MODE_Y:  watch_display_text(WATCH_POSITION_BOTTOM, "  Y   "); break;
                case BUBBLE_MODE_2D: watch_display_text(WATCH_POSITION_BOTTOM, " Y  X "); break;
            }
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
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LvL  ", "LvL");
            watch_display_text(WATCH_POSITION_BOTTOM, "--LvL-");
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void bubble_level_face_resign(void *context) {
    bubble_level_state_t *state = (bubble_level_state_t *)context;

    // Restore tick frequency so faces that don't set it explicitly get 1 Hz
    movement_request_tick_frequency(1);

    // Clean up accelerometer FIFO.
    // If step counting is active, we must not disable the FIFO — just clear
    // the stale samples so the pedometer resumes cleanly from here.
    lis2dw_clear_fifo();
    if (movement_step_count_is_enabled()) {
        lis2dw_enable_fifo(); // restore FIFO for step counting
    } else {
        lis2dw_disable_fifo();
    }

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
