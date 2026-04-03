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
#include "active_countdown_face.h"
#include "watch.h"
#include "watch_utility.h"

/*
 * Unix timestamp for November 1, 2069 00:00:00 UTC — the assumed end of an
 * 80-year lifespan starting November 1, 1989.
 *
 * Computed as: watch_utility_convert_to_unix_time(2069, 11, 1, 0, 0, 0, 0)
 * = 3150489600
 *
 * Note: this value exceeds 2^31 but fits in uint32_t (max ~4.29 billion).
 * The watch_utility functions use uint32_t throughout, so arithmetic is
 * performed in uint32_t to avoid signed-overflow. We cast to int64_t only
 * when computing the difference to allow for a negative result check.
 */
#define ACTIVE_COUNTDOWN_DEATH_EPOCH ((uint32_t)3150489600UL)

static void _active_countdown_update(active_countdown_state_t *state) {
    watch_date_time_t date_time = watch_rtc_get_date_time();
    uint32_t now = watch_utility_date_time_to_unix_time(date_time, 0);

    char bot[7];

    uint32_t secs_left = (now >= ACTIVE_COUNTDOWN_DEATH_EPOCH)
                         ? 0 : (ACTIVE_COUNTDOWN_DEATH_EPOCH - now);

    if (state->view == ACTIVE_COUNTDOWN_VIEW_DAYS) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "dSLFT", "dy");
        snprintf(bot, sizeof(bot), "%6lu", (unsigned long)(secs_left / 86400));
    } else {
        /* Weeks remaining — ~15,800 at time of writing */
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "WkSLF", "Wk");
        snprintf(bot, sizeof(bot), "%6lu", (unsigned long)(secs_left / 604800));
    }
    watch_display_text(WATCH_POSITION_BOTTOM, bot);
}

void active_countdown_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(active_countdown_state_t));
        memset(*context_ptr, 0, sizeof(active_countdown_state_t));
    }
}

void active_countdown_face_activate(void *context) {
    active_countdown_state_t *state = (active_countdown_state_t *)context;
    state->view = ACTIVE_COUNTDOWN_VIEW_DAYS;
    /* 1 Hz so the count decrements in real time. */
    movement_request_tick_frequency(1);
}

bool active_countdown_face_loop(movement_event_t event, void *context) {
    active_countdown_state_t *state = (active_countdown_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _active_countdown_update(state);
            break;

        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            _active_countdown_update(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            /* Toggle between days and weeks remaining. */
            state->view = (state->view == ACTIVE_COUNTDOWN_VIEW_DAYS)
                          ? ACTIVE_COUNTDOWN_VIEW_WEEKS
                          : ACTIVE_COUNTDOWN_VIEW_DAYS;
            _active_countdown_update(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void active_countdown_face_resign(void *context) {
    (void) context;
}
