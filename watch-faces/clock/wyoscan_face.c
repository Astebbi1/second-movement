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
#include "watch_common_display.h"

/*
Slowly render the current time from left to right,
scanning across its liquid crystal face, completing 1 cycle every 2 seconds.

Created to mimic the wyoscan watch that was produced by Halmos and designed by Dexter Sinister
It looks like this https://www.o-r-g.com/apps/wyoscan

You'll notice that reading this watch requires more attention than usual,
as the seven segments of each digit are lit one by one across its display.
This speed may be adjusted until it reaches the limits of your perception.
You and your watch are now in tune.

This is a relatively generic way of animating a time display.
If you want to modify the animation, you can change the segment_map
the A-F are corresponding to the segments on the watch face
  A
F   B
  G
E   C
  D
the X's are the frames that will be skipped in the animation
This particular segment_map allocates 8 frames to display each number
this is to achieve the 2 second cycle time.
8 frames per number * 6 numbers + the trailing 16 frames = 64 frames
at 32 frames per second, this is a 2 second cycle time.

I tried to make the animation of each number display similar to if you were
to draw the number on the watch face with a pen, pausing with 'X'
when your pen might turn a corner or when you might cross over
a line you've already drawn. It is vaguely top to bottom and counter,
clockwise when possible.

The top row (day-of-week + day-of-month) is displayed statically using the
firmware's native character set via watch_display_text_with_fallback, so
weekday abbreviations render correctly on the custom LCD.
The bottom row (HHMMSS) is animated with the scanning effect.
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

// 3-char DOW names for custom LCD (pos0, pos1, pos10 = left, centre-left, centre)
static const char *_wyoscan_dow_3[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
// 2-char fallback for classic LCD
static const char *_wyoscan_dow_2[] = {"Mo", "tu", "WE", "tH", "Fr", "SA", "Su"};

// Position indices used by watch_display_text_with_fallback(WATCH_POSITION_TOP, 5-char-str)
// Custom LCD maps char[0..4] → LCD positions {0,1,10,2,3}
static const uint8_t _top_positions[5] = {0, 1, 10, 2, 3};

// Returns 0=Monday ... 6=Sunday using Zeller's congruence.
// unit.year is years since 2020, so add 20 to get years since 2000.
static uint8_t _wyoscan_dow(watch_date_time_t dt) {
    uint8_t y = dt.unit.year + 20;
    uint8_t m = dt.unit.month;
    if (m <= 2) { m += 12; y--; }
    return (dt.unit.day + 13 * (m + 1) / 5 + y + y / 4 + 525 - 2) % 7;
}

// Store date strings into state from the given date-time.
static void _wyoscan_store_date(wyoscan_state_t *state, watch_date_time_t dt) {
    uint8_t dow = _wyoscan_dow(dt);
    sprintf(state->date_top, "%s%2d", _wyoscan_dow_3[dow], dt.unit.day);
    strncpy(state->date_classic, _wyoscan_dow_2[dow], 3);
}

// Reveal top row chars 0..upto (0-based) from stored date strings.
// Custom LCD: up to 5 chars at positions {0,1,10,2,3}.
// Classic LCD: up to 2 chars at positions 0,1 (DOW only).
static void _wyoscan_reveal_date_upto(wyoscan_state_t *state, uint8_t upto) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        if (upto > 4) upto = 4;
        for (uint8_t i = 0; i <= upto; i++)
            watch_display_character(state->date_top[i], _top_positions[i]);
    } else {
        if (upto > 1) upto = 1;
        for (uint8_t i = 0; i <= upto; i++)
            watch_display_character(state->date_classic[i], i);
    }
}

// Blank the top row.
static void _wyoscan_blank_top(void) {
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        for (uint8_t i = 0; i < 5; i++) watch_display_character(' ', _top_positions[i]);
    } else {
        watch_display_character(' ', 0);
        watch_display_character(' ', 1);
    }
}

/*
This is the mapping of input to the watch_set_pixel() function
for each position in hhmmss it defines the 2 dimension input at each of A-F*/
static const int32_t clock_mapping[6][7][2] = {
    // hour 1 (custom LCD position 4): A=3,16  B=2,16  C=1,16  D=0,16  E=1,22  F=3,22  G=2,22
    {{3,16}, {2,16}, {1,16}, {0,16}, {1,22}, {3,22}, {2,22}},
    // hour 2 (custom LCD position 5): A=3,14  B=2,14  C=1,14  D=0,15  E=1,15  F=3,15  G=2,15
    {{3,14}, {2,14}, {1,14}, {0,15}, {1,15}, {3,15}, {2,15}},
    // minute 1 (custom LCD position 6): A=3,1   B=2,2   C=0,2   D=0,1   E=1,1   F=2,1   G=1,2
    {{3,1}, {2,2}, {0,2}, {0,1}, {1,1}, {2,1}, {1,2}},
    // minute 2 (custom LCD position 7): A=3,3   B=2,4   C=0,4   D=0,3   E=1,3   F=2,3   G=1,4
    {{3,3}, {2,4}, {0,4}, {0,3}, {1,3}, {2,3}, {1,4}},
    // second 1 (custom LCD position 8): A=3,10  B=3,8   C=0,5   D=1,5   E=3,4   F=3,2   G=2,5
    {{3,10}, {3,8}, {0,5}, {1,5}, {3,4}, {3,2}, {2,5}},
    // second 2 (custom LCD position 9): A=3,6   B=3,7   C=2,7   D=0,7   E=0,6   F=2,6   G=1,6
    {{3,6}, {3,7}, {2,7}, {0,7}, {0,6}, {2,6}, {1,6}},
};

