/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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
#include "day_one_face.h"
#include "watch.h"
#include "watch_utility.h"

static uint32_t _day_one_face_juliandaynum(uint16_t year, uint16_t month, uint16_t day) {
    // from here: https://en.wikipedia.org/wiki/Julian_day#Julian_day_number_calculation
    return (1461 * (year + 4800 + (month - 14) / 12)) / 4 + (367 * (month - 2 - 12 * ((month - 14) / 12))) / 12 - (3 * ((year + 4900 + (month - 14) / 12) / 100))/4 + day - 32075;
}

static void _day_one_face_update(day_one_state_t *state) {
    watch_date_time_t date_time = watch_rtc_get_date_time();
    uint32_t julian_date = _day_one_face_juliandaynum(date_time.unit.year + WATCH_RTC_REFERENCE_YEAR, date_time.unit.month, date_time.unit.day);
    uint32_t julian_birthdate = _day_one_face_juliandaynum(state->birth_year, state->birth_month, state->birth_day);
    uint32_t days = (julian_date < julian_birthdate) ? (julian_birthdate - julian_date) : (julian_date - julian_birthdate);

    // "DAYno" fills the 5-char top on custom LCD; classic falls back to "DA"
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "DAYno", "DA");

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // Custom LCD: far-left "1" pixel + 4 digits in hours+minutes (positions 4-7), seconds blank.
        // Handles 0-19999 days (~54 years). For 13287: pixel-1 + "3287".
        char num_buf[5];
        if (days >= 10000) {
            watch_set_pixel(0, 22);
            snprintf(num_buf, sizeof(num_buf), "%4u", (unsigned)(days % 10000));
        } else {
            watch_clear_pixel(0, 22);
            snprintf(num_buf, sizeof(num_buf), "%4u", (unsigned)days);
        }
        watch_display_text(WATCH_POSITION_HOURS,   num_buf);
        watch_display_text(WATCH_POSITION_MINUTES, num_buf + 2);
        watch_display_text(WATCH_POSITION_SECONDS, "  ");
    } else {
        // Classic LCD: 6-digit count in the full bottom row (positions 4-9)
        char num_buf[7];
        snprintf(num_buf, sizeof(num_buf), "%6u", (unsigned)days);
        watch_display_text(WATCH_POSITION_BOTTOM, num_buf);
    }
}

static void _day_one_face_abort_quick_cycle(day_one_state_t *state) {
    if (state->quick_cycle) {
        state->quick_cycle = false;
        movement_request_tick_frequency(4);
    }
}

static void _day_one_face_increment(day_one_state_t *state) {
    state->birthday_changed = true;
    switch (state->current_page) {
        case DAY_ONE_PAGE_YEAR:
            state->birth_year = state->birth_year + 1;
            if (state->birth_year > 2080) state->birth_year = 1900;
            break;
        case DAY_ONE_PAGE_MONTH:
            state->birth_month = (state->birth_month % 12) + 1;
            break;
        case DAY_ONE_PAGE_DAY:
            state->birth_day = state->birth_day + 1;
            break;
        default:
            break;
    }
    if (state->birth_day == 0 || state->birth_day > watch_utility_days_in_month(state->birth_month, state->birth_year))
        state->birth_day = 1;
}

void day_one_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(day_one_state_t));
        memset(*context_ptr, 0, sizeof(day_one_state_t));
        movement_birthdate_t movement_birthdate = (movement_birthdate_t) watch_get_backup_data(2);
        if (movement_birthdate.reg == 0) {
            // if birth date is totally blank, set a reasonable starting date. this works well for anyone under 63, but
            // you can keep pressing to go back to 1900; just pass the year 2080. also picked this date because if you
            // set it to 1959-01-02, it counts up from the launch of Luna-1, the first spacecraft to leave the well.
            movement_birthdate.bit.year = 1989;
            movement_birthdate.bit.month = 11;
            movement_birthdate.bit.day = 1;
            watch_store_backup_data(movement_birthdate.reg, 2);
        }
    }
}

void day_one_face_activate(void *context) {
    day_one_state_t *state = (day_one_state_t *)context;

    state->current_page = DAY_ONE_PAGE_DISPLAY;
    state->quick_cycle = false;
    state->ticks = 0;

    // fetch the user's birth date from the birthday register.
    movement_birthdate_t movement_birthdate = (movement_birthdate_t) watch_get_backup_data(2);
    state->birth_year = movement_birthdate.bit.year;
    state->birth_month = movement_birthdate.bit.month;
    state->birth_day = movement_birthdate.bit.day;
}

