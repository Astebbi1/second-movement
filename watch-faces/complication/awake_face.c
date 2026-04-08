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

// Returns temperature in tenths of °C (e.g. 325 = 32.5°C).
// Returns INT16_MIN if sensor unavailable.
static int16_t _read_temp_tenths(void) {
    float t = movement_get_temperature();
    if (t > 1000.0f) return INT16_MIN; // sentinel for unavailable
    return (int16_t)(t * 10.0f);
}

// Exponential moving average over up to 8 samples.
static void _update_wrist_temp(awake_state_t *state, int16_t temp) {
    if (state->wrist_temp_samples == 0) {
        state->wrist_temp_avg = temp;
        state->wrist_temp_samples = 1;
        return;
    }
    if (state->wrist_temp_samples < 8) state->wrist_temp_samples++;
    // new_avg = old * 7/8 + temp * 1/8
    state->wrist_temp_avg = (int16_t)((state->wrist_temp_avg * 7 + temp + 4) / 8);
}

// True if current temp has dropped far enough below the wrist baseline.
static bool _is_off_wrist(awake_state_t *state, int16_t temp) {
    if (state->wrist_temp_samples < 3) return false; // no baseline yet
    if (temp == INT16_MIN) return false;
    return (state->wrist_temp_avg - temp) >= AWAKE_OFFWRIST_TEMP_TENTHS;
}

// ─── display ─────────────────────────────────────────────────────────────────

static void _show_duration(uint32_t seconds) {
    char buf[5];
    uint16_t h = (uint16_t)(seconds / 3600);
    uint8_t  m = (uint8_t)((seconds % 3600) / 60);
    if (h > 99) h = 99;
    watch_set_colon();
    snprintf(buf, sizeof(buf), "%02u%02u", h, m);
    watch_display_text(WATCH_POSITION_HOURS,   buf);
    watch_display_text(WATCH_POSITION_MINUTES, buf + 2);
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
}

static void _awake_update_display(awake_state_t *state) {
    uint32_t now = _now_epoch();

    if (state->mode == AWAKE_STATE_OFF_WRIST) {
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "OFF  ", "OF");
        watch_display_text(WATCH_POSITION_BOTTOM, "WrISt ");
        return;
    }

    if (state->show_sleep) {
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEPT", "SL");
        if (state->last_sleep_seconds > 0) {
            _show_duration(state->last_sleep_seconds);
        } else {
            watch_display_text(WATCH_POSITION_BOTTOM, " none ");
        }
        return;
    }

    if (state->mode == AWAKE_STATE_ASLEEP) {
        watch_set_indicator(WATCH_INDICATOR_SLEEP);
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "SLEEP", "SL");
        uint32_t elapsed = (now > state->sleep_start_epoch)
                           ? (now - state->sleep_start_epoch) : 0;
        _show_duration(elapsed);
    } else {
        // AWAKE or MAYBE_SLEEPING — show awake duration
        watch_clear_indicator(WATCH_INDICATOR_SLEEP);
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "AWAKE", "AW");
        uint32_t elapsed = (now > state->wake_epoch)
                           ? (now - state->wake_epoch) : 0;
        _show_duration(elapsed);
    }
}

// ─── poll logic ──────────────────────────────────────────────────────────────

