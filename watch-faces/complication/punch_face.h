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

#ifndef PUNCH_FACE_H_
#define PUNCH_FACE_H_

/*
 * PUNCH face
 *
 * Wear the watch on your wrist, long-press ALARM to start.
 * A Mario Kart-style countdown plays: doot doot DEEET.
 * Punch a pillow on the DEEET — the accelerometer measures the impact.
 * Score 0–1000. A glitchy animation settles into your score,
 * followed by ascending chimes (the higher the score, the more notes).
 *
 * Sensor: LIS2DW accelerometer (Sensor Watch Pro). Without a sensor
 * (emulator / older hardware) a fun random score is used instead.
 *
 * ALARM button: cycle forward through last 5 scores.
 * LIGHT button: cycle backward.
 * ALARM long-press: start a new measurement from any state.
 * LIGHT long-press: illuminate LED.
 *
 * Tuning: PUNCH_SCALE_FACTOR controls what peak impact = score 1000.
 * At default FS=±2g LP mode: 1g ≈ 2048 LSB. A 3g punch on a single
 * axis gives impact ≈ 37,748,736. Adjust if scores feel too high/low.
 */

#include "movement.h"

#define PUNCH_HISTORY_SIZE    5

// Score 1000 maps to this raw impact magnitude² delta.
// Tune up to make scoring harder, down to make it easier.
// At FS=±2g LP mode: 1g ≈ 2048 LSB. A 6g punch on a single axis ≈ 150,994,944.
#define PUNCH_SCALE_FACTOR    150994944ULL

// State timing (tick counts per state)
#define PUNCH_COUNTDOWN_TICKS 5   // at 1 Hz  = 5 full seconds (5 / 4 / 3 / 2 / 1)
#define PUNCH_MEASURE_TICKS   24  // at 16 Hz = 1.5 second punch window (one-shot exits early)
#define PUNCH_ANIM_TICKS      16  // at 8 Hz  = 2 second animation

typedef enum {
    PUNCH_MODE_IDLE,
    PUNCH_MODE_COUNTDOWN,
    PUNCH_MODE_READY,
    PUNCH_MODE_ANIMATING,
    PUNCH_MODE_SCORE,
} punch_mode_t;

typedef struct {
    punch_mode_t mode;
    uint8_t  tick_count;
    uint16_t score;
    uint16_t history[PUNCH_HISTORY_SIZE];  // sorted descending (best first)
    uint8_t  history_count;
    uint8_t  view_idx;        // 0 = current round, 1..N = rank in history
    int64_t  baseline_mag2;
    int64_t  peak_mag2;
    bool     impact_detected;  // true once a significant spike is seen
    uint8_t  post_peak_ticks;  // ticks of decay after impact; triggers early exit at 4
    bool     no_sensor;
    uint16_t rng;              // LFSR state for animation noise
} punch_state_t;

void punch_face_setup(uint8_t watch_face_index, void **context_ptr);
void punch_face_activate(void *context);
bool punch_face_loop(movement_event_t event, void *context);
void punch_face_resign(void *context);

#define punch_face ((const watch_face_t){ \
    punch_face_setup, \
    punch_face_activate, \
    punch_face_loop, \
    punch_face_resign, \
    NULL, \
})

#endif // PUNCH_FACE_H_
