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
A single comet wave sweeps left-to-right across the ENTIRE display every 3.3
seconds (107 frames at 32 fps), touching every character and indicator in order:

  Phase 1 (frames   0-39): top row chars, segment-by-segment
                            pos0 (DOW[0]), pos1 (DOW[1]), pos10 (DOW[2]),
                            pos2 (date tens), pos3 (date units)
  Phase 2 (frames  40-42): indicator column — PM, BELL, SIGNAL, one per frame
  Phase 3 (frames  43-90): time digits — H1 H2 : M1 M2 : S1 S2
  Phase 4 (frames 91-106): trailing comet clearance (16 frames)

A ring buffer of MAX_ILLUMINATED_SEGMENTS=16 pixels creates the comet tail.
Old pixels are erased as new ones are added, giving the glowing-trail effect.
When an indicator pixel enters the ring buffer it lights for ~15 frames then
is naturally extinguished as the tail advances — no independent blink logic.

Top-row characters are drawn using Custom_LCD_Character_Set (bit per segment)
and Custom_LCD_Display_Mapping (segment → com/seg pixel address), the same
tables used by watch_display_character().  Time digits use the hand-crafted
segment_map / clock_mapping from the original wyoscan implementation.

The animation runs on the custom LCD only; classic LCD shows a static display
updated 1 Hz with the same 1-Hz indicator blink as before.

Created to mimic the wyoscan watch that was produced by Halmos and designed by
Dexter Sinister.  https://www.o-r-g.com/apps/wyoscan

Segment naming convention (A-G standard):
  A
F   B
  G
E   C
  D
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

// 3-char DOW names for custom LCD (pos0, pos1, pos10)
static const char *_wyoscan_dow_3[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
// 2-char fallback for classic LCD
static const char *_wyoscan_dow_2[] = {"Mo", "tu", "WE", "tH", "Fr", "SA", "Su"};

// Custom LCD top-row position order (physical left-to-right):
// char[0]→pos0, char[1]→pos1, char[2]→pos10, char[3]→pos2, char[4]→pos3
static const uint8_t _top_positions[5] = {0, 1, 10, 2, 3};

// Returns 0=Monday ... 6=Sunday (Zeller's congruence).
// unit.year is years since 2020; add 20 to get years since 2000.
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

// Blank the top row (custom LCD).
static void _wyoscan_blank_top(void) {
    for (uint8_t i = 0; i < 5; i++) watch_display_character(' ', _top_positions[i]);
}

/*
Pixel coordinates for each segment (A-G) of each time-digit position.
Index [pos][seg_letter-'A'] → {com, seg} for watch_set_pixel / watch_clear_pixel.
*/
static const int32_t clock_mapping[6][7][2] = {
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

/* Indicator pixel coordinates for the custom LCD (from _watch_update_indicator_segments):
   PM   = com 3, seg 21
   BELL = com 1, seg 21
   SIGNAL = com 0, seg 21                                          */
#define IND_PM_COM     3
#define IND_PM_SEG     21
#define IND_BELL_COM   1
#define IND_BELL_SEG   21
#define IND_SIG_COM    0
#define IND_SIG_SEG    21

/* Total animation frames:
   40 top-row + 3 indicators + 48 time digits + 16 clearance = 107  */
#define WYOSCAN_TOTAL_FRAMES 107

void wyoscan_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(wyoscan_state_t));
        memset(*context_ptr, 0, sizeof(wyoscan_state_t));
    }
}

void wyoscan_face_activate(void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;
    state->total_frames = WYOSCAN_TOTAL_FRAMES;
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        movement_request_tick_frequency(32);
        // Show full date immediately; the wave will blank and re-reveal on first cycle.
        watch_date_time_t dt = movement_get_local_date_time();
        _wyoscan_store_date(state, dt);
        for (uint8_t i = 0; i < 5; i++)
            watch_display_character(state->date_top[i], _top_positions[i]);
    } else {
        // Classic LCD: 4 Hz sweep animation.  Show a static full display immediately,
        // then the first tick will start the left-to-right reveal cycle.
        movement_request_tick_frequency(4);
        watch_date_time_t dt = movement_get_local_date_time();
        _wyoscan_store_date(state, dt);
        state->animation = 0;
        uint8_t h = dt.unit.hour;
        bool is_pm = !movement_clock_mode_24h() && (h >= 12);
        if (!movement_clock_mode_24h()) { h = h % 12; if (h == 0) h = 12; }
        char buf[11];
        snprintf(buf, sizeof(buf), "%c%c%2u%2u%02u%02u",
            state->date_classic[0], state->date_classic[1],
            (unsigned)dt.unit.day, (unsigned)h,
            (unsigned)dt.unit.minute, (unsigned)dt.unit.second);
        watch_display_text(WATCH_POSITION_FULL, buf);
        watch_set_colon();
        if (is_pm) watch_set_indicator(WATCH_INDICATOR_PM);
        if (movement_alarm_enabled()) watch_set_indicator(WATCH_INDICATOR_BELL);
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    }
}

