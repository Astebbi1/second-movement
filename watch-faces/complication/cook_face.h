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

#pragma once

#include "movement.h"

/*
 * COOK FACE
 *
 * A quick-reference cooking temperature guide.
 *
 * Top row shows the food type. Bottom row alternates every 3 seconds between:
 *   NNN°H  — recommended oven/heat temperature (°F), degree dot + H in seconds position
 *   NNN°T  — target internal temperature (°F), degree dot + T in seconds position; shows "--°T" if not applicable
 *
 * ALARM button: next food item
 * LIGHT button: LED
 *
 * Items (in order):
 *   beef rare, beef med-rare, beef medium, beef well,
 *   lamb, pork, chicken, fish, veg
 */

typedef struct {
    uint8_t item_idx;   /* 0 – (COOK_NUM_ITEMS-1) */
    uint8_t tick;       /* toggles 0/1 each second for oven/internal alternation */
} cook_face_state_t;

void cook_face_setup(uint8_t watch_face_index, void **context_ptr);
void cook_face_activate(void *context);
bool cook_face_loop(movement_event_t event, void *context);
void cook_face_resign(void *context);

#define cook_face ((const watch_face_t){ \
    cook_face_setup, \
    cook_face_activate, \
    cook_face_loop, \
    cook_face_resign, \
    NULL, \
})
