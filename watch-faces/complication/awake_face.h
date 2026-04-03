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
 * AWAKE face — sleep/wake tracker v2
 * ====================================
 * Polls every 20 minutes via background task. Uses step count delta and
 * wrist temperature together to detect sleep, waking, and watch removal.
 *
 * SLEEP detection:
 *   After AWAKE_SLEEP_POLLS consecutive still polls (~100 min), confirms sleep
 *   and backdates sleep start to when stillness began. Ignores sleep blocks
 *   shorter than AWAKE_MIN_SLEEP_SEC (filters naps / desk vegging).
 *
 * WAKE detection:
 *   Any poll with step count increase while confirmed asleep → immediate wake.
 *
 * OFF-WRIST detection:
 *   Temperature drops more than AWAKE_OFFWRIST_TEMP_TENTHS (6°C) below the
 *   rolling wrist average → watch is on a table, excluded from tracking.
 *
 * ALARM button:  toggle between "time awake" and "last sleep duration"
 * ALARM long:    manual "I just woke up" reset
 * LIGHT long:    manual "I'm still awake" — resets the still poll counter
 *                (use this when lying in bed doom scrolling)
 *
 * Target accuracy: ~35 minutes (one poll interval).
 *
 * Display:
 *   AWAKE mode:  top "AWAKE", bottom HH:MM time awake
 *   SLEEP mode:  top "SLEEP", bottom HH:MM time asleep, moon indicator on
 *   SLEPT view:  top "SLEPT", bottom HH:MM last sleep duration (alarm toggle)
 *   OFF-WRIST:   top "OFF  ", bottom "WrISt"
 */

#include "movement.h"

#define AWAKE_POLL_MINUTES         20     // background poll interval
#define AWAKE_SLEEP_POLLS           5     // 5 × 20 min ≈ 100 min → confirm sleep
#define AWAKE_WAKE_POLLS            2     // 2 × 20 min = 40 min of sustained motion → confirm wake
#define AWAKE_OFFWRIST_TEMP_TENTHS 60     // 6.0 °C drop from wrist avg → off wrist
#define AWAKE_MIN_SLEEP_SEC      (30 * 60)// ignore sleep blocks shorter than 30 min

typedef enum {
    AWAKE_STATE_AWAKE = 0,
    AWAKE_STATE_MAYBE_SLEEPING,   // stillness accumulating, not yet confirmed
    AWAKE_STATE_ASLEEP,
    AWAKE_STATE_OFF_WRIST,
} awake_mode_t;

typedef struct {
    awake_mode_t mode;
    bool         show_sleep;           // display toggle: show last sleep vs. current mode
    uint32_t     still_since_epoch;    // when continuous stillness began
    uint32_t     sleep_start_epoch;    // confirmed (backdated) sleep start
    uint32_t     wake_epoch;           // when current awake period began
    uint32_t     last_sleep_seconds;   // duration of last completed sleep period
    uint8_t      still_poll_count;     // consecutive still polls (toward sleep confirm)
    uint8_t      active_poll_count;    // consecutive active polls (toward wake confirm)
    uint32_t     first_active_epoch;   // epoch of first active poll in current wake run
    uint32_t     last_step_count;      // step count at last poll
    int16_t      wrist_temp_avg;       // rolling avg temp on wrist (tenths of °C)
    uint8_t      wrist_temp_samples;   // samples collected for avg (caps at 8)
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
