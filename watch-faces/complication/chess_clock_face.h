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

#ifndef CHESS_CLOCK_FACE_H_
#define CHESS_CLOCK_FACE_H_

/*
 * CHESS CLOCK face
 *
 * A two-player chess clock where each player's elapsed time counts up.
 * Only one player's clock runs at a time.
 *
 * Usage:
 *   - Press ALARM to start Player 1's clock (from the initial paused state).
 *   - Press ALARM to pass the clock to the other player.
 *   - Hold ALARM to reset both timers to zero and pause.
 *   - Press LIGHT (short) to peek at the other player's time.
 *     The display returns to the active player on the next tick.
 *   - Hold LIGHT to illuminate the LED.
 *
 * Display:
 *   - Top:    "P 1  " / "P 2  " (active player), or "CHESS" when paused at start.
 *   - Bottom: Active player's elapsed time as MM:SS.
 *             While peeking, shows the other player's time instead.
 */

#include "movement.h"

typedef struct {
    uint32_t time_cs[2];   // elapsed centiseconds for each player
    uint8_t active_player; // 0 or 1
    bool running;
    bool peeking;          // true while showing the other player's time
} chess_clock_state_t;

void chess_clock_face_setup(uint8_t watch_face_index, void ** context_ptr);
void chess_clock_face_activate(void *context);
bool chess_clock_face_loop(movement_event_t event, void *context);
void chess_clock_face_resign(void *context);

#define chess_clock_face ((const watch_face_t){ \
    chess_clock_face_setup, \
    chess_clock_face_activate, \
    chess_clock_face_loop, \
    chess_clock_face_resign, \
    NULL, \
})

#endif // CHESS_CLOCK_FACE_H_
