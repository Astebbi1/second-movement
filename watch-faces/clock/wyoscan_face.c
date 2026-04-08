/*
 * MIT License
 *
 * Copyright (c) 2023 <#author_name#>
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
#include "wyoscan_face.h"
#include "watch.h"

/*
Slowly render the current time from left to right,
scanning across its liquid crystal face, completing 1 cycle every 2 seconds.

Created to mimic the wyoscan watch that was produced by Halmos and designed by Dexter Sinister.
https://www.o-r-g.com/apps/wyoscan

The seven segments of each digit are lit one by one across the display.
8 frames per digit * 6 digits + 16 trailing frames = 64 frames total.
At 32 frames per second this is a 2-second cycle time (0.5 Hz).

The top row shows a static date (DOW + day of month) refreshed each cycle.
Indicators (PM, BELL, SIGNAL) are also updated each cycle.
Leading zero on the hour tens digit is suppressed.

Segment naming:
  A
F   B
  G
E   C
  D
'X' frames are skipped (pen lift / corner pause in the drawing metaphor).
*/

static char *segment_map[] = {
    "AXFBDEXC",  // 0
    "BXXXCXXX",  // 1
    "ABGEXXXD",  // 2
    "ABGXXXCD",  // 3
    "FXGBXXXC",  // 4
    "AXFXGXCD",  // 5
    "AXFEDCXG",  // 6
    "AXXBXXCX",  // 7
    "AFGCDEXB",  // 8
    "AFGBXXCD"   // 9
};

/*
 * Pixel coordinates {com, seg} for each segment A-G of each time-digit position.
 * Two tables: one for the custom LCD, one for the classic LCD.
 * Segments are ordered A, B, C, D, E, F, G (standard seven-segment naming).
 */
static const int32_t clock_mapping_custom[6][7][2] = {
    // hour 1 (LCD pos 4)
    {{3,16}, {2,16}, {1,16}, {0,16}, {1,22}, {3,22}, {2,22}},
    // hour 2 (LCD pos 5)
    {{3,14}, {2,14}, {1,14}, {0,15}, {1,15}, {3,15}, {2,15}},
    // minute 1 (LCD pos 6)
    {{3,1},  {2,2},  {0,2},  {0,1},  {1,1},  {2,1},  {1,2}},
    // minute 2 (LCD pos 7)
    {{3,3},  {2,4},  {0,4},  {0,3},  {1,3},  {2,3},  {1,4}},
    // second 1 (LCD pos 8)
    {{3,10}, {3,8},  {0,5},  {1,5},  {3,4},  {3,2},  {2,5}},
    // second 2 (LCD pos 9)
    {{3,6},  {3,7},  {2,7},  {0,7},  {0,6},  {2,6},  {1,6}},
};

static const int32_t clock_mapping_classic[6][7][2] = {
    // hour 1 (LCD pos 4)
    {{1,18}, {2,19}, {0,19}, {1,18}, {0,18}, {2,18}, {1,19}},
    // hour 2 (LCD pos 5)
    {{2,20}, {2,21}, {1,21}, {0,21}, {0,20}, {1,17}, {1,20}},
    // minute 1 (LCD pos 6)
    {{0,22}, {2,23}, {0,23}, {0,22}, {1,22}, {2,22}, {1,23}},
    // minute 2 (LCD pos 7)
    {{2,1},  {2,10}, {0,1},  {0,0},  {1,0},  {2,0},  {1,1}},
    // second 1 (LCD pos 8)
    {{2,2},  {2,3},  {0,4},  {0,3},  {0,2},  {1,2},  {1,3}},
    // second 2 (LCD pos 9)
    {{2,4},  {2,5},  {1,6},  {0,6},  {0,5},  {1,4},  {1,5}},
};

// 2-char DOW abbreviations for classic LCD (0=Mon ... 6=Sun)
static const char *_wyoscan_dow2[] = {"Mo","tu","WE","tH","Fr","SA","Su"};
// 3-char DOW abbreviations for custom LCD
static const char *_wyoscan_dow3[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};

// Returns 0=Monday ... 6=Sunday
static uint8_t _wyoscan_dow(watch_date_time_t dt) {
    uint8_t y = dt.unit.year + 20;
    uint8_t m = dt.unit.month;
    if (m <= 2) { m += 12; y--; }
    return (dt.unit.day + 13 * (m + 1) / 5 + y + y / 4 + 525 - 2) % 7;
}