bool wyoscan_face_loop(movement_event_t event, void *context) {
    wyoscan_state_t *state = (wyoscan_state_t *)context;

    watch_date_time_t date_time;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            // Display was already set up in wyoscan_face_activate.
            break;

        case EVENT_TICK:
            // ── Classic LCD: left-to-right sweep animation at 4 Hz ───────────
            // 8-phase cycle (0.25 s/phase = 2 s/cycle):
            //   0: clear + DOW  1: date day  2: hour + colon  3: minute
            //   4: second + indicators (full display)  5-7: hold, update seconds
            if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
                uint8_t phase = state->animation;
                char buf[3];
                if (phase == 0) {
                    // Snapshot time; clear display; reveal DOW only.
                    date_time = movement_get_local_date_time();
                    _wyoscan_store_date(state, date_time);
                    uint8_t h = date_time.unit.hour;
                    bool is_pm = !movement_clock_mode_24h() && (h >= 12);
                    if (!movement_clock_mode_24h()) { h = h % 12; if (h == 0) h = 12; }
                    state->time_digits[0] = h;
                    state->time_digits[1] = date_time.unit.minute;
                    state->time_digits[2] = date_time.unit.second;
                    state->time_digits[3] = date_time.unit.day;
                    state->time_digits[4] = is_pm ? 1 : 0;
                    watch_display_text(WATCH_POSITION_FULL, "          ");
                    watch_clear_colon();
                    watch_clear_indicator(WATCH_INDICATOR_PM);
                    watch_clear_indicator(WATCH_INDICATOR_BELL);
                    watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
                    watch_display_character(state->date_classic[0], 0);
                    watch_display_character(state->date_classic[1], 1);
                } else if (phase == 1) {
                    // Reveal: date day of month.
                    snprintf(buf, sizeof(buf), "%2u", (unsigned)state->time_digits[3]);
                    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                } else if (phase == 2) {
                    // Reveal: hour + colon.
                    snprintf(buf, sizeof(buf), "%2u", (unsigned)state->time_digits[0]);
                    watch_display_text(WATCH_POSITION_HOURS, buf);
                    watch_set_colon();
                } else if (phase == 3) {
                    // Reveal: minute.
                    snprintf(buf, sizeof(buf), "%02u", (unsigned)state->time_digits[1]);
                    watch_display_text(WATCH_POSITION_MINUTES, buf);
                } else if (phase == 4) {
                    // Reveal: second — display now complete; set indicators.
                    snprintf(buf, sizeof(buf), "%02u", (unsigned)state->time_digits[2]);
                    watch_display_text(WATCH_POSITION_SECONDS, buf);
                    if (state->time_digits[4]) watch_set_indicator(WATCH_INDICATOR_PM);
                    if (movement_alarm_enabled()) watch_set_indicator(WATCH_INDICATOR_BELL);
                    watch_set_indicator(WATCH_INDICATOR_SIGNAL);
                } else {
                    // Hold phases 5-7: update seconds live.
                    date_time = movement_get_local_date_time();
                    snprintf(buf, sizeof(buf), "%02u", (unsigned)date_time.unit.second);
                    watch_display_text(WATCH_POSITION_SECONDS, buf);
                }
                state->animation = (state->animation + 1) % 8;
                break;
            }

            // ── Custom LCD: comet wave animation ──────────────────────────────
            if (!state->animate) {
                date_time = movement_get_local_date_time();
                state->start = 0;
                state->end = 0;
                state->animation = 0;
                state->animate = true;
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
                // Leading-zero skip for hour tens is handled in the draw logic.
            }

            if (state->animate) {
                // Erase the oldest comet pixel when the ring buffer is full.
                if ((state->end + 1) % MAX_ILLUMINATED_SEGMENTS == state->start) {
                    uint32_t ox = state->illuminated_segments[state->start][0];
                    uint32_t oy = state->illuminated_segments[state->start][1];
                    if (ox != 99 && oy != 99) watch_clear_pixel((uint8_t)ox, (uint8_t)oy);
                    state->start = (state->start + 1) % MAX_ILLUMINATED_SEGMENTS;
                }

                if (state->animation < state->total_frames - MAX_ILLUMINATED_SEGMENTS) {
                    // Colon blinks at ~1 Hz (every 32 frames at 32 fps).
                    if (state->animation % 32 == 0) {
                        if (state->colon) watch_set_colon(); else watch_clear_colon();
                        state->colon = !state->colon;
                    }

                    uint8_t anim = state->animation;
                    bool is_skip = true;
                    uint8_t px = 0, py = 0;

                    if (anim < 40) {
                        // ── Phase 1: top-row characters ─────────────────────
                        // 5 chars × 8 segments each, using the character set
                        // and position pixel map from watch_common_display.h.
                        uint8_t char_idx = anim / 8;
                        uint8_t seg_idx  = anim % 8;
                        uint8_t lcd_pos  = _top_positions[char_idx];
                        char c = state->date_top[char_idx];
                        if (c >= 0x20 && c < 0x80) {
                            uint8_t bits = Custom_LCD_Character_Set[(uint8_t)(c - 0x20)];
                            const segment_mapping_t *seg =
                                &Custom_LCD_Display_Mapping[lcd_pos].segment[seg_idx];
                            if (((bits >> seg_idx) & 1) &&
                                (seg->value != segment_does_not_exist)) {
                                px = seg->address.com;
                                py = seg->address.seg;
                                is_skip = false;
                            }
                        }

                    } else if (anim < 43) {
                        // ── Phase 2: indicator column ────────────────────────
                        // One indicator per frame; each enters the ring buffer
                        // and is cleared naturally ~15 frames later.
                        uint8_t idx = anim - 40;
                        if (idx == 0) {
                            // PM: only show when in 12h mode and afternoon
                            date_time = movement_get_local_date_time();
                            if (!movement_clock_mode_24h() && date_time.unit.hour >= 12) {
                                px = IND_PM_COM; py = IND_PM_SEG; is_skip = false;
                            }
                        } else if (idx == 1) {
                            // BELL: only when alarm is enabled
                            if (movement_alarm_enabled()) {
                                px = IND_BELL_COM; py = IND_BELL_SEG; is_skip = false;
                            }
                        } else {
                            // SIGNAL: always present on this face
                            px = IND_SIG_COM; py = IND_SIG_SEG; is_skip = false;
                        }

                    } else {
                        // ── Phase 3: time digits ─────────────────────────────
                        // 6 digits × 8 frames, using clock_mapping pixel coords.
                        uint8_t adjusted = anim - 43;
                        uint8_t position = adjusted / 8;
                        uint8_t seg_idx  = adjusted % 8;
                        // Blank the hour-tens position when it is zero.
                        if (!(position == 0 && state->time_digits[0] == 0)) {
                            const char *segs = segment_map[state->time_digits[position]];
                            if (segs[seg_idx] != 'X') {
                                px = (uint8_t)clock_mapping[position][segs[seg_idx]-'A'][0];
                                py = (uint8_t)clock_mapping[position][segs[seg_idx]-'A'][1];
                                is_skip = false;
                            }
                        }
                    }

                    // Commit to ring buffer.
                    if (is_skip) {
                        state->illuminated_segments[state->end][0] = 99;
                        state->illuminated_segments[state->end][1] = 99;
                    } else {
                        watch_set_pixel(px, py);
                        state->illuminated_segments[state->end][0] = px;
                        state->illuminated_segments[state->end][1] = py;
                    }
                    state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;

                } else if (state->animation < state->total_frames) {
                    // Trailing clearance: advance the tail with no new pixels.
                    state->end = (state->end + 1) % MAX_ILLUMINATED_SEGMENTS;
                } else {
                    state->animate = false;
                }
                state->animation++;
            }
            break;

        case EVENT_TIMEOUT:
            // Stay on this face indefinitely.
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
