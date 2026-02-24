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

#ifndef STEBBS_FACE_H_
#define STEBBS_FACE_H_

/*
 * STEBBS WATCH FACE
 *
 * Simultaneously scrolls "StEbbS HH:MM" across the bottom 6 positions
 * while a fireworks animation loops in the top 4 positions.
 *
 * Usage:
 * - LIGHT button: Illuminate LED
 * - MODE button: Move to next watch face
 * - ALARM button: Pause/resume animations
 */

#include "movement.h"

#define STEBBS_SCROLL_TEXT_LEN 40

typedef struct {
    uint8_t scroll_pos;           // Current character position in scroll text
    uint8_t fireworks_frame;      // Current fireworks animation frame
    uint8_t tick_count;           // Raw tick counter (for scroll rate divisor)
    bool paused;                  // Animation paused
    char scroll_text[STEBBS_SCROLL_TEXT_LEN]; // Pre-built scroll text with current time
} stebbs_state_t;

void stebbs_face_setup(uint8_t watch_face_index, void ** context_ptr);
void stebbs_face_activate(void *context);
bool stebbs_face_loop(movement_event_t event, void *context);
void stebbs_face_resign(void *context);

#define stebbs_face ((const watch_face_t){ \
    stebbs_face_setup, \
    stebbs_face_activate, \
    stebbs_face_loop, \
    stebbs_face_resign, \
    NULL, \
})

#endif // STEBBS_FACE_H_
