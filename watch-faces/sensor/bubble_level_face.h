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

#ifndef BUBBLE_LEVEL_FACE_H_
#define BUBBLE_LEVEL_FACE_H_

/*
 * BUBBLE LEVEL FACE
 *
 * Turns the watch into a 1D bubble level using the LIS2DW accelerometer.
 *
 * Top row displays "Level".
 * Bottom row shows a bubble sliding left/right as the watch tilts:
 *   [0-----]  far left
 *   [---0--]  centered (level)
 *   [-----0]  far right
 *
 * When level (bubble at center):
 *   - Green LED stays on
 *   - SIGNAL indicator flashes
 *   - Short repeating beep once per second
 *   - Bubble character flashes between '0' and '8'
 *
 * Controls:
 *   ALARM — toggle between X-axis and Y-axis (for different holding orientations)
 *   MODE  — move to next face
 */

#include "movement.h"

typedef struct {
    int16_t smoothed_x;    // low-pass filtered X reading
    int16_t smoothed_y;    // low-pass filtered Y reading
    uint8_t tick_count;    // wrapping tick counter (for flash/beep timing)
    bool use_y_axis;       // false=X axis (default), true=Y axis
    bool led_enabled;      // tracks whether we called watch_enable_leds
} bubble_level_state_t;

void bubble_level_face_setup(uint8_t watch_face_index, void **context_ptr);
void bubble_level_face_activate(void *context);
bool bubble_level_face_loop(movement_event_t event, void *context);
void bubble_level_face_resign(void *context);

#define bubble_level_face ((const watch_face_t){ \
    bubble_level_face_setup, \
    bubble_level_face_activate, \
    bubble_level_face_loop, \
    bubble_level_face_resign, \
    NULL, \
})

#endif // BUBBLE_LEVEL_FACE_H_
