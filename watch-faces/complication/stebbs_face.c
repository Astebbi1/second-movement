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
// Flop advances every 2 ticks → 4 steps/sec, 8-frame cycle = 2 sec/swing
#define FLOP_DIVISOR       2

// 5 leading / 3 trailing spaces — extra lead-in so text doesn't appear abruptly
// Full text: "     StEbbS WAtCH  MON Feb 23   " = 32 chars
#define SCROLL_CONTENT_LEN 32

// Wave animation: each of the 4 top positions is offset by 2 frames in the
// cycle, so the rotation flows left-to-right across the top rather than all
// positions moving in lockstep.
// l = left vertical (E+F), / = forward slash (B+E), - = middle bar (G),
// \ = backslash (C+F), 1 = right vertical (B+C)
#define FLOP_CYCLE_LEN   8
#define FLOP_WAVE_OFFSET 2   // frame offset between adjacent top positions
static const char flop_seq[FLOP_CYCLE_LEN] = {'l', '/', '-', '\\', '1', '\\', '-', '/'};

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

static void _update_scroll_text(stebbs_state_t *state) {
    watch_date_time_t date_time = movement_get_local_date_time();
    uint8_t dow = _stebbs_dow(date_time);
    // 5 leading + "StEbbS WAtCH  " (14) + dow (3) + " " + month (3) + " " + day (2) + 3 trailing = 32 chars
    sprintf(state->scroll_text, "     StEbbS WAtCH  %s %s %2d   ",
            _stebbs_dow_names[dow], _stebbs_month_names[date_time.unit.month], date_time.unit.day);
}

static void _stebbs_display(stebbs_state_t *state) {
    char display_buf[11];

    // Top 4 positions: wave — each offset by FLOP_WAVE_OFFSET frames
    for (int i = 0; i < 4; i++) {
        display_buf[i] = flop_seq[(state->flop_frame + i * FLOP_WAVE_OFFSET) % FLOP_CYCLE_LEN];
    }

    // Bottom 6 positions: scrolling text
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
    state->flop_frame = 0;
    state->scroll_pos = 0;
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

                // Advance flop every FLOP_DIVISOR ticks
                if (state->tick_count % FLOP_DIVISOR == 0) {
                    state->flop_frame = (state->flop_frame + 1) % FLOP_CYCLE_LEN;
                }

                // Advance scroll every SCROLL_DIVISOR ticks
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