bool day_one_face_loop(movement_event_t event, void *context) {
    day_one_state_t *state = (day_one_state_t *)context;

    char buf[9];

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _day_one_face_update(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
        case EVENT_TICK:
            if (state->quick_cycle) {
                if (HAL_GPIO_BTN_ALARM_read()) {
                    _day_one_face_increment(state);
                } else {
                    _day_one_face_abort_quick_cycle(state);
                }
            }
            switch (state->current_page) {
                // if in settings mode, update whatever the current page is
                case DAY_ONE_PAGE_YEAR:
                    watch_display_text(WATCH_POSITION_FULL, "YR        ");
                    if (event.subsecond % 2) {
                        sprintf(buf, "%4d", state->birth_year);
                        watch_display_text(WATCH_POSITION_HOURS,   buf);
                        watch_display_text(WATCH_POSITION_MINUTES, buf + 2);
                    }
                    break;
                case DAY_ONE_PAGE_MONTH:
                    watch_display_text(WATCH_POSITION_FULL, "MO        ");
                    if (event.subsecond % 2) {
                        sprintf(buf, "%2d", state->birth_month);
                        watch_display_text(WATCH_POSITION_HOURS, buf);
                    }
                    break;
                case DAY_ONE_PAGE_DAY:
                    watch_display_text(WATCH_POSITION_FULL, "DA        ");
                    if (event.subsecond % 2) {
                        sprintf(buf, "%2d", state->birth_day);
                        watch_display_text(WATCH_POSITION_MINUTES, buf);
                    }
                    break;
                // Update every tick so the count is always correct if the face
                // is left on across midnight (the old second==0 check could miss
                // the midnight tick and leave a stale count until next activation).
                case DAY_ONE_PAGE_DISPLAY:
                    _day_one_face_update(state);
                    break;
                case DAY_ONE_PAGE_DATE:
                    if (state->ticks > 0) {
                        state->ticks--;
                    } else {
                        state->current_page = DAY_ONE_PAGE_DISPLAY;
                        _day_one_face_update(state);
                    }
                    break;
                default:
                    break;
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // only illuminate if we're in display mode
            switch (state->current_page) {
                case DAY_ONE_PAGE_DISPLAY:
                    // fall through
                case DAY_ONE_PAGE_DATE:
                    movement_illuminate_led();
                    break;
                default:
                    break;
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // otherwise use the light button to advance settings pages.
            switch (state->current_page) {
                case DAY_ONE_PAGE_YEAR:
                    // fall through
                case DAY_ONE_PAGE_MONTH:
                    // fall through
                case DAY_ONE_PAGE_DAY:
                    // go to next setting page...
                    state->current_page = (state->current_page + 1) % 4;
                    if (state->current_page == DAY_ONE_PAGE_DISPLAY) {
                        // ...unless we've been pushed back to display mode.
                        movement_request_tick_frequency(1);
                        // force display since it normally won't update til midnight.
                        _day_one_face_update(state);
                    }
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            // if we are on a settings page, increment whatever value we're setting.
            switch (state->current_page) {
                case DAY_ONE_PAGE_YEAR:
                    // fall through
                case DAY_ONE_PAGE_MONTH:
                    // fall through
                case DAY_ONE_PAGE_DAY:
                    _day_one_face_abort_quick_cycle(state);
                    _day_one_face_increment(state);
                    break;
                case DAY_ONE_PAGE_DISPLAY:
                    state->current_page = DAY_ONE_PAGE_DATE;
                    sprintf(buf, "%04d%02d%02d", state->birth_year % 10000, state->birth_month % 100, state->birth_day % 100);
                    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                    watch_display_text(WATCH_POSITION_HOURS,     buf + 2);
                    watch_display_text(WATCH_POSITION_MINUTES,   buf + 4);
                    watch_display_text(WATCH_POSITION_SECONDS,   buf + 6);
                    state->ticks = 2;
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            // if we aren't already in settings mode, put us there.
            switch (state->current_page) {
                case DAY_ONE_PAGE_DISPLAY:
                    state->current_page++;
                    movement_request_tick_frequency(4);
                    break;
                case DAY_ONE_PAGE_YEAR:
                    // fall through
                case DAY_ONE_PAGE_MONTH:
                    // fall through
                case DAY_ONE_PAGE_DAY:
                    state->quick_cycle = true;
                    movement_request_tick_frequency(8);
                    break;
                default:
                    break;
            }
            break;
        case EVENT_ALARM_LONG_UP:
            _day_one_face_abort_quick_cycle(state);
            break;
        case EVENT_TIMEOUT:
            _day_one_face_abort_quick_cycle(state);
            // return home if we're on a settings page (this saves our changes when we resign).
            if (state->current_page != DAY_ONE_PAGE_DISPLAY) {
                movement_move_to_face(0);
            }
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void day_one_face_resign(void *context) {
    day_one_state_t *state = (day_one_state_t *)context;

    // if the user changed their birth date, store it to the birth date register
    if (state->birthday_changed) {
        day_one_state_t *state = (day_one_state_t *)context;
        movement_birthdate_t movement_birthdate = (movement_birthdate_t) watch_get_backup_data(2);
        movement_birthdate.bit.year = state->birth_year;
        movement_birthdate.bit.month = state->birth_month;
        movement_birthdate.bit.day = state->birth_day;
        watch_store_backup_data(movement_birthdate.reg, 2);
        state->birthday_changed = false;
    }
}