static void _do_poll(awake_state_t *state) {
    uint32_t now = _now_epoch();
    int16_t  temp = _read_temp_tenths();
    uint32_t steps = movement_get_step_count();
    bool had_motion = (steps > state->last_step_count);
    state->last_step_count = steps;

    // --- Off-wrist detection ---
    if (_is_off_wrist(state, temp)) {
        if (state->mode != AWAKE_STATE_OFF_WRIST) {
            // Finalise any in-progress confirmed sleep before removing
            if (state->mode == AWAKE_STATE_ASLEEP) {
                uint32_t dur = (now > state->sleep_start_epoch)
                               ? (now - state->sleep_start_epoch) : 0;
                if (dur >= AWAKE_MIN_SLEEP_SEC)
                    state->last_sleep_seconds = dur;
            }
            state->mode = AWAKE_STATE_OFF_WRIST;
            state->still_poll_count = 0;
        }
        return;
    }

    // --- Returning from off-wrist ---
    if (state->mode == AWAKE_STATE_OFF_WRIST) {
        state->mode = AWAKE_STATE_AWAKE;
        state->wake_epoch = now;
        state->still_poll_count = 0;
        // Don't update wrist temp yet — wait for it to stabilise
        return;
    }

    // --- Update wrist temp baseline (only when clearly on wrist) ---
    if (temp != INT16_MIN) {
        _update_wrist_temp(state, temp);
    }

    // --- Motion / stillness ---
    if (had_motion) {
        // Reset stillness counter in all cases
        state->still_poll_count = 0;
        if (state->mode == AWAKE_STATE_MAYBE_SLEEPING)
            state->mode = AWAKE_STATE_AWAKE;

        if (state->mode == AWAKE_STATE_ASLEEP) {
            // Accumulate active polls — need AWAKE_WAKE_POLLS consecutive to confirm wake
            if (state->active_poll_count == 0)
                state->first_active_epoch = now - (AWAKE_POLL_MINUTES * 60UL);
            state->active_poll_count++;

            if (state->active_poll_count >= AWAKE_WAKE_POLLS) {
                // Sustained motion confirmed — finalise sleep, backdate wake to first active poll
                uint32_t sleep_end = state->first_active_epoch;
                uint32_t dur = (sleep_end > state->sleep_start_epoch)
                               ? (sleep_end - state->sleep_start_epoch) : 0;
                if (dur >= AWAKE_MIN_SLEEP_SEC)
                    state->last_sleep_seconds = dur;
                state->mode             = AWAKE_STATE_AWAKE;
                state->wake_epoch       = state->first_active_epoch;
                state->active_poll_count = 0;
            }
        } else {
            state->active_poll_count = 0;
        }

    } else {
        // No motion — reset wake counter so brief trips (bathroom, shifting) don't accumulate
        state->active_poll_count = 0;
        if (state->mode == AWAKE_STATE_AWAKE ||
            state->mode == AWAKE_STATE_MAYBE_SLEEPING) {

            if (state->still_poll_count == 0) {
                // Record when stillness started (backdate by one poll interval)
                state->still_since_epoch = now - (AWAKE_POLL_MINUTES * 60UL);
                state->mode = AWAKE_STATE_MAYBE_SLEEPING;
            }
            state->still_poll_count++;

            if (state->still_poll_count >= AWAKE_SLEEP_POLLS) {
                // Enough consecutive still polls — confirm sleep
                state->mode = AWAKE_STATE_ASLEEP;
                state->sleep_start_epoch = state->still_since_epoch;
            }
        }
        // If already AWAKE_STATE_ASLEEP: stay asleep, nothing to do
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
        state->mode       = AWAKE_STATE_AWAKE;
        state->wake_epoch = now;

        int16_t temp = _read_temp_tenths();
        if (temp != INT16_MIN) {
            state->wrist_temp_avg     = temp;
            state->wrist_temp_samples = 1;
        }
    }
}

void awake_face_activate(void *context) {
    awake_state_t *state = (awake_state_t *)context;
    state->show_sleep = false;
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
            // Toggle between current-mode view and last sleep duration.
            // Always allow toggle — SLEPT view shows " none " if no sleep recorded yet.
            state->show_sleep = !state->show_sleep;
            _awake_update_display(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Manual "I just woke up"
            state->mode             = AWAKE_STATE_AWAKE;
            state->wake_epoch       = _now_epoch();
            state->still_poll_count = 0;
            state->show_sleep       = false;
            _awake_update_display(state);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            // "Still awake" dead man's switch — resets stillness counter
            state->still_poll_count = 0;
            if (state->mode == AWAKE_STATE_MAYBE_SLEEPING)
                state->mode = AWAKE_STATE_AWAKE;
            _awake_update_display(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            break; // suppress LED

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
    advisory.wants_background_task = (dt.unit.minute % AWAKE_POLL_MINUTES == 0);
    return advisory;
}
