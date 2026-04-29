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
 * AWAKE face — wear & sleep tracker
 * ===================================
 * Five pages, cycled with ALARM (forward) and LIGHT (backward):
 *
 *   AW — time since you woke up (or current sleep duration if asleep)
 *   SL — last completed sleep duration
 *   Wr — current continuous wear session (temperature-based, reliable)
 *   oF — last off-wrist duration (temperature-based, reliable)
 *   LG — longest continuous wear session ever (resets on power cycle)
 *
 * Sleep detection (time-gated):
 *   Polls every 15 min. Within the night window (10 pm – 8 am) or nap window
 *   (1 pm – 6 pm), sustained stillness (no step-count increase) confirms sleep:
 *     Night: 3 consecutive still polls = 45 min
 *     Nap:   5 consecutive still polls = 75 min
 *   Sleep start is backdated to the first still poll. Wake is confirmed on the
 *   first poll showing steps, or auto-closed when the window expires (8 am / 6 pm).
 *   Outside both windows, stillness is ignored — sitting still at 9 pm won't
 *   register as sleep.
 *
 * Off-wrist detection:
 *   Temperature drops ≥ 6 °C below rolling wrist baseline → off-wrist state.
 *   No display takeover — the face just tracks duration silently.
 *   The Wr page freezes at the last on-wrist duration; the oF page counts up.
 *
 * ALARM long-press (any page): manual "I just woke up" — corrects drift
 * ALARM long-press on LG page:  full hard reset — clears all stats
 * LIGHT long-press: illuminate LED
 */

#include "movement.h"

#define AWAKE_POLL_MINUTES          15    // background poll interval (minutes)
#define AWAKE_NIGHT_SLEEP_POLLS      3    // 3 × 15 min = 45 min → confirm night sleep
#define AWAKE_NAP_POLLS              5    // 5 × 15 min = 75 min → confirm nap
#define AWAKE_SLEEP_START_HOUR      22    // night window opens  (10 pm local)
#define AWAKE_SLEEP_END_HOUR         8    // night window closes (8 am local) → auto-wake
#define AWAKE_NAP_START_HOUR        13    // nap window opens    (1 pm local)
#define AWAKE_NAP_END_HOUR          18    // nap window closes   (6 pm local) → auto-wake
#define AWAKE_OFFWRIST_TEMP_TENTHS  60    // 6.0 °C drop from wrist avg → off-wrist
#define AWAKE_MIN_SLEEP_SEC     (30*60)   // ignore sleep blocks shorter than 30 min

typedef enum {
    AWAKE_STATE_AWAKE = 0,
    AWAKE_STATE_ASLEEP,
    AWAKE_STATE_OFF_WRIST,
} awake_mode_t;

typedef struct {
    awake_mode_t mode;
    uint8_t      page;                   // 0=AW 1=SL 2=Wr 3=oF 4=LG

    // Sleep / wake tracking
    uint32_t     wake_epoch;             // start of current awake period
    uint32_t     still_since_epoch;      // when in-window stillness began
    uint32_t     sleep_start_epoch;      // confirmed (backdated) sleep start
    uint32_t     last_sleep_seconds;     // last completed sleep duration
    uint8_t      still_poll_count;       // consecutive still polls in sleep window

    // Wear-session tracking (temperature-based, reliable)
    uint32_t     wear_session_start;     // epoch when current wear session began
    uint32_t     off_wrist_start_epoch;  // epoch when current off-wrist period began
    uint32_t     last_off_wrist_seconds; // duration of last completed off-wrist period
    uint32_t     longest_wear_seconds;   // all-time longest continuous wear (resets on power cycle)

    // Wrist temperature (for off-wrist detection)
    int16_t      wrist_temp_avg;         // rolling wrist temperature (tenths of °C)
    uint8_t      wrist_temp_samples;     // samples used for avg (caps at 8)

    // Steps
    uint32_t     last_step_count;
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
