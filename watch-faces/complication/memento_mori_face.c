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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "memento_mori_face.h"
#include "watch.h"
#include "watch_utility.h"

/*
 * Unix timestamp for November 1, 1989 00:00:00 UTC (birth date).
 *   watch_utility_convert_to_unix_time(1989, 11, 1, 0, 0, 0, 0) = 625881600
 *
 * Unix timestamp for November 1, 2069 00:00:00 UTC (assumed death).
 *   watch_utility_convert_to_unix_time(2069, 11, 1, 0, 0, 0, 0) = 3150489600
 *
 * Total lifespan in seconds: 2524608000  (80 years * 365.25 days ≈ exact)
 *
 * Both values fit in uint32_t (max 4294967295). Intermediate percentage
 * arithmetic uses uint64_t to avoid overflow when multiplying by 1000.
 */
#define MEMENTO_MORI_BIRTH_EPOCH ((uint32_t)625881600UL)
#define MEMENTO_MORI_DEATH_EPOCH ((uint32_t)3150489600UL)
#define MEMENTO_MORI_LIFESPAN    ((uint32_t)(MEMENTO_MORI_DEATH_EPOCH - MEMENTO_MORI_BIRTH_EPOCH))

static void _memento_mori_update(memento_mori_state_t *state) {
    watch_date_time_t date_time = watch_rtc_get_date_time();
    uint32_t now = watch_utility_date_time_to_unix_time(date_time, 0);

    char bot[7];

    if (state->view == MEMENTO_MORI_VIEW_PERCENT) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "MORT ", "MO");

        if (now <= MEMENTO_MORI_BIRTH_EPOCH) {
            /* Not yet born — show 0.0 */
            watch_display_text(WATCH_POSITION_BOTTOM, "  0.0 ");
        } else if (now >= MEMENTO_MORI_DEATH_EPOCH) {
            /* Past assumed death — show 100.0 */
            watch_display_text(WATCH_POSITION_BOTTOM, "100.0 ");
        } else {
            /*
             * Compute percentage to one decimal place using integer math.
             * pct_tenths = (elapsed * 1000) / lifespan
             *   => whole part = pct_tenths / 10
             *   => fractional digit = pct_tenths % 10
             *
             * Use uint64_t for the multiply to prevent 32-bit overflow:
             *   max elapsed ~= 2524608000, * 1000 = ~2.5e12 — needs 64 bits.
             */
            uint32_t elapsed = now - MEMENTO_MORI_BIRTH_EPOCH;
            uint32_t pct_tenths = (uint32_t)(((uint64_t)elapsed * 1000ULL) / MEMENTO_MORI_LIFESPAN);
            uint32_t whole = pct_tenths / 10;
            uint32_t frac  = pct_tenths % 10;

            if (whole >= 100) {
                /* Clamp — shouldn't reach here given the guard above */
                watch_display_text(WATCH_POSITION_BOTTOM, "100.0 ");
            } else if (whole >= 10) {
                /* Two-digit whole part: " XX.Y " — 6 chars */
                snprintf(bot, sizeof(bot), " %2lu.%lu ", (unsigned long)whole, (unsigned long)frac);
                watch_display_text(WATCH_POSITION_BOTTOM, bot);
            } else {
                /* Single-digit whole part: "  X.Y " — 6 chars */
                snprintf(bot, sizeof(bot), "  %1lu.%lu ", (unsigned long)whole, (unsigned long)frac);
                watch_display_text(WATCH_POSITION_BOTTOM, bot);
            }
        }
    } else {
        /* Years remaining view */
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "YrS  ", "YR");

        if (now >= MEMENTO_MORI_DEATH_EPOCH) {
            watch_display_text(WATCH_POSITION_BOTTOM, "     0");
        } else {
            uint32_t secs_left  = MEMENTO_MORI_DEATH_EPOCH - now;
            /* Integer years: divide by seconds-per-year (365.25 days).
             * Use 365 * 86400 + 21600 = 31557600 (Julian year). */
            uint32_t years_left = secs_left / 31557600UL;
            snprintf(bot, sizeof(bot), "%6lu", (unsigned long)years_left);
            watch_display_text(WATCH_POSITION_BOTTOM, bot);
        }
    }
}

void memento_mori_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(memento_mori_state_t));
        memset(*context_ptr, 0, sizeof(memento_mori_state_t));
    }
}

void memento_mori_face_activate(void *context) {
    memento_mori_state_t *state = (memento_mori_state_t *)context;
    state->view = MEMENTO_MORI_VIEW_PERCENT;
    /* 1 Hz is sufficient; display only changes once per minute but EVENT_TICK
     * is cheap and keeps the face responsive to button presses. */
    movement_request_tick_frequency(1);
}

bool memento_mori_face_loop(movement_event_t event, void *context) {
    memento_mori_state_t *state = (memento_mori_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _memento_mori_update(state);
            break;

        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            /* Only the minute boundary changes the percentage, but refreshing
             * every tick costs nothing and avoids a stale display on wake. */
            _memento_mori_update(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            /* Toggle between percentage and years remaining. */
            if (state->view == MEMENTO_MORI_VIEW_PERCENT) {
                state->view = MEMENTO_MORI_VIEW_YEARS;
            } else {
                state->view = MEMENTO_MORI_VIEW_PERCENT;
            }
            _memento_mori_update(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;

        case EVENT_TIMEOUT:
            /* No timeout — stay on this face indefinitely. */
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void memento_mori_face_resign(void *context) {
    (void) context;
}
