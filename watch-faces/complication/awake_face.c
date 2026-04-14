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
#include "awake_face.h"
#include "watch_utility.h"

// ─── helpers ─────────────────────────────────────────────────────────────────

static uint32_t _now_epoch(void) {
    return watch_utility_date_time_to_unix_time(movement_get_local_date_time(), 0);
}

// Returns temperature in tenths of °C (e.g. 325 = 32.5 °C).
// Returns INT16_MIN if sensor is unavailable.
static int16_t _read_temp_tenths(void) {
    float t = movement_get_temperature();
    if (t > 1000.0f) return INT16_MIN;
    return (int16_t)(t * 10.0f);
}

// Exponential moving average over up to 8 samples (weight 7/8 old + 1/8 new).
static void _update_wrist_temp(awake_state_t *state, int16_t temp) {
    if (state->wrist_temp_samples == 0) {
        state->wrist_temp_avg     = temp;
        state->wrist_temp_samples = 1;
        return;
    }
    if (state->wrist_temp_samples < 8) state->wrist_temp_samples++;
    state->wrist_temp_avg = (int16_t)((state->wrist_temp_avg * 7 + temp + 4) / 8);
}

// True when temp has dropped far enough below the established wrist baseline.
static bool _is_off_wrist(awake_state_t *state, int16_t temp) {
    if (state->wrist_temp_samples < 3) return false; // no reliable baseline yet
    if (temp == INT16_MIN) return false;
    return (state->wrist_temp_avg - temp) >= AWAKE_OFFWRIST_TEMP_TENTHS;
}

// Commit the current sleep period to last_sleep_seconds if it meets the minimum.
static void _finalize_sleep(awake_state_t *state, uint32_t now) {
    if (state->sleep_start_epoch == 0) return;
    uint32_t dur = (now > state->sleep_start_epoch)
                   ? (now - state->sleep_start_epoch) : 0;
    if (dur >= AWAKE_MIN_SLEEP_SEC)
        state->last_sleep_seconds = dur;
    state->sleep_start_epoch = 0;
}

// ─── display ─────────────────────────────────────────────────────────────────

static void _show_duration(uint32_t seconds) {
    char buf[3];
    uint16_t h = (uint16_t)(seconds / 3600);
    uint8_t  m = (uint8_t)((seconds % 3600) / 60);
    if (h > 99) h = 99;
    watch_set_colon();
    snprintf(buf, sizeof(buf), "%2u", h);
    watch_display_text(WATCH_POSITION_HOURS, buf);
    snprintf(buf, sizeof(buf), "%02u", m);
    watch_display_text(WATCH_POSITION_MINUTES, buf);
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
}

static void _show_no_data(void) {
    watch_clear_colon();
    watch_display_text(WATCH_POSITION_HOURS,   "  ");
    watch_display_text(WATCH_POSITION_MINUTES, "--");
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
}

static void _awake_update_display(awake_state_t *state) {
    uint32_t now = _now_epoch();

    switch (state->page) {

        case 0: // AW — current state: time awake, or time into sleep if currently asleep
            if (state->mode == AWAKE_STATE_ASLEEP) {
                watch_set_indicator(WATCH_INDICATOR_SLEEP);
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEEP", "SL");
                uint32_t slp = (now > state->sleep_start_epoch)
                               ? (now - state->sleep_start_epoch) : 0;
                _show_duration(slp);
            } else {
                watch_clear_indicator(WATCH_INDICATOR_SLEEP);
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "AWAKE", "AW");
                uint32_t awk = (now > state->wake_epoch)
                               ? (now - state->wake_epoch) : 0;
                _show_duration(awk);
            }
            break;

        case 1: // SL — last completed sleep duration
            watch_clear_indicator(WATCH_INDICATOR_SLEEP);
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEPT", "SL");
            if (state->last_sleep_seconds > 0)
                _show_duration(state->last_sleep_seconds);
            else
                _show_no_data();
            break;

        case 2: // Wr — current (or last) continuous wear session
            watch_clear_indicator(WATCH_INDICATOR_SLEEP);
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "WEAR ", "Wr");
            if (state->mode == AWAKE_STATE_OFF_WRIST) {
                // Frozen: show how long the last on-wrist session lasted
                uint32_t frozen = (state->off_wrist_start_epoch > state->wear_session_start)
                                  ? (state->off_wrist_start_epoch - state->wear_session_start) : 0;
                if (frozen > 0) _show_duration(frozen);
                else            _show_no_data();
            } else {
                uint32_t wear = (now > state->wear_session_start)
                                ? (now - state->wear_session_start) : 0;
                _show_duration(wear);
            }
            break;

        case 3: // oF — last (or current) off-wrist duration
            watch_clear_indicator(WATCH_INDICATOR_SLEEP);
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "OFF  ", "oF");
            if (state->mode == AWAKE_STATE_OFF_WRIST) {
                // Currently off wrist — count up
                uint32_t off = (now > state->off_wrist_start_epoch)
                               ? (now - state->off_wrist_start_epoch) : 0;
                _show_duration(off);
            } else if (state->last_off_wrist_seconds > 0) {
                _show_duration(state->last_off_wrist_seconds);
            } else {
                _show_no_data();
            }
            break;

        case 4: // LG — longest continuous wear session (since power-on)
            watch_clear_indicator(WATCH_INDICATOR_SLEEP);
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "LONG ", "LG");
            if (state->longest_wear_seconds > 0)
                _show_duration(state->longest_wear_seconds);
            else
                _show_no_data();
            break;
    }
}

// ─── poll logic ──────────────────────────────────────────────────────────────

