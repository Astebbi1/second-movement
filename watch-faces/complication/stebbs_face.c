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
// Pulse advances every 2 ticks → 4 steps/sec, 8-step cycle = 2 sec/pulse
#define PULSE_DIVISOR      2

// Build-date overlay: 3 seconds of build date then 3 seconds of live time
#define BUILD_DATE_PHASE1  (3 * TICK_FREQUENCY)   // ticks before switching to time
#define BUILD_DATE_TOTAL   (6 * TICK_FREQUENCY)   // ticks before dismissing

// 5 leading / 3 trailing spaces — extra lead-in so text doesn't appear abruptly
// Full text: "     StEbbS WAtCH  MON Feb 23   " = 32 chars
#define SCROLL_CONTENT_LEN 32

// Heartbeat pulse animation for the top 4 positions.
// Segments grow outward from the centre, peak at full brightness ('8' = all
// segments), then shrink back and rest. Each of the 8 steps is held for
// PULSE_DIVISOR ticks, giving a 2-second cycle at 8 Hz.
//
// Step layout (4 chars = top positions left to right):
//   0: "    "  rest / blank
//   1: " -- "  centre pair emerge (middle bars)
//   2: "----"  all four bars
//   3: "8888"  full brightness — peak
//   4: "8888"  peak hold
//   5: "----"  decay to bars
//   6: " -- "  shrink to centre
//   7: "    "  rest / blank
#define PULSE_CYCLE_LEN 8
static const char pulse_frames[PULSE_CYCLE_LEN][4] = {
    {' ', ' ', ' ', ' '},
    {' ', '-', '-', ' '},
    {'-', '-', '-', '-'},
    {'8', '8', '8', '8'},
    {'8', '8', '8', '8'},
    {'-', '-', '-', '-'},
    {' ', '-', '-', ' '},
    {' ', ' ', ' ', ' '},
};

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

// Parse __DATE__ ("Mmm  D YYYY" or "Mmm DD YYYY") into "MM-DD-YY" (8 chars + NUL).
// ':' is blank on the classic LCD, so '-' is used as separator instead.
static void _stebbs_build_date_str(char out[9]) {
    static const char _mon_table[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *d = __DATE__;
    uint8_t month = 0;
    for (uint8_t i = 0; i < 12; i++) {
        if (d[0] == _mon_table[i*3] && d[1] == _mon_table[i*3+1] && d[2] == _mon_table[i*3+2]) {
            month = i + 1;
            break;
        }
    }
    // d[4..5] = space-padded day ("  7" → d[4]=' ', d[5]='7'; " 17" → d[4]='1', d[5]='7')
    uint8_t day  = ((d[4] == ' ') ? 0 : (uint8_t)(d[4] - '0')) * 10 + (uint8_t)(d[5] - '0');
    // d[9..10] = last two digits of 4-digit year
    uint8_t year = (uint8_t)(d[9] - '0') * 10 + (uint8_t)(d[10] - '0');
    snprintf(out, 9, "%02d-%02d-%02d", month, day, year);
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

    if (state->show_build_date) {
        // Heartbeat continues in positions 0-3; positions 4-9 show the overlay.
        for (int i = 0; i < 4; i++) {
            display_buf[i] = pulse_frames[state->pulse_step][i];
        }

        if (state->build_date_ticks < BUILD_DATE_PHASE1) {
            // Phase 1: build date "MMDDYY" (compile-time constant, no separators)
            char bd[9];
            _stebbs_build_date_str(bd);
            // bd is "DD-MM-YY"; extract only the 6 digit chars
            display_buf[4] = bd[0]; display_buf[5] = bd[1];
            display_buf[6] = bd[3]; display_buf[7] = bd[4];
            display_buf[8] = bd[6]; display_buf[9] = bd[7];
        } else {
            // Phase 2: build time in 12h format "HMMSS" from __TIME__ ("HH:MM:SS")
            const char *t = __TIME__;
            uint8_t h = (uint8_t)((t[0] - '0') * 10 + (t[1] - '0'));
            if (h == 0) h = 12;
            else if (h > 12) h -= 12;
            char bt[7];
            snprintf(bt, sizeof(bt), "%2u%c%c%c%c", h, t[3], t[4], t[6], t[7]);
            display_buf[4] = bt[0]; display_buf[5] = bt[1];
            display_buf[6] = bt[2]; display_buf[7] = bt[3];
            display_buf[8] = bt[4]; display_buf[9] = bt[5];
        }

        display_buf[10] = '\0';
        watch_clear_colon();
        watch_display_text(WATCH_POSITION_FULL, display_buf);
        return;
    }

    // Normal display: top 4 positions heartbeat, bottom 6 scrolling text
    for (int i = 0; i < 4; i++) {
        display_buf[i] = pulse_frames[state->pulse_step][i];
    }

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
    state->pulse_step = 0;
    state->scroll_pos = 0;
    state->show_build_date = false;
    state->build_date_ticks = 0;
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
            // Build-date overlay timeout counts regardless of pause state
            if (state->show_build_date) {
                state->build_date_ticks++;
                if (state->build_date_ticks >= BUILD_DATE_TOTAL) {
                    state->show_build_date = false;
                }
            }

            if (!state->paused) {
                state->tick_count++;

                // Advance pulse every PULSE_DIVISOR ticks
                if (state->tick_count % PULSE_DIVISOR == 0) {
                    state->pulse_step = (state->pulse_step + 1) % PULSE_CYCLE_LEN;
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

        case EVENT_ALARM_LONG_PRESS:
            // Show build date (phase 1) then live time (phase 2) overlay
            state->show_build_date = true;
            state->build_date_ticks = 0;
            _stebbs_display(state);
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
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "Stb  ", "Sb");
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
