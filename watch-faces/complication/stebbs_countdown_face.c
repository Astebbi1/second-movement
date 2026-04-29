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
#include "stebbs_countdown_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "melody_library.h"
#include "movement_custom_signal_tunes.h"

// ── Presets ────────────────────────────────────────────────────────────────────

#define SCD_NUM_PRESETS 7
static const uint16_t _scd_presets[SCD_NUM_PRESETS] = {
    30, 60, 180, 300, 600, 900, 1800
};

// ── Tunes ──────────────────────────────────────────────────────────────────────

#define SCD_NUM_TUNES 10

typedef struct {
    char abbr[3];    // 2-char display abbreviation (null-terminated)
    bool is_signal;  // true = signal_tunes[], false = melody_tunes[]
    uint8_t idx;     // index into the chosen library
} scd_tune_t;

// User-selected tunes in order. melody_tunes indices from melody_library.c,
// signal_tunes indices from movement_custom_signal_tunes.c.
static const scd_tune_t _scd_tunes[SCD_NUM_TUNES] = {
    { "GL", false,  0 },  // Gambler
    { "PK", false,  2 },  // Pokemon
    { "NK", false,  5 },  // Nickelodeon
    { "JP", false,  7 },  // Jurassic Park
    { "PU", false,  9 },  // Mario Power-Up
    { "XP", false, 10 },  // Windows XP
    { "rG", false, 14 },  // Power Rangers
    { "WE", false, 13 },  // Westminster
    { "Cn", false,  6 },  // Carmen
    { "bP", true,   0 },  // Default beep
};

// ── Helpers ────────────────────────────────────────────────────────────────────

static uint32_t _scd_now_unix(void) {
    return watch_utility_date_time_to_unix_time(
        movement_get_utc_date_time(), movement_get_current_timezone_offset());
}

static void _scd_play_tune(scd_state_t *state) {
    const scd_tune_t *t = &_scd_tunes[state->tune_idx];
    const int8_t *seq = t->is_signal
        ? signal_tunes[t->idx].sequence
        : melody_tunes[t->idx].sequence;
    movement_play_sequence((int8_t *)seq, BUZZER_PRIORITY_ALARM);
}

// Start (or restart) the timer for a given number of seconds.
static void _scd_schedule(scd_state_t *state, uint32_t seconds) {
    uint32_t now = _scd_now_unix();
    state->now_ts = now;
    state->target_ts = now + seconds;
    watch_date_time_t target_dt = watch_utility_date_time_from_unix_time(
        state->target_ts, movement_get_current_timezone_offset());
    movement_schedule_background_task_for_face(state->watch_face_index, target_dt);
    state->mode = SCD_RUNNING;
}

static void _scd_reset(scd_state_t *state) {
    movement_cancel_background_task_for_face(state->watch_face_index);
    state->mode = SCD_IDLE;
    watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static void _scd_draw(scd_state_t *state) {
    char buf[7];
    uint32_t remaining;

    switch (state->mode) {
        case SCD_RUNNING:
            remaining = (state->target_ts > state->now_ts)
                        ? (state->target_ts - state->now_ts) : 0;
            break;
        case SCD_PAUSED:
            remaining = state->paused_remaining;
            break;
        case SCD_IDLE:
        default:
            remaining = _scd_presets[state->preset_idx];
            break;
    }

    uint8_t mins = (uint8_t)(remaining / 60);
    uint8_t secs = (uint8_t)(remaining % 60);

    // Bottom: "MM:SSab" — colon between positions 5/6 makes this read as MM:SS,
    // the 2-letter tune abbr in the seconds position flags it as a timer not a clock.
    sprintf(buf, "%02d%02d%s", mins, secs, _scd_tunes[state->tune_idx].abbr);
    watch_set_colon();
    // Top-left: face label; top-right (date position): fire count 0–31.
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "TiMr", "Ti");
    char fcbuf[3];
    sprintf(fcbuf, "%2u", state->fire_count);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, fcbuf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    // LAP = repeat on; BELL = timer running
    if (state->repeat) watch_set_indicator(WATCH_INDICATOR_LAP);
    else               watch_clear_indicator(WATCH_INDICATOR_LAP);

    if (state->mode == SCD_RUNNING) watch_set_indicator(WATCH_INDICATOR_BELL);
    else                            watch_clear_indicator(WATCH_INDICATOR_BELL);
}