void wyoscan_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wyoscan_state_t));
        memset(*context_ptr, 0, sizeof(wyoscan_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void wyoscan_face_activate(void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;
    state->total_frames = 64;
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        movement_request_tick_frequency(32);
    } else {
        movement_request_tick_frequency(1); /* classic: static display, 1 Hz is enough */
    }
    // Pre-populate date strings; show the full date statically on activation.
    // The animation will blank and re-reveal it on the first tick cycle.
    watch_date_time_t dt = movement_get_local_date_time();
    _wyoscan_store_date(state, dt);
    _wyoscan_reveal_date_upto(state, 4);
}

bool wyoscan_face_loop(movement_event_t event, void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;

    watch_date_time_t date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // Date already shown by wyoscan_face_activate; nothing more to do.
            break;
        case EVENT_TICK:
            // Classic LCD: static time display with 1-Hz indicator blink; skip pixel animation.
            if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
                date_time = movement_get_local_date_time();
                _wyoscan_store_date(state, date_time);
                _wyoscan_reveal_date_upto(state, 4);
                {
                    char tbuf[3];
                    uint8_t h = date_time.unit.hour;
                    if (!movement_clock_mode_24h()) { h = h % 12; if (h == 0) h = 12; }
                    snprintf(tbuf, sizeof(tbuf), "%02u", (unsigned)h);
                    watch_display_text(WATCH_POSITION_HOURS, tbuf);
                    snprintf(tbuf, sizeof(tbuf), "%02u", (unsigned)date_time.unit.minute);
                    watch_display_text(WATCH_POSITION_MINUTES, tbuf);
                    snprintf(tbuf, sizeof(tbuf), "%02u", (unsigned)date_time.unit.second);
                    watch_display_text(WATCH_POSITION_SECONDS, tbuf);
                    bool blink_on = (date_time.unit.second % 2 == 0);
                    if (blink_on) watch_set_colon(); else watch_clear_colon();
                    bool is_pm = !movement_clock_mode_24h() && (date_time.unit.hour >= 12);
                    if (is_pm) {
                        if (blink_on) watch_set_indicator(WATCH_INDICATOR_PM);
                        else          watch_clear_indicator(WATCH_INDICATOR_PM);
                    } else {
                        watch_clear_indicator(WATCH_INDICATOR_PM);
                    }
                    if (movement_alarm_enabled()) {
                        if (blink_on) watch_set_indicator(WATCH_INDICATOR_BELL);
                        else          watch_clear_indicator(WATCH_INDICATOR_BELL);
                    } else {
                        watch_clear_indicator(WATCH_INDICATOR_BELL);
                    }
                    if (blink_on) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    else          watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                }
                break;
            }
            if (!state->animate) {
                date_time = movement_get_local_date_time();
                state->start = 0;
                state->end = 0;
                state->animation = 0;
                state->animate = true;
                // Store updated date strings and blank top row for animated reveal.
                _wyoscan_store_date(state, date_time);
                _wyoscan_blank_top();
                {
                    uint8_t hour = date_time.unit.hour;
                    if (!movement_clock_mode_24h()) {
                        hour = hour % 12;
                        if (hour == 0) hour = 12;
                    }
                    state->time_digits[0] = hour / 10;
                    state->time_digits[1] = hour % 10;
                }
                state->time_digits[2] = date_time.unit.minute / 10;
                state->time_digits[3] = date_time.unit.minute % 10;
                state->time_digits[4] = date_time.unit.second / 10;
                state->time_digits[5] = date_time.unit.second % 10;
                // skip leading zero on hour tens digit
                if (state->time_digits[0] == 0) state->animation = 8;
            }
            if ( state->animate ) {
                // Progressive top-row reveal: one char per 8-frame time-digit span.
                // animation/8 = 0..7; chars are at positions {0,1,10,2,3} (custom) / {0,1} (classic).
                if (state->animation % 8 == 0) {
                    _wyoscan_reveal_date_upto(state, (uint8_t)(state->animation / 8));
                }

                // Blink PM, BELL, and SIGNAL indicators at 1 Hz (16 frames on / 16 off at 32 fps).
                {
                    watch_date_time_t blink_dt = movement_get_local_date_time();
                    bool blink_on = (state->animation % 32) < 16;
                    bool is_pm = !movement_clock_mode_24h() && (blink_dt.unit.hour >= 12);
                    if (is_pm) {
                        if (blink_on) watch_set_indicator(WATCH_INDICATOR_PM);
                        else          watch_clear_indicator(WATCH_INDICATOR_PM);
                    } else {
                        watch_clear_indicator(WATCH_INDICATOR_PM);
                    }
                    if (movement_alarm_enabled()) {
                        if (blink_on) watch_set_indicator(WATCH_INDICATOR_BELL);
                        else          watch_clear_indicator(WATCH_INDICATOR_BELL);
                    } else {
                        watch_clear_indicator(WATCH_INDICATOR_BELL);
                    }
                    if (blink_on) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                    else          watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                }

                // if we have reached the max number of illuminated segments, we clear the oldest one
                if ((state->end + 1) % MAX_ILLUMINATED_SEGMENTS == state->start) {
                    // clear the oldest pixel if it's not 'X'
                    if (state->illuminated_segments[state->start][0] != 99 && state->illuminated_segments[state->start][1] != 99) {
                        watch_clear_pixel(state->illuminated_segments[state->start][0], state->illuminated_segments[state->start][1]);
                    }
                    // increment the start index to point to the next oldest pixel
                    state->start = (state->start + 1) % MAX_ILLUMINATED_SEGMENTS;
                }
                if (state->animation < state->total_frames - MAX_ILLUMINATED_SEGMENTS) {
                    if (state->animation % 32 == 0) {
                        if (state->colon) {
                            watch_set_colon();
                        } else {
                            watch_clear_colon();
                        }
                        state->colon = !state->colon;
                    }

                    // calculate the start position for the current frame
                    state->position = (state->animation / 8) % 6;
                    // calculate the current segment for the current digit
                    state->segment = state->animation % strlen(segment_map[state->time_digits[state->position]]);
                    // get the segments for the current digit
                    state->segments = segment_map[state->time_digits[state->position]];

                    if (state->segments[state->segment] == 'X') {
                        // if 'X', skip this frame
                        state->illuminated_segments[state->end][0] = 99;
                        state->illuminated_segments[state->end][1] = 99;
                        state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                        state->animation = (state->animation + 1);
                        break;
                    }

                    // calculate the animation frame
                    state->x = clock_mapping[state->position][state->segments[state->segment]-'A'][0];
                    state->y = clock_mapping[state->position][state->segments[state->segment]-'A'][1];

                    // set the new pixel
                    watch_set_pixel(state->x, state->y);

                    // store this pixel in the buffer
                    state->illuminated_segments[state->end][0] = state->x;
                    state->illuminated_segments[state->end][1] = state->y;
                    // increment the end index to the next position
                    state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                }
                else if (state->animation >= state->total_frames - MAX_ILLUMINATED_SEGMENTS && state->animation < state->total_frames) {
                    state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                }
                else {
                    // reset the animation state
                    state->animate = false;
                }
                state->animation = (state->animation + 1);
            }
            break;
        case EVENT_TIMEOUT:
            // stay on this face indefinitely
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