static void _do_poll(awake_state_t *state) {
    uint32_t now   = _now_epoch();
    int16_t  temp  = _read_temp_tenths();
    uint32_t steps = movement_get_step_count();
    bool had_motion = (steps > state->last_step_count);
    state->last_step_count = steps;

    watch_date_time_t dt = movement_get_local_date_time();
    uint8_t hour = dt.unit.hour;

    // ── Off-wrist detection ───────────────────────────────────────────────────
    if (_is_off_wrist(state, temp)) {
        if (state->mode != AWAKE_STATE_OFF_WRIST) {
            // Entering off-wrist: close any active sleep period first
            if (state->mode == AWAKE_STATE_ASLEEP)
                _finalize_sleep(state, now);
            state->off_wrist_start_epoch = now;
            state->still_poll_count      = 0;
        }
        state->mode = AWAKE_STATE_OFF_WRIST;
        return;
    }

    // ── Returning from off-wrist ──────────────────────────────────────────────
    if (state->mode == AWAKE_STATE_OFF_WRIST) {
        uint32_t off_dur = (now > state->off_wrist_start_epoch)
                           ? (now - state->off_wrist_start_epoch) : 0;
        if (off_dur > 0)
            state->last_off_wrist_seconds = off_dur;
        state->wear_session_start = now;  // new wear session begins
        state->mode               = AWAKE_STATE_AWAKE;
        state->wake_epoch         = now;
        state->still_poll_count   = 0;
        // Don't update wrist temp yet — let the sensor stabilise on-wrist
        return;
    }

    // ── Update wrist temperature baseline ────────────────────────────────────
    if (temp != INT16_MIN) _update_wrist_temp(state, temp);

    // ── Update longest wear streak ────────────────────────────────────────────
    uint32_t wear_dur = (now > state->wear_session_start)
                        ? (now - state->wear_session_start) : 0;
    if (wear_dur > state->longest_wear_seconds)
        state->longest_wear_seconds = wear_dur;

    // ── Determine sleep-eligible windows ─────────────────────────────────────
    // Night window: 10 pm – 8 am  |  Nap window: 1 pm – 6 pm
    bool in_night  = (hour >= AWAKE_SLEEP_START_HOUR || hour < AWAKE_SLEEP_END_HOUR);
    bool in_nap    = (hour >= AWAKE_NAP_START_HOUR   && hour < AWAKE_NAP_END_HOUR);
    bool in_window = (in_night || in_nap);
    uint8_t threshold = in_nap ? AWAKE_NAP_POLLS : AWAKE_NIGHT_SLEEP_POLLS;

    // ── Sleep state machine ───────────────────────────────────────────────────
    if (state->mode == AWAKE_STATE_ASLEEP) {
        if (had_motion || !in_window) {
            // Motion detected, or we've left the sleep window → wake up
            _finalize_sleep(state, now);
            state->mode             = AWAKE_STATE_AWAKE;
            state->wake_epoch       = now;
            state->still_poll_count = 0;
        }
        // else: still in window with no motion → remain confirmed asleep

    } else {
        // AWAKE: only accumulate stillness inside a sleep-eligible window
        if (had_motion || !in_window) {
            state->still_poll_count = 0;
        } else {
            // In window, no motion — work toward sleep confirmation
            if (state->still_poll_count == 0)
                // Backdate one poll interval: stillness probably started last poll
                state->still_since_epoch = now - (AWAKE_POLL_MINUTES * 60UL);
            state->still_poll_count++;

            if (state->still_poll_count >= threshold) {
                // Enough consecutive still polls — confirm sleep
                state->mode              = AWAKE_STATE_ASLEEP;
                state->sleep_start_epoch = state->still_since_epoch;
                state->still_poll_count  = 0;
            }
        }
    }
}

// ─── face callbacks ──────────────────────────────────────────────────────────

void awake_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(awake_state_t));
        awake_state_t *state = (awake_state_t *)*context_ptr;
        memset(state, 0, sizeof(awake_state_t));

        uint32_t now = _now_epoch();
        state->mode               = AWAKE_STATE_AWAKE;
        state->wake_epoch         = now;
        state->wear_session_start = now;
        state->page               = 0;

        int16_t temp = _read_temp_tenths();
        if (temp != INT16_MIN) {
            state->wrist_temp_avg     = temp;
            state->wrist_temp_samples = 1;
        }
    }
}

void awake_face_activate(void *context) {
    awake_state_t *state = (awake_state_t *)context;
    state->page = 0; // always open on the current-state page
    _awake_update_display(state);
}

bool awake_face_loop(movement_event_t event, void *context) {
    awake_state_t *state = (awake_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _awake_update_display(state);
            break;

        case EVENT_BACKGROUND_TASK:
            _do_poll(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            state->page = (state->page + 1) % 5;
            _awake_update_display(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            break; // suppress LED on short press; handled in BUTTON_UP

        case EVENT_LIGHT_BUTTON_UP:
            state->page = (state->page + 4) % 5; // cycle backward
            _awake_update_display(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Manual "I just woke up" — corrects algorithm drift without clearing other stats
            state->mode             = AWAKE_STATE_AWAKE;
            state->wake_epoch       = _now_epoch();
            state->still_poll_count = 0;
            state->page             = 0;
            _awake_update_display(state);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void awake_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t awake_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t advisory = { 0 };
    watch_date_time_t dt = movement_get_local_date_time();
    // Poll every 15 minutes (fires at :00, :15, :30, :45)
    advisory.wants_background_task = (dt.unit.minute % AWAKE_POLL_MINUTES == 0);
    return advisory;
}
