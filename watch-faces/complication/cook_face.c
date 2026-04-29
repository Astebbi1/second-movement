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
#include "cook_face.h"

/* --- Lookup table --------------------------------------------------------
 * oven_f   : recommended oven temperature in °F
 * internal_f: target internal temperature in °F  (0 = not applicable)
 *
 * Internal temp sources: USDA guidelines + common culinary practice.
 * Beef rare/med-rare are below the USDA 145°F minimum for whole muscle —
 * these are standard restaurant targets for whole-muscle steaks.
 * ------------------------------------------------------------------------- */
typedef struct {
    const char top_custom[6];   /* 5 chars + NUL, for custom LCD top row  */
    const char top_classic[4];  /* 3 chars + NUL, fallback for classic LCD */
    uint16_t   oven_f;          /* oven temp °F                            */
    uint16_t   internal_f;      /* internal target °F  (0 = N/A)           */
    uint8_t    beef_rank;       /* 1–5 for beef doneness levels, 0 = not beef */
} cook_item_t;

static const cook_item_t ITEMS[] = {
    { "bF 1 ", "bF1",  450, 120, 1 },   /* beef rare        */
    { "bF 2 ", "bF2",  450, 135, 2 },   /* beef med-rare    */
    { "bF 3 ", "bF3",  375, 145, 3 },   /* beef medium      */
    { "bF 4 ", "bF4",  375, 155, 4 },   /* beef med-well    */
    { "bF 5 ", "bF5",  375, 160, 5 },   /* beef well-done   */
    { "LAMb ", "LAM",  425, 145, 0 },   /* lamb             */
    { "POrk ", "PRk",  400, 145, 0 },   /* pork             */
    { "CHkn ", "CHk",  400, 165, 0 },   /* chicken          */
    { "FH   ", "FH",   350, 145, 0 },   /* fish             */
    { "VEG  ", "VEG",  425,   0, 0 },   /* roasted veg      */
};

#define COOK_NUM_ITEMS ((uint8_t)(sizeof(ITEMS) / sizeof(ITEMS[0])))

static void _cook_display(cook_face_state_t *state) {
    const cook_item_t *item = &ITEMS[state->item_idx];

    /* Top row: food label.
     * Beef items use split positions so the rank digit renders on classic LCD
     * (TOP_LEFT="bF", TOP_RIGHT=" N"). Other items use the normal fallback. */
    if (item->beef_rank > 0) {
        char rank[3];
        watch_display_text(WATCH_POSITION_TOP_LEFT, "bF");
        snprintf(rank, sizeof(rank), " %u", item->beef_rank);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, rank);
    } else {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  "); // clear any lingering beef rank digit
        watch_display_text_with_fallback(WATCH_POSITION_TOP, item->top_custom, item->top_classic);
    }

    /* Bottom row: right-aligned 3-digit temp, space, mode letter (H=oven, T=target) */
    char bot[9];
    if (state->show_heat) {
        snprintf(bot, sizeof(bot), " %3u H", item->oven_f);
    } else {
        if (item->internal_f > 0) {
            snprintf(bot, sizeof(bot), " %3u T", item->internal_f);
        } else {
            snprintf(bot, sizeof(bot), "  -- T");
        }
    }
    watch_display_text(WATCH_POSITION_BOTTOM, bot);
}

void cook_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(cook_face_state_t));
        memset(*context_ptr, 0, sizeof(cook_face_state_t));
    }
}

void cook_face_activate(void *context) {
    cook_face_state_t *state = (cook_face_state_t *)context;
    state->show_heat = false; /* default: show target internal temp */
}

bool cook_face_loop(movement_event_t event, void *context) {
    cook_face_state_t *state = (cook_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _cook_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            state->item_idx = (state->item_idx + 1) % COOK_NUM_ITEMS;
            _cook_display(state);
            break;

        case EVENT_LIGHT_BUTTON_UP:
            state->show_heat = !state->show_heat;
            _cook_display(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            break; /* suppress LED; toggle happens on button up */

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            _cook_display(state);
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void cook_face_resign(void *context) {
    (void) context;
}
