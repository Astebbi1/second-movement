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

#ifndef ACTIVE_COUNTDOWN_FACE_H_
#define ACTIVE_COUNTDOWN_FACE_H_

/*
 * ACTIVE COUNTDOWN face
 *
 * Displays the estimated minutes (or days) of life remaining, actively
 * counting down every second. The lifespan assumption is birth on
 * November 1, 1989 and death on November 1, 2069 (80 years).
 *
 * The bottom row shows the minutes remaining, right-aligned in 6 digits
 * (~22 million minutes from 2026). The number visibly decrements every
 * 60 seconds — the visceral point of this face.
 *
 * Press ALARM to toggle between minutes remaining and days remaining.
 * Press MODE to leave the face.
 */

#include "movement.h"

typedef enum {
    ACTIVE_COUNTDOWN_VIEW_DAYS,
    ACTIVE_COUNTDOWN_VIEW_WEEKS,
} active_countdown_view_t;

typedef struct {
    active_countdown_view_t view;
} active_countdown_state_t;

void active_countdown_face_setup(uint8_t watch_face_index, void **context_ptr);
void active_countdown_face_activate(void *context);
bool active_countdown_face_loop(movement_event_t event, void *context);
void active_countdown_face_resign(void *context);

#define active_countdown_face ((const watch_face_t){ \
    active_countdown_face_setup, \
    active_countdown_face_activate, \
    active_countdown_face_loop, \
    active_countdown_face_resign, \
    NULL, \
})

#endif // ACTIVE_COUNTDOWN_FACE_H_
