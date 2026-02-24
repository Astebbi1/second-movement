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
#include <stdio.h>
#include "stebbs_face.h"
#include "watch.h"

// 8 ticks per second; scroll advances every 2 ticks → 4 chars/sec
#define TICK_FREQUENCY     8
#define SCROLL_DIVISOR     2

// 6 leading/trailing spaces so text scrolls cleanly in and out
// Full text: "      StEbbS WAtCH  MON Feb 23      " = 6+14+3+1+3+1+2+6 = 36 chars
#define SCROLL_CONTENT_LEN 36

// Day-of-week abbreviations (0=Monday...6=Sunday)
static const char *_stebbs_dow_names[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

// Month abbreviations (1-indexed; index 0 unused)
static const char *_stebbs_month_names[] = {
    "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// Returns 0=Monday ... 6=Sunday
static uint8_t _stebbs_dow(watch_date_time_t dt) {
    uint8_t y = dt.unit.year + 20;
    uint8_t m = dt.unit.month;
    if (m <= 2) { m += 12; y--; }
    return (dt.unit.day + 13 * (m + 1) / 5 + y + y / 4 + 525 - 2) % 7;
}

// Star twinkle animation: each of the 4 top positions independently cycles
// through this sequence, staggered by star_phase_offset[].
// Uses '-' (dim), '0' (ring with top segment A), '8' (all segments bright).
static const char star_chars[] = {
    ' ', ' ', ' ', ' ',   // 4 dark frames (waiting between twinkles)
    '-',                   // 1 dim glimmer
    '0',                   // 1 ring glow (segment A = top bar lit)
    '8',                   // 1 full burst (all segments)
    '0',                   // 1 ring glow (dimming)
    '-',                   // 1 last glimmer
    ' ', ' ', ' ',         // 3 dark frames
};
#define STAR_CYCLE_LEN 13

// Stagger each of the 4 positions so they don't twinkle simultaneously
static const uint8_t star_phase_offset[4] = {0, 5, 9, 3};

static void _update_scroll_text(stebbs_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    uint8_t dow = _stebbs_dow(date_time);
    // 6 leading + "StEbbS WAtCH  " (14) + dow (3) + " " + month (3) + " " + day (2) + 6 trailing = 36 chars
    sprintf(state->scroll_text, "      StEbbS WAtCH  %s %s %2d      ",
            _stebbs_dow_names[dow], _stebbs_month_names[date_time.unit.month], date_time.unit.day);
}

static void _stebbs_display(stebbs_state_t *state) {
    char display_buf[11];

    // Top 4 chars: independently twinkling stars
    for (int i = 0; i < 4; i++) {
        uint8_t phase = (state->fireworks_frame + star_phase_offset[i]) % STAR_CYCLE_LEN;
        display_buf[i] = star_chars[phase];
    }

    // Bottom 6 chars: scrolling text
    for (int i = 0; i < 6; i++) {
        int idx = (int)state->scroll_pos + i;
        display_buf[4 + i] = (idx < SCROLL_CONTENT_LEN) ? state->scroll_text[idx] : ' ';
    }
    display_buf[10] = '\0';

    watch_clear_colon();
    watch_display_text(WATCH_POSITION_FULL, display_buf);
}

void stebbs_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(stebbs_state_t));
        stebbs_state_t *state = (stebbs_state_t *)*context_ptr;
        memset(state, 0, sizeof(stebbs_state_t));
        _update_scroll_text(state);
    }
}

void stebbs_face_activate(void *context) {
    stebbs_state_t *state = (stebbs_state_t *)context;
    state->tick_count = 0;
    _update_scroll_text(state);  // refresh time every time face is entered
    movement_request_tick_frequency(TICK_FREQUENCY);
}

bool stebbs_face_loop(movement_event_t event, void *context) {
    stebbs_state_t *state = (stebbs_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _stebbs_display(state);
            break;

        case EVENT_TICK:
            if (!state->paused) {
                state->tick_count++;

                // Advance star twinkle every tick (wraps cleanly at STAR_CYCLE_LEN)
                state->fireworks_frame = (state->fireworks_frame + 1) % STAR_CYCLE_LEN;

                // Advance scroll every SCROLL_DIVISOR ticks (4 chars/sec)
                if (state->tick_count % SCROLL_DIVISOR == 0) {
                    state->scroll_pos++;
                    if (state->scroll_pos >= SCROLL_CONTENT_LEN) {
                        state->scroll_pos = 0;
                        _update_scroll_text(state);
                    }
                }
            }
            _stebbs_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Toggle pause
            state->paused = !state->paused;
            break;

        case EVENT_LIGHT_BUTTON_UP:
            movement_illuminate_led();
            break;

        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        case EVENT_LOW_ENERGY_UPDATE: {
            // In low energy mode, show static abbreviated display
            char time_buf[7];
            watch_date_time_t date_time = movement_get_local_date_time();
            uint8_t hour = date_time.unit.hour;
            if (!movement_clock_mode_24h()) {
                if (hour == 0) hour = 12;
                else if (hour > 12) hour -= 12;
            }
            sprintf(time_buf, "%2d-%02d ", hour, date_time.unit.minute);
            watch_display_text(WATCH_POSITION_BOTTOM, time_buf);
            watch_display_text(WATCH_POSITION_TOP, "Stb ");
            break;
        }

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void stebbs_face_resign(void *context) {
    (void) context;

    movement_request_tick_frequency(1);
    watch_clear_colon();
}
