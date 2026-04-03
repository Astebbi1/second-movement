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
#include "dice_face.h"
#include "delay.h"

/* Duration unit = TC0 ticks at ~64 Hz (~15 ms each).
 *
 * Roll sequence layout:
 *   5 descending notes  × 7 ticks each (~105 ms) = ~525 ms
 *   1 REST              × 15 ticks       (~225 ms) = ~225 ms
 *   1 result note       × 20 ticks       (~300 ms) = ~300 ms
 *   total                                          = ~1050 ms
 *
 * The blocking animation below is timed to match, so the result note starts
 * just as the number appears on screen.
 */

/* File-scope so the async sequence player can access it after we return. */
static int8_t _dice_seq[16];

/* Result notes — C-major arpeggio ascending over two octaves.
 * Index 0 = die face 1 (lowest), index 5 = die face 6 (highest). */
static const int8_t _result_notes[6] = {
    BUZZER_NOTE_C4,   /* 1 */
    BUZZER_NOTE_E4,   /* 2 */
    BUZZER_NOTE_G4,   /* 3 */
    BUZZER_NOTE_C5,   /* 4 */
    BUZZER_NOTE_E5,   /* 5 */
    BUZZER_NOTE_G5,   /* 6 */
};

/* Descending rattle notes played while the die rolls. */
static const int8_t _roll_notes[5] = {
    BUZZER_NOTE_G5,
    BUZZER_NOTE_E5,
    BUZZER_NOTE_C5,
    BUZZER_NOTE_G4,
    BUZZER_NOTE_E4,
};

static void _build_and_play_seq(uint8_t result) {
    int pos = 0;
    for (int i = 0; i < 5; i++) {
        _dice_seq[pos++] = _roll_notes[i];
        _dice_seq[pos++] = 7;   /* ~105 ms per descending note */
    }
    _dice_seq[pos++] = BUZZER_NOTE_REST;
    _dice_seq[pos++] = 15;      /* ~225 ms pause */
    _dice_seq[pos++] = _result_notes[result - 1];
    _dice_seq[pos++] = 20;      /* ~300 ms result note */
    _dice_seq[pos]   = 0;       /* end marker */
    watch_buzzer_play_sequence(_dice_seq, NULL);
}

static uint32_t _get_random(uint32_t max) {
#if __EMSCRIPTEN__
    return rand() % max;
#else
    return arc4random_uniform(max);
#endif
}

static void _show_result(uint8_t result) {
    char buf[7];
    watch_display_text(WATCH_POSITION_TOP, "di");
    snprintf(buf, sizeof(buf), "%6u", result);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void _do_roll(dice_face_state_t *state) {
    state->result = (uint8_t)(_get_random(6) + 1);

    /* Start the full sound sequence asynchronously. */
    _build_and_play_seq(state->result);

    /* Blocking display animation synchronized to the sound.
     * 5 notes × ~105 ms = ~525 ms of rolling frames. */
    static const char *frames[5] = {
        "- -- -", "------", " ---- ", "------", "- -- -",
    };
    watch_display_text(WATCH_POSITION_TOP, "di");
    for (int i = 0; i < 5; i++) {
        watch_display_text(WATCH_POSITION_BOTTOM, frames[i]);
        delay_ms(105);
    }

    /* Pause while REST plays (~225 ms). */
    delay_ms(225);

    /* Result note is now playing — reveal the number. */
    _show_result(state->result);

    /* Wait for result note to finish (~300 ms). */
    delay_ms(320);
}

void dice_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(dice_face_state_t));
        memset(*context_ptr, 0, sizeof(dice_face_state_t));
    }
}

void dice_face_activate(void *context) {
    dice_face_state_t *state = (dice_face_state_t *)context;
    watch_display_text(WATCH_POSITION_TOP, "di");
    if (state->result == 0) {
        watch_display_text(WATCH_POSITION_BOTTOM, " roll ");
    } else {
        _show_result(state->result);
    }
}

bool dice_face_loop(movement_event_t event, void *context) {
    dice_face_state_t *state = (dice_face_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            /* Handled in activate. */
            break;

        case EVENT_ALARM_BUTTON_UP:
        case EVENT_LIGHT_BUTTON_UP:
            _do_roll(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            /* Suppress default LED behavior so LIGHT can also roll. */
            break;

        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;

        case EVENT_LOW_ENERGY_UPDATE:
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void dice_face_resign(void *context) {
    (void) context;
    watch_buzzer_abort_sequence();
}
