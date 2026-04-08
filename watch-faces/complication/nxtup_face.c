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
#include "nxtup_face.h"
#include "movement_custom_events.h"

// ─── date / calendar helpers ──────────────────────────────────────────────────

// Julian Day Number — same algorithm as day_one_face
static uint32_t _jdn(uint16_t y, uint8_t m, uint8_t d) {
    return (1461 * (y + 4800 + (m - 14) / 12)) / 4
         + (367  * (m - 2 - 12 * ((m - 14) / 12))) / 12
         - (3    * ((y + 4900 + (m - 14) / 12) / 100)) / 4
         + d - 32075;
}

// Clamp event day to valid range for the given month/year (handles Feb on non-leap years)
static uint8_t _clamp_day(uint8_t month, uint8_t day, uint16_t year) {
    static const uint8_t days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    uint8_t max_day = days_in_month[month];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) max_day = 29;
    return (day > max_day) ? max_day : day;
}

static uint16_t _days_until_event(uint8_t event_month, uint8_t event_day) {
    watch_date_time_t today = movement_get_local_date_time();
    uint16_t y = today.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint32_t today_jdn = _jdn(y, today.unit.month, today.unit.day);

    // This year's occurrence
    uint8_t  clamped_day = _clamp_day(event_month, event_day, y);
    uint32_t event_jdn   = _jdn(y, event_month, clamped_day);

    if (event_jdn >= today_jdn) {
        return (uint16_t)(event_jdn - today_jdn);
    }
    // Already passed — use next year
    clamped_day = _clamp_day(event_month, event_day, y + 1);
    event_jdn   = _jdn(y + 1, event_month, clamped_day);
    return (uint16_t)(event_jdn - today_jdn);
}

// ─── sorting ──────────────────────────────────────────────────────────────────

static void _build_sorted_list(nxtup_state_t *state) {
    uint8_t count = YEARLY_EVENT_COUNT;
    if (count > NXTUP_MAX_EVENTS) count = NXTUP_MAX_EVENTS;
    state->event_count = count;

    // Populate days_until for every event
    for (uint8_t i = 0; i < count; i++) {
        state->days_until[i]   = _days_until_event(yearly_events[i].month,
                                                    yearly_events[i].day);
        state->sorted_order[i] = i;
    }

    // Insertion sort sorted_order by days_until ascending
    for (uint8_t i = 1; i < count; i++) {
        uint8_t  key_ord  = state->sorted_order[i];
        uint16_t key_days = state->days_until[key_ord];
        int8_t   j        = (int8_t)i - 1;
        while (j >= 0 && state->days_until[state->sorted_order[j]] > key_days) {
            state->sorted_order[j + 1] = state->sorted_order[j];
            j--;
        }
        state->sorted_order[j + 1] = key_ord;
    }
}

// ─── scroll string ────────────────────────────────────────────────────────────

// (jdn + 1) % 7 → 0=Sun, 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat
static const char *_nxtup_dow_names[] = {
    "Sun","Mon","tuE","WEd","tHu","Fri","SAt"
};

// Month abbreviations chosen for segment-display readability (1-indexed)
static const char *_nxtup_month_names[] = {
    "", "JAn","FEb","MAr","APr","MAY","Jun",
    "JuL","AuG","SEP","OCt","noV","dEC"
};

// Builds the scroll string into state->scroll_buf.
// Custom LCD: "   name DOW Mon day  " — days shown in top-right, no need in scroll.
// Classic LCD: "   name DOW Mon day -Xd  " — days appended since classic has no top-right.
// Trailing 2 spaces act as the gap before the loop repeats.
// Also resets scroll_pos to 0.
static void _build_scroll_string(nxtup_state_t *state) {
    uint8_t orig_idx = state->sorted_order[state->current_idx];
    const yearly_event_t *ev = &yearly_events[orig_idx];
    uint16_t days = state->days_until[orig_idx];

    // Compute the day-of-week the event falls on
    watch_date_time_t today = movement_get_local_date_time();
    uint16_t y = today.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint32_t today_jdn = _jdn(y, today.unit.month, today.unit.day);
    uint32_t event_jdn = today_jdn + days;
    uint8_t  dow = (uint8_t)((event_jdn + 1) % 7);

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        snprintf(state->scroll_buf, sizeof(state->scroll_buf),
                 "   %s %s %s %u  ",
                 ev->name,
                 _nxtup_dow_names[dow],
                 _nxtup_month_names[ev->month],
                 ev->day);
    } else {
        // Classic LCD: append " -Xd" so the days count scrolls past with the event.
        unsigned disp_days = days > 365 ? 365 : days;
        snprintf(state->scroll_buf, sizeof(state->scroll_buf),
                 "   %s %s %s %u -%ud  ",
                 ev->name,
                 _nxtup_dow_names[dow],
                 _nxtup_month_names[ev->month],
                 ev->day,
                 disp_days);
    }
    state->scroll_buf_len = (uint8_t)strlen(state->scroll_buf);
    state->scroll_pos = 0;
}

