/*
 * MIT License
 *
 * Copyright (c) 2023 <#author_name#>
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

#ifndef WYOSCAN_FACE_H_
#define WYOSCAN_FACE_H_

/*
 * WYOSCAN .5 hz watchface
 *
 * Recreation of the Wyoscan watch produced by Halmos / designed by Dexter Sinister.
 * Seven segments of each digit are lit one by one left-to-right across the display.
 * One full cycle takes 2 seconds (64 frames at 32 fps).
 *
 * Video:  https://user-images.githubusercontent.com/1795778/252550124-e07f0ed1-e328-4337-a654-fa1ee65d883f.mp4
 * Origin: https://artmetropole.com/shop/11460
 * Demo:   https://www.o-r-g.com/apps/wyoscan
 *
 * 8 frames per digit x 6 digits + 16 trailing clearance frames = 64 total.
 * No controls; animates continuously while the face is active.
 */

#include "movement.h"

#define MAX_ILLUMINATED_SEGMENTS 16

typedef struct {
    uint8_t  animation;
    bool     animate;
    uint32_t start;
    uint32_t end;
    uint32_t total_frames;
    bool     colon;
    uint8_t  position;
    uint8_t  segment;
    char    *segments;
    uint8_t  x;
    uint8_t  y;
    uint32_t time_digits[6];
    uint32_t illuminated_segments[MAX_ILLUMINATED_SEGMENTS][2];
} wyoscan_state_t;

void wyoscan_face_setup(uint8_t watch_face_index, void **context_ptr);
void wyoscan_face_activate(void *context);
bool wyoscan_face_loop(movement_event_t event, void *context);
void wyoscan_face_resign(void *context);

#define wyoscan_face ((const watch_face_t){ \
    wyoscan_face_setup,    \
    wyoscan_face_activate, \
    wyoscan_face_loop,     \
    wyoscan_face_resign,   \
    NULL,                  \
})

#endif // WYOSCAN_FACE_H_