// ── Face callbacks ─────────────────────────────────────────────────────────────

void stebbs_countdown_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(scd_state_t));
        scd_state_t *state = (scd_state_t *)*context_ptr;
        memset(state, 0, sizeof(scd_state_t));
        state->watch_face_index = watch_face_index;
        state->preset_idx = 3;  // default: 5 min
        state->tune_idx   = 0;  // default: Gambler
        state->repeat     = true;
        state->mode = SCD_IDLE;
    }
}

void stebbs_countdown_face_activate(void *context) {
    scd_state_t *state = (scd_state_t *)context;
    watch_set_colon();
    if (state->mode == SCD_RUNNING) {
        // Resync now_ts — we may have been away from this face for a while.
        state->now_ts = _scd_now_unix();
        watch_set_indicator(WATCH_INDICATOR_BELL);
    }
    if (state->repeat) watch_set_indicator(WATCH_INDICATOR_LAP);
    movement_request_tick_frequency(1);
}

bool stebbs_countdown_face_loop(movement_event_t event, void *context) {
    scd_state_t *state = (scd_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _scd_draw(state);
            break;

        case EVENT_TICK:
            if (state->mode == SCD_RUNNING) state->now_ts++;
            _scd_draw(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            switch (state->mode) {
                case SCD_IDLE:
                    // Cycle to next preset
                    state->preset_idx = (state->preset_idx + 1) % SCD_NUM_PRESETS;
                    break;
                case SCD_RUNNING:
                    // Pause: save remaining seconds and cancel the background task
                    state->paused_remaining = (state->target_ts > state->now_ts)
                                             ? (state->target_ts - state->now_ts) : 0;
                    movement_cancel_background_task_for_face(state->watch_face_index);
                    state->mode = SCD_PAUSED;
                    break;
                case SCD_PAUSED:
                    // Resume from where we left off
                    _scd_schedule(state, state->paused_remaining);
                    break;
            }
            _scd_draw(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            switch (state->mode) {
                case SCD_IDLE:
                    // Fresh start — reset fire count then schedule
                    state->fire_count = 0;
                    _scd_schedule(state, _scd_presets[state->preset_idx]);
                    break;
                case SCD_RUNNING:
                case SCD_PAUSED:
                    // Manual abort — reset fire count and return to idle
                    _scd_reset(state);
                    state->fire_count = 0;
                    break;
            }
            _scd_draw(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            // Squelch default LED-on behaviour so LIGHT_BUTTON_UP can decide.
            break;

        case EVENT_LIGHT_BUTTON_UP:
            if (state->mode == SCD_IDLE) {
                // Cycle to next tune and play a preview
                state->tune_idx = (state->tune_idx + 1) % SCD_NUM_TUNES;
                const scd_tune_t *t = &_scd_tunes[state->tune_idx];
                const int8_t *seq = t->is_signal
                    ? signal_tunes[t->idx].sequence
                    : melody_tunes[t->idx].sequence;
                watch_buzzer_play_sequence((int8_t *)seq, NULL);
            } else {
                // LED when running or paused
                movement_illuminate_led();
            }
            _scd_draw(state);
            break;

        case EVENT_LIGHT_LONG_PRESS:
            if (state->mode == SCD_IDLE) {
                state->repeat = !state->repeat;
                _scd_draw(state);
            }
            break;

        case EVENT_BACKGROUND_TASK:
            // Timer fired — increment the fire counter (capped at 31), then play tune.
            state->fire_count = (state->fire_count < 31) ? state->fire_count + 1 : 31;
            _scd_play_tune(state);
            if (state->repeat) {
                // Immediately restart with the same preset
                _scd_schedule(state, _scd_presets[state->preset_idx]);
            } else {
                _scd_reset(state);
            }
            _scd_draw(state);
            break;

        case EVENT_TIMEOUT:
            // Don't time out while the timer is running
            if (state->mode != SCD_RUNNING) {
                movement_move_to_face(0);
            }
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            // Only reached when timer is stopped (running timers hold a background
            // task which prevents low-energy mode). Nothing to do.
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void stebbs_countdown_face_resign(void *context) {
    // Background task (if any) stays scheduled so the timer keeps firing
    // even when navigated away. Nothing to clean up.
    (void) context;
}
