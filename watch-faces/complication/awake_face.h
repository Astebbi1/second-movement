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
 * AWAKE face — sleep/wake tracker using step counting
 * ====================================================
 * Detects awake vs. sleep state based on accelerometer step activity.
 * After AWAKE_SLEEP_THRESHOLD_MINUTES of no steps, transitions to SLEEP mode.
 * Any new steps while in SLEEP mode transitions back to AWAKE.
 *
 * AWAKE mode:
 *   Top: "AWAKE"    Bottom: HH:MM time since waking
 *
 * SLEEP mode:
 *   Top: "SLEEP"    Bottom: HH:MM time sleeping    Crescent moon ON
 *
 * ALARM tap: toggle showing last sleep duration ("SLEPT HH:MM").
 *   Press again to return to current mode view.
 *
 * ALARM long-press: manual "I just woke up" reset.
 *   Forces AWAKE mode and resets the awake timer to right now.
 *   Use this after taking the watch off, long naps, etc.
 *
 * Monitoring runs every minute as a background task on any face.
 * Requires step counting to be enabled (step_counter_face enables it).
 */

#include "movement.h"

// Minutes of step inactivity before declaring sleep
#define AWAKE_SLEEP_THRESHOLD_MINUTES 60
#define AWAKE_SLEEP_THRESHOLD_SEC     (AWAKE_SLEEP_THRESHOLD_MINUTES * 60)

typedef enum {
    AWAKE_MODE_AWAKE = 0,
    AWAKE_MODE_SLEEP,
} awake_mode_t;

typedef struct {
    awake_mode_t mode;
    bool show_prev_sleep;           // toggle: showing last sleep duration instead of current
    uint32_t mode_start_epoch;      // epoch when current mode (awake/sleep) started
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