// ─── display helpers ──────────────────────────────────────────────────────────

static void _nxtup_update_display(nxtup_state_t *state) {
    if (state->event_count == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "NX---", "NU");
        watch_display_text(WATCH_POSITION_BOTTOM, "nO EVt");
        return;
    }

    uint8_t  orig_idx = state->sorted_order[state->current_idx];
    uint16_t days     = state->days_until[orig_idx];

    // Top: "NX" + 3-digit days remaining (custom 5-char top); "NU" fallback on classic.
    // Classic: also use top-right 2 chars to show days when event is within 39 days
    // (the date-digit segments only support 0-3 in the tens place, so 39 is the max).
    char top_buf[6];
    unsigned disp_days = days > 365 ? 365 : days;
    snprintf(top_buf, sizeof(top_buf), "NX%3u", disp_days);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, top_buf, "NU");

    if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
        if (days <= 39) {
            char tr[3];
            snprintf(tr, sizeof(tr), "%2u", days);
            watch_display_text(WATCH_POSITION_TOP_RIGHT, tr);
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
        }
    }

    // Bottom: 6-char window into the scroll string, wraps seamlessly via modulo
    uint8_t len = state->scroll_buf_len;
    char disp[7];
    for (uint8_t i = 0; i < 6; i++) {
        disp[i] = state->scroll_buf[(state->scroll_pos + i) % len];
    }
    disp[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, disp);

    watch_clear_colon();
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
}

// ─── face callbacks ───────────────────────────────────────────────────────────

void nxtup_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(nxtup_state_t));
        memset(*context_ptr, 0, sizeof(nxtup_state_t));
    }
}

void nxtup_face_activate(void *context) {
    nxtup_state_t *state = (nxtup_state_t *)context;
    _build_sorted_list(state);
    state->current_idx = 0;
    _build_scroll_string(state);
    // Classic LCD: 8 Hz ticks, advance scroll every 3rd tick = 375 ms/char.
    // Custom LCD: 4 Hz ticks, advance every tick = 250 ms/char.
    movement_request_tick_frequency((watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) ? 8 : 4);
}

bool nxtup_face_loop(movement_event_t event, void *context) {
    nxtup_state_t *state = (nxtup_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _nxtup_update_display(state);
            break;

        case EVENT_TICK:
            if (state->event_count == 0) break;

            // Classic LCD runs at 8 Hz; only advance scroll every 3rd tick (375 ms/char).
            // Custom LCD runs at 4 Hz and advances every tick (250 ms/char).
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
                state->scroll_tick = (state->scroll_tick + 1) % 3;
                if (state->scroll_tick != 0) break;
            }

            // Advance scroll one position per tick
            state->scroll_pos = (state->scroll_pos + 1) % state->scroll_buf_len;

            // Recompute at midnight so days counts and DOW stay accurate
            if (event.subsecond == 0) {
                watch_date_time_t dt = movement_get_local_date_time();
                if (dt.unit.hour == 0 && dt.unit.minute == 0) {
                    _build_sorted_list(state);
                    state->current_idx = 0;
                    _build_scroll_string(state);
                }
            }
            _nxtup_update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Next event
            if (state->event_count > 0) {
                state->current_idx = (state->current_idx + 1) % state->event_count;
                _build_scroll_string(state);
                _nxtup_update_display(state);
            }
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            // Suppress LED
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Previous event
            if (state->event_count > 0) {
                state->current_idx = (state->current_idx == 0)
                                     ? (state->event_count - 1)
                                     : (state->current_idx - 1);
                _build_scroll_string(state);
                _nxtup_update_display(state);
            }
            break;

        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Jump back to nearest upcoming event
            state->current_idx = 0;
            _build_scroll_string(state);
            _nxtup_update_display(state);
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void nxtup_face_resign(void *context) {
    (void) context;
    movement_request_tick_frequency(1);
}
