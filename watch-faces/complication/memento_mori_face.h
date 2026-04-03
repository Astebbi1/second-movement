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

#ifndef MEMENTO_MORI_FACE_H_
#define MEMENTO_MORI_FACE_H_

/*
 * MEMENTO MORI face
 *
 * Shows the percentage of an assumed 80-year lifespan that has elapsed,
 * to one decimal place. Birth: November 1, 1989. Death: November 1, 2069.
 *
 *   Percentage = (now - birth_epoch) / (death_epoch - birth_epoch) * 100
 *
 * The top row reads "MORT " (fallback "MO") and the bottom row shows the
 * percentage centered, e.g. " 45.7 ". Updates once per minute.
 *
 * Press ALARM to toggle to a "years remaining" view that shows the whole
 * number of years left (e.g. "   43  "). Press ALARM again to return.
 * Press MODE to leave the face. No automatic timeout.
 */

#include "movement.h"

typedef enum {
    MEMENTO_MORI_VIEW_PERCENT,
    MEMENTO_MORI_VIEW_YEARS,
} memento_mori_view_t;

typedef struct {
    memento_mori_view_t view;
} memento_mori_state_t;

void memento_mori_face_setup(uint8_t watch_face_index, void **context_ptr);
void memento_mori_face_activate(void *context);
bool memento_mori_face_loop(movement_event_t event, void *context);
void memento_mori_face_resign(void *context);

#define memento_mori_face ((const watch_face_t){ \
    memento_mori_face_setup, \
    memento_mori_face_activate, \
    memento_mori_face_loop, \
    memento_mori_face_resign, \
    NULL, \
})

#endif // MEMENTO_MORI_FACE_H_
