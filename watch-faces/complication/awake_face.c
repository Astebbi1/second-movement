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

static uint32_t _awake_now_epoch(void) {
    // UTC offset not needed — we only ever compute deltas between two calls
    return watch_utility_date_time_to_unix_time(movement_get_local_date_time(), 0);
}

// Check step count and advance the state machine.
// Called every minute from EVENT_BACKGROUND_TASK (any face) and on EVENT_ACTIVATE.
static void _awake_check_activity(awake_state_t *state) {
    uint32_t now = _awake_now_epoch();
    uint32_t current_steps = movement_get_step_count();

    bool new_steps = (current_steps > state->last_step_count);
    state->last_step_count = current_steps;

    if (new_steps) {
        state->last_step_epoch = now;
    }

    uint32_t inactive_sec = (now > state->last_step_epoch)
                            ? (now - state->last_step_epoch) : 0;

    if (state->mode == AWAKE_MODE_AWAKE) {
        if (inactive_sec >= AWAKE_SLEEP_THRESHOLD_SEC) {
            // Awake → sleep: sleep start = last confirmed activity
            state->prev_awake_start_epoch = state->mode_start_epoch;
            state->mode = AWAKE_MODE_SLEEP;
            state->mode_start_epoch = state->last_step_epoch;
        }
    } else {
        // Sleep mode: any new steps → wake up
        if (new_steps) {
            state->prev_sleep_seconds = (now > state->mode_start_epoch)
                                        ? (now - state->mode_start_epoch) : 0;
            state->mode = AWAKE_MODE_AWAKE;
            state->mode_start_epoch = now;
            // Reset show_prev_sleep so pressing ALARM will show the fresh sleep total
            state->show_prev_sleep = false;
        }
    }
}

static void _awake_update_display(awake_state_t *state) {
    char buf[5];
    uint32_t now = _awake_now_epoch();
    uint32_t elapsed;
    bool show_sleep_indicator;

    if (state->show_prev_sleep) {
        // Temporarily showing last sleep duration while in AWAKE mode
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEEP", "SL");
        elapsed = state->prev_sleep_seconds;
        show_sleep_indicator = true;
    } else if (state->mode == AWAKE_MODE_AWAKE) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "AWAKE", "AW");
        elapsed = (now > state->mode_start_epoch) ? (now - state->mode_start_epoch) : 0;
        show_sleep_indicator = false;
    } else {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEEP", "SL");
        elapsed = (now > state->mode_start_epoch) ? (now - state->mode_start_epoch) : 0;
        show_sleep_indicator = true;
    }

    uint16_t hours   = (uint16_t)(elapsed / 3600);
    uint8_t  minutes = (uint8_t)((elapsed % 3600) / 60);
    if (hours > 99) hours = 99;

    snprintf(buf, sizeof(buf), "%02u%02u", hours, minutes);
    watch_set_colon();
    watch_display_text(WATCH_POSITION_HOURS,   buf);
    watch_display_text(WATCH_POSITION_MINUTES, buf + 2);
    watch_display_text(WATCH_POSITION_SECONDS, "  ");

    // WATCH_INDICATOR_SLEEP is a crescent moon, available on the custom LCD
    if (show_sleep_indicator) {
        watch_set_indicator(WATCH_INDICATOR_SLEEP);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
    }
}

// ─── face callbacks ───────────────────────────────────────────────────────────

void awake_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(awake_state_t));
        awake_state_t *state = (awake_state_t *)*context_ptr;
        memset(state, 0, sizeof(awake_state_t));

        uint32_t now = _awake_now_epoch();
        state->mode = AWAKE_MODE_AWAKE;
        state->mode_start_epoch      = now;
        state->prev_awake_start_epoch = now;
        state->last_step_epoch       = now;
        state->last_step_count       = 0;
    }
}

void awake_face_activate(void *context) {
    awake_state_t *state = (awake_state_t *)context;

    // Guard against a stale epoch stored before the RTC was set (e.g. right
    // after flashing). If the saved start time is more than 24 h in the past,
    // the value is clearly wrong — reset to now so the display doesn't rail at 99:xx.
    uint32_t now = _awake_now_epoch();
    if (now > state->mode_start_epoch && (now - state->mode_start_epoch) > 86400) {
        state->mode                   = AWAKE_MODE_AWAKE;
        state->mode_start_epoch       = now;
        state->prev_awake_start_epoch = now;
        state->last_step_epoch        = now;
        state->last_step_count        = 0;
        state->prev_sleep_seconds     = 0;
        state->show_prev_sleep        = false;
    }

    // Snapshot current step count so the first check has a baseline
    state->last_step_count = movement_get_step_count();
    _awake_check_activity(state);
    movement_request_tick_frequency(1);
}

bool awake_face_loop(movement_event_t event, void *context) {
    awake_state_t *state = (awake_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _awake_update_display(state);
            break;

        case EVENT_TICK:
            // Count down the prev-sleep view
            if (state->show_prev_sleep) {
                if (state->prev_sleep_ticks > 0) {
                    state->prev_sleep_ticks--;
                } else {
                    state->show_prev_sleep = false;
                }
            }
            _awake_update_display(state);
            break;

        case EVENT_BACKGROUND_TASK:
            // Runs every minute regardless of active face; only update state, not display
            _awake_check_activity(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // While AWAKE: toggle the previous sleep readout
            if (state->mode == AWAKE_MODE_AWAKE) {
                if (state->show_prev_sleep) {
                    state->show_prev_sleep = false;
                } else if (state->prev_sleep_seconds > 0) {
                    state->show_prev_sleep = true;
                    state->prev_sleep_ticks = AWAKE_PREV_SLEEP_DISPLAY_TICKS;
                }
                _awake_update_display(state);
            }
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            // Suppress LED
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // False-sleep correction: revert to AWAKE, restoring original wake start time
            if (state->mode == AWAKE_MODE_SLEEP) {
                state->mode = AWAKE_MODE_AWAKE;
                state->mode_start_epoch = state->prev_awake_start_epoch;
                // Reset activity timer so we don't immediately re-detect sleep
                state->last_step_epoch  = _awake_now_epoch();
                state->last_step_count  = movement_get_step_count();
                state->show_prev_sleep  = false;
                _awake_update_display(state);
            }
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void awake_face_resign(void *context) {
    (void) context;
    // Step counting managed by step_counter_face; don't touch it here.
    // State continues to update via EVENT_BACKGROUND_TASK (advise).
}

// Called every minute by movement firmware — always request background task
// so the state machine runs even when this face is not the active one.
movement_watch_face_advisory_t awake_face_advise(void *context) {
    (void) context;
    movement_watch_face_advisory_t advisory = { 0 };
    advisory.wants_background_task = true;
    return advisory;
}
