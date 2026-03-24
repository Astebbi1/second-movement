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
    return watch_utility_date_time_to_unix_time(movement_get_local_date_time(), 0);
}

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
            state->prev_sleep_seconds = 0; // reset before new sleep period
            state->mode = AWAKE_MODE_SLEEP;
            state->mode_start_epoch = state->last_step_epoch;
        }
    } else {
        if (new_steps) {
            state->prev_sleep_seconds = (now > state->mode_start_epoch)
                                        ? (now - state->mode_start_epoch) : 0;
            state->mode = AWAKE_MODE_AWAKE;
            state->mode_start_epoch = now;
            state->show_prev_sleep = false;
        }
    }
}

static void _awake_update_display(awake_state_t *state) {
    char buf[5];
    uint32_t now = _awake_now_epoch();
    uint32_t elapsed;

    if (state->show_prev_sleep && state->prev_sleep_seconds > 0) {
        // Showing last sleep duration — use "SLEPT" so it's clearly history
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEPT", "SL");
        elapsed = state->prev_sleep_seconds;
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
    } else if (state->mode == AWAKE_MODE_AWAKE) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "AWAKE", "AW");
        elapsed = (now > state->mode_start_epoch) ? (now - state->mode_start_epoch) : 0;
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
    } else {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEEP", "SL");
        elapsed = (now > state->mode_start_epoch) ? (now - state->mode_start_epoch) : 0;
        watch_set_indicator(WATCH_INDICATOR_SLEEP);
    }

    uint16_t hours   = (uint16_t)(elapsed / 3600);
    uint8_t  minutes = (uint8_t)((elapsed % 3600) / 60);
    if (hours > 99) hours = 99;

    snprintf(buf, sizeof(buf), "%02u%02u", hours, minutes);
    watch_set_colon();
    watch_display_text(WATCH_POSITION_HOURS,   buf);
    watch_display_text(WATCH_POSITION_MINUTES, buf + 2);
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
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
        state->mode_start_epoch = now;
        state->last_step_epoch  = now;
        state->last_step_count  = 0;
    }
}

void awake_face_activate(void *context) {
    awake_state_t *state = (awake_state_t *)context;

    // Guard against stale epoch stored before the RTC was set.
    uint32_t now = _awake_now_epoch();
    if (now > state->mode_start_epoch && (now - state->mode_start_epoch) > 86400) {
        state->mode              = AWAKE_MODE_AWAKE;
        state->mode_start_epoch  = now;
        state->last_step_epoch   = now;
        state->last_step_count   = 0;
        state->prev_sleep_seconds = 0;
        state->show_prev_sleep   = false;
    }

    state->last_step_count = movement_get_step_count();
    _awake_check_activity(state);
    movement_request_tick_frequency(1);
}

bool awake_face_loop(movement_event_t event, void *context) {
    awake_state_t *state = (awake_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _awake_update_display(state);
            break;

        case EVENT_BACKGROUND_TASK:
            _awake_check_activity(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Toggle showing last sleep duration vs. current mode time
            if (state->prev_sleep_seconds > 0) {
                state->show_prev_sleep = !state->show_prev_sleep;
            } else {
                state->show_prev_sleep = false;
            }
            _awake_update_display(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Manual "I just woke up" — reset awake timer to right now
            state->mode             = AWAKE_MODE_AWAKE;
            state->mode_start_epoch = _awake_now_epoch();
            state->last_step_epoch  = state->mode_start_epoch;
            state->last_step_count  = movement_get_step_count();
            state->show_prev_sleep  = false;
            _awake_update_display(state);
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
    advisory.wants_background_task = true;
    return advisory;
}
