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

#ifndef AWAKE_FACE_H_
#define AWAKE_FACE_H_

/*
 * AWAKE face — automatic sleep/wake tracker using step counting
 * =============================================================
 * Detects awake vs. sleep state based on accelerometer step activity.
 * After AWAKE_SLEEP_THRESHOLD_MINUTES of no steps, transitions to SLEEP mode.
 * Any new steps while in SLEEP mode transitions back to AWAKE.
 *
 * AWAKE mode display:
 *   Top: "AWAKE" (custom) / "AW" (classic)
 *   Bottom: HH:MM elapsed since waking
 *   Sleep indicator: OFF
 *
 * SLEEP mode display:
 *   Top: "SLEEP" (custom) / "SL" (classic)
 *   Bottom: HH:MM elapsed since falling asleep
 *   Sleep indicator: ON
 *
 * ALARM button (while AWAKE): show/dismiss previous sleep duration.
 *   Displays "SLEEP HH:MM" with sleep indicator for 8 seconds.
 *
 * LIGHT long-press (while SLEEP): false-sleep correction.
 *   Reverts to AWAKE mode, restoring the original wake start time.
 *   Use this when the watch detects sleep but you were just sitting still
 *   or had the watch off for a while.
 *
 * Monitoring runs every minute as a background task, even on other faces.
 * Requires step counting to be enabled (step_counter_face enables it).
 */

#include "movement.h"

// Minutes of step inactivity before declaring sleep
#define AWAKE_SLEEP_THRESHOLD_MINUTES 30
#define AWAKE_SLEEP_THRESHOLD_SEC     (AWAKE_SLEEP_THRESHOLD_MINUTES * 60)

// Seconds to display previous sleep duration on ALARM press
#define AWAKE_PREV_SLEEP_DISPLAY_TICKS 8

typedef enum {
    AWAKE_MODE_AWAKE = 0,
    AWAKE_MODE_SLEEP,
} awake_mode_t;

typedef struct {
    awake_mode_t mode;
    bool show_prev_sleep;           // currently showing previous sleep on ALARM press
    uint8_t prev_sleep_ticks;       // countdown for show_prev_sleep display
    uint32_t mode_start_epoch;      // epoch when current mode (awake/sleep) started
    uint32_t prev_awake_start_epoch;// saved wake start for false-sleep correction
    uint32_t prev_sleep_seconds;    // duration of last completed sleep period
    uint32_t last_step_epoch;       // epoch when steps were last detected
    uint32_t last_step_count;       // step count at last background check
} awake_state_t;

void awake_face_setup(uint8_t watch_face_index, void **context_ptr);
void awake_face_activate(void *context);
bool awake_face_loop(movement_event_t event, void *context);
void awake_face_resign(void *context);
movement_watch_face_advisory_t awake_face_advise(void *context);

#define awake_face ((const watch_face_t){ \
    awake_face_setup, \
    awake_face_activate, \
    awake_face_loop, \
    awake_face_resign, \
    awake_face_advise, \
})

#endif // AWAKE_FACE_H_
