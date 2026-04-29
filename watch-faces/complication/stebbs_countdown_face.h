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

#ifndef STEBBS_COUNTDOWN_FACE_H_
#define STEBBS_COUNTDOWN_FACE_H_

/*
 * STEBBS COUNTDOWN face
 * =====================
 * Preset-based repeating countdown timer.
 *
 * Display: "TIMER" at top. Bottom shows MM:SS + 2-letter tune abbreviation.
 *   e.g. "05:00GL" = 5-minute timer, Gambler tune.
 *
 * Presets (cycle with ALARM tap while idle):
 *   30s · 1m · 3m · 5m · 10m · 15m · 30m
 *
 * Tunes (cycle with LIGHT tap while idle):
 *   GL · PK · NK · JP · PU · XP · rG · WE · Cn · bP
 *
 * Controls:
 *   IDLE:
 *     ALARM tap       — cycle preset
 *     ALARM long      — start timer
 *     LIGHT tap       — cycle tune
 *     LIGHT long      — toggle repeat (LAP indicator = repeat on)
 *
 *   RUNNING:
 *     ALARM tap       — pause
 *     ALARM long      — reset to idle
 *     LIGHT tap       — illuminate LED
 *
 *   PAUSED:
 *     ALARM tap       — resume
 *     ALARM long      — reset to idle
 *     LIGHT tap       — illuminate LED
 *
 * On fire: plays selected tune. If repeat on, restarts immediately.
 */

#include "movement.h"

typedef enum {
    SCD_IDLE = 0,
    SCD_RUNNING,
    SCD_PAUSED,
} scd_mode_t;

typedef struct {
    uint8_t watch_face_index;
    uint8_t preset_idx;        // index into _scd_presets[]
    uint8_t tune_idx;          // index into _scd_tunes[]
    uint8_t fire_count;        // times timer has fired this run (0-31); shown in date position
    bool repeat;
    scd_mode_t mode;
    uint32_t target_ts;        // unix time when timer fires
    uint32_t now_ts;           // unix time at last tick
    uint32_t paused_remaining; // seconds remaining when paused
} scd_state_t;

void stebbs_countdown_face_setup(uint8_t watch_face_index, void **context_ptr);
void stebbs_countdown_face_activate(void *context);
bool stebbs_countdown_face_loop(movement_event_t event, void *context);
void stebbs_countdown_face_resign(void *context);

#define stebbs_countdown_face ((const watch_face_t){ \
    stebbs_countdown_face_setup, \
    stebbs_countdown_face_activate, \
    stebbs_countdown_face_loop, \
    stebbs_countdown_face_resign, \
    NULL, \
})

#endif // STEBBS_COUNTDOWN_FACE_H_