// Update the static top-row date display and indicators for the new cycle.
static void _wyoscan_update_static(watch_date_time_t dt, bool is_pm) {
    uint8_t dow = _wyoscan_dow(dt);
    char buf[3];

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // Custom LCD: 3-char DOW in positions 0-1-10, day in positions 2-3
        watch_display_text(WATCH_POSITION_TOP_LEFT,  _wyoscan_dow3[dow]);
        snprintf(buf, sizeof(buf), "%2u", dt.unit.day);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        // Classic LCD: 2-char DOW in positions 0-1, day in positions 2-3
        watch_display_text(WATCH_POSITION_TOP_LEFT, _wyoscan_dow2[dow]);
        snprintf(buf, sizeof(buf), "%2u", dt.unit.day);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }

    // Indicators
    if (is_pm) watch_set_indicator(WATCH_INDICATOR_PM);
    else        watch_clear_indicator(WATCH_INDICATOR_PM);

    if (movement_alarm_enabled()) watch_set_indicator(WATCH_INDICATOR_BELL);
    else                          watch_clear_indicator(WATCH_INDICATOR_BELL);

    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
}

void wyoscan_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wyoscan_state_t));
        memset(*context_ptr, 0, sizeof(wyoscan_state_t));
    }
}

void wyoscan_face_activate(void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;
    movement_request_tick_frequency(32);
    state->total_frames = 64;
}

bool wyoscan_face_loop(movement_event_t event, void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;
    const int32_t (*mapping)[7][2] = (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM)
                                     ? clock_mapping_custom
                                     : clock_mapping_classic;

    watch_date_time_t date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;
        case EVENT_TICK:
            if (!state->animate) {
                date_time = watch_rtc_get_date_time();
                state->start     = 0;
                state->end       = 0;
                state->animation = 0;
                state->animate   = true;

                // Resolve hour with 12h support
                uint8_t hour = date_time.unit.hour;
                bool is_pm = false;
                if (!movement_clock_mode_24h()) {
                    is_pm = (hour >= 12);
                    hour  = hour % 12;
                    if (hour == 0) hour = 12;
                }

                state->time_digits[0] = hour / 10;
                state->time_digits[1] = hour % 10;
                state->time_digits[2] = date_time.unit.minute / 10;
                state->time_digits[3] = date_time.unit.minute % 10;
                state->time_digits[4] = date_time.unit.second / 10;
                state->time_digits[5] = date_time.unit.second % 10;

                // Refresh date row and indicators once per cycle
                _wyoscan_update_static(date_time, is_pm);
            }
            if (state->animate) {
                // Erase the oldest comet pixel when the ring buffer is full.
                if ((state->end + 1) % MAX_ILLUMINATED_SEGMENTS == state->start) {
                    if (state->illuminated_segments[state->start][0] != 99 &&
                        state->illuminated_segments[state->start][1] != 99) {
                        watch_clear_pixel(state->illuminated_segments[state->start][0],
                                          state->illuminated_segments[state->start][1]);
                    }
                    state->start = (state->start + 1) % MAX_ILLUMINATED_SEGMENTS;
                }

                if (state->animation < state->total_frames - MAX_ILLUMINATED_SEGMENTS) {
                    // Blink the colon at ~1 Hz (every 32 frames at 32 fps).
                    if (state->animation % 32 == 0) {
                        if (state->colon) watch_set_colon();
                        else              watch_clear_colon();
                        state->colon = !state->colon;
                    }

                    state->position = (state->animation / 8) % 6;
                    state->segment  =  state->animation % strlen(segment_map[state->time_digits[state->position]]);
                    state->segments = segment_map[state->time_digits[state->position]];

                    // Suppress leading zero on the hour tens digit
                    if (state->position == 0 && state->time_digits[0] == 0) {
                        state->illuminated_segments[state->end][0] = 99;
                        state->illuminated_segments[state->end][1] = 99;
                        state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                        state->animation++;
                        break;
                    }

                    if (state->segments[state->segment] == 'X') {
                        // Skip frame — record a sentinel so the ring buffer advances.
                        state->illuminated_segments[state->end][0] = 99;
                        state->illuminated_segments[state->end][1] = 99;
                        state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                        state->animation++;
                        break;
                    }

                    state->x = mapping[state->position][state->segments[state->segment] - 'A'][0];
                    state->y = mapping[state->position][state->segments[state->segment] - 'A'][1];

                    watch_set_pixel(state->x, state->y);
                    state->illuminated_segments[state->end][0] = state->x;
                    state->illuminated_segments[state->end][1] = state->y;
                    state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;

                } else if (state->animation >= state->total_frames - MAX_ILLUMINATED_SEGMENTS &&
                           state->animation  < state->total_frames) {
                    // Trailing clearance: advance tail with no new pixels.
                    state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                } else {
                    state->animate = false;
                }
                state->animation++;
            }
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            break;
        case EVENT_ALARM_LONG_PRESS:
            break;
        case EVENT_BACKGROUND_TASK:
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void wyoscan_face_resign(void *context) {
    (void) context;
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_BELL);
    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}
