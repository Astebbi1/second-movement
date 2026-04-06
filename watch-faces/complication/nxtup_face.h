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

#ifndef NXTUP_FACE_H_
#define NXTUP_FACE_H_

/*
 * NXTUP face — next upcoming yearly events
 * =========================================
 * Shows the next upcoming event from your personal event list
 * (holidays, birthdays, anniversaries, etc.) defined in
 * movement_custom_events.h. Events are sorted nearest-first at startup.
 *
 * Custom LCD display:
 *   Top 5 chars: "NX" + 3-digit days remaining (e.g. "NX 42" or "NX365")
 *   Bottom 6 chars: event name, scrolling if longer than 6 chars
 *   Days = 0: top shows "NX  0", bottom shows "TODAY "
 *
 * Classic LCD display:
 *   Top: "NX" (2 chars)
 *   Bottom: scrolling event name / alternates to show day count
 *
 * Bottom 6 chars scroll: "name DOW Mon day" (e.g. "AndrEW bdAY SAt noV 1"),
 * repeating with a 2-space gap. Days=0 still shows the name scroll (0 in top is enough).
 *
 * ALARM button: advance to next event (wraps around)
 * LIGHT button: go back to previous event
 * LIGHT long-press: return to the nearest upcoming event (index 0)
 */

#include "movement.h"

// Maximum number of events supported (increase if needed)
#define NXTUP_MAX_EVENTS 64

typedef struct {
    uint8_t  current_idx;                   // position in sorted list currently shown
    uint8_t  event_count;                   // total events loaded (≤ NXTUP_MAX_EVENTS)
    uint8_t  sorted_order[NXTUP_MAX_EVENTS];// sorted_order[i] = index into yearly_events[]
    uint16_t days_until[NXTUP_MAX_EVENTS];  // days until each event (parallel, unsorted)
    char     scroll_buf[56];                // "name DOW Mon day  " — built per event
    uint8_t  scroll_buf_len;                // strlen(scroll_buf)
    uint8_t  scroll_pos;                    // current scroll offset into scroll_buf
} nxtup_state_t;

void nxtup_face_setup(uint8_t watch_face_index, void **context_ptr);
void nxtup_face_activate(void *context);
bool nxtup_face_loop(movement_event_t event, void *context);
void nxtup_face_resign(void *context);

#define nxtup_face ((const watch_face_t){ \
    nxtup_face_setup, \
    nxtup_face_activate, \
    nxtup_face_loop, \
    nxtup_face_resign, \
    NULL, \
})

#endif // NXTUP_FACE_H_
