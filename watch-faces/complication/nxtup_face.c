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

// Classic LCD: ticks to show each phase (name vs days) at 2Hz = 0.5s/tick
#define CLASSIC_NAME_TICKS  8   // 4 seconds of name scroll
#define CLASSIC_DAYS_TICKS  6   // 3 seconds of days display

// ─── date math ───────────────────────────────────────────────────────────────

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

// ─── sorting ─────────────────────────────────────────────────────────────────

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

// ─── display helpers ─────────────────────────────────────────────────────────

// Advance scroll position (call once per tick when scrolling)
static void _advance_scroll(nxtup_state_t *state) {
    const yearly_event_t *ev = &yearly_events[state->sorted_order[state->current_idx]];
    uint8_t len = (uint8_t)strlen(ev->name);
    if (len <= 6) return; // no scroll needed
    uint8_t cycle = len + 2; // name + 2-space gap before repeat
    state->scroll_pos = (state->scroll_pos + 1) % cycle;
}

static void _display_name(const char *name, uint8_t scroll_pos) {
    uint8_t len   = (uint8_t)strlen(name);
    char    buf[7];

    if (len <= 6) {
        // Short name — left-align, space-pad
        snprintf(buf, sizeof(buf), "%-6s", name);
    } else {
        // Scroll a 6-char window through name + 2-space gap
        uint8_t cycle = len + 2;
        for (uint8_t i = 0; i < 6; i++) {
            uint8_t idx = (scroll_pos + i) % cycle;
            buf[i] = (idx < len) ? name[idx] : ' ';
        }
        buf[6] = '\0';
    }
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void _display_days_bottom(uint16_t days) {
    char buf[7];
    if (days == 0) {
        watch_display_text(WATCH_POSITION_BOTTOM, "TODAY ");
    } else {
        unsigned d = days > 365 ? 365 : days;
        snprintf(buf, sizeof(buf), "  %3u ", d);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
}

static void _nxtup_update_display(nxtup_state_t *state) {
    if (state->event_count == 0) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "NX---", "NX");
        watch_display_text(WATCH_POSITION_BOTTOM, "nO EVt");
        return;
    }

    uint8_t  orig_idx = state->sorted_order[state->current_idx];
    uint16_t days     = state->days_until[orig_idx];
    const yearly_event_t *ev = &yearly_events[orig_idx];

    // Top: "NX" + 3-digit days (custom 5-char top); "NX" fallback on classic
    char top_buf[6];
    unsigned disp_days = days > 365 ? 365 : days;
    snprintf(top_buf, sizeof(top_buf), "NX%3u", disp_days);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, top_buf, "NX");

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // Custom LCD: show name scroll in bottom, days always visible in top
        if (days == 0) {
            watch_display_text(WATCH_POSITION_BOTTOM, "TODAY ");
        } else {
            _display_name(ev->name, state->scroll_pos);
        }
    } else {
        // Classic LCD: alternate between name and days
        if (state->classic_phase == 0) {
            _display_name(ev->name, state->scroll_pos);
        } else {
            _display_days_bottom(days);
        }
    }

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
    state->current_idx        = 0;
    state->scroll_pos         = 0;
    state->classic_phase      = 0;
    state->classic_phase_ticks = CLASSIC_NAME_TICKS;
    movement_request_tick_frequency(2); // 2 Hz for smooth scrolling
}

bool nxtup_face_loop(movement_event_t event, void *context) {
    nxtup_state_t *state = (nxtup_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _nxtup_update_display(state);
            break;

        case EVENT_TICK:
            if (state->event_count == 0) break;
            // Advance name scroll
            _advance_scroll(state);
            // Classic LCD: manage phase alternation
            if (watch_get_lcd_type() != WATCH_LCD_TYPE_CUSTOM) {
                if (state->classic_phase_ticks > 0) {
                    state->classic_phase_ticks--;
                } else {
                    state->classic_phase = !state->classic_phase;
                    state->classic_phase_ticks = state->classic_phase
                                                 ? CLASSIC_DAYS_TICKS
                                                 : CLASSIC_NAME_TICKS;
                }
            }
            // Recompute at midnight so days counts stay accurate
            if (event.subsecond == 0) {
                watch_date_time_t dt = movement_get_local_date_time();
                if (dt.unit.hour == 0 && dt.unit.minute == 0) {
                    _build_sorted_list(state);
                    state->current_idx = 0;
                }
            }
            _nxtup_update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Next event
            if (state->event_count > 0) {
                state->current_idx = (state->current_idx + 1) % state->event_count;
                state->scroll_pos  = 0;
                state->classic_phase       = 0;
                state->classic_phase_ticks = CLASSIC_NAME_TICKS;
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
                state->scroll_pos  = 0;
                state->classic_phase       = 0;
                state->classic_phase_ticks = CLASSIC_NAME_TICKS;
                _nxtup_update_display(state);
            }
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // Jump back to nearest upcoming event
            state->current_idx        = 0;
            state->scroll_pos         = 0;
            state->classic_phase      = 0;
            state->classic_phase_ticks = CLASSIC_NAME_TICKS;
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
