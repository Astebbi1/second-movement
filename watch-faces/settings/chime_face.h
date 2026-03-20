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

#ifndef CHIME_FACE_H_
#define CHIME_FACE_H_

/*
 * CHIME face — runtime hourly signal tune picker
 * ===============================================
 * Lets you choose which tune plays as the hourly chime.
 * The selection takes effect immediately and persists until power cycle.
 *
 * Display: top-left "CH", top-right tune number (01-17),
 *          bottom: scrolling 6-char tune name.
 *
 * ALARM button: cycle to next tune (selection takes effect immediately).
 * ALARM long-press: preview/play the current tune.
 * LIGHT button: suppress LED (don't illuminate).
 * MODE: exit to next face.
 */

#include "movement.h"

typedef struct {
    uint8_t current_tune;
    uint8_t scroll_pos;
    bool playing;
} chime_state_t;

void chime_face_setup(uint8_t watch_face_index, void ** context_ptr);
void chime_face_activate(void *context);
bool chime_face_loop(movement_event_t event, void *context);
void chime_face_resign(void *context);

#define chime_face ((const watch_face_t){ \
    chime_face_setup, \
    chime_face_activate, \
    chime_face_loop, \
    chime_face_resign, \
    NULL, \
})

#endif // CHIME_FACE_H_
