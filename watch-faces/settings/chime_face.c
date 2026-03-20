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
#include <stdio.h>  // snprintf
#include "chime_face.h"
#include "movement_custom_signal_tunes.h"

static chime_state_t *_active_chime_state = NULL;

static void _chime_finished_callback(void) {
    if (_active_chime_state != NULL) {
        _active_chime_state->playing = false;
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
}

static void _chime_update_display(chime_state_t *state) {
    const signal_tune_t *tune = &signal_tunes[state->current_tune];

    // Top: "CH" label + 2-digit tune number (1-based)
    char top_label[7];
    snprintf(top_label, sizeof(top_label), "CH %2d", state->current_tune + 1);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, top_label, "CH");

    // Bottom: 6-char scrolling window of tune name
    char name_window[7];
    uint8_t name_len = strlen(tune->name);
    for (int i = 0; i < 6; i++) {
        name_window[i] = tune->name[(state->scroll_pos + i) % name_len];
    }
    name_window[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, name_window);

    if (state->playing) {
        watch_set_indicator(WATCH_INDICATOR_BELL);
    } else {
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
    watch_clear_colon();
}

void chime_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(chime_state_t));
        chime_state_t *state = (chime_state_t *)*context_ptr;
        memset(state, 0, sizeof(chime_state_t));
        // Sync to whatever tune is currently selected in movement_state
        state->current_tune = movement_get_signal_tune_index();
    }
}

void chime_face_activate(void *context) {
    chime_state_t *state = (chime_state_t *)context;
    _active_chime_state = state;
    // Sync in case it was changed elsewhere
    state->current_tune = movement_get_signal_tune_index();
    state->scroll_pos = 0;
    movement_request_tick_frequency(4);
}

bool chime_face_loop(movement_event_t event, void *context) {
    chime_state_t *state = (chime_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _chime_update_display(state);
            break;

        case EVENT_TICK:
            {
                const signal_tune_t *tune = &signal_tunes[state->current_tune];
                uint8_t name_len = strlen(tune->name);
                state->scroll_pos = (state->scroll_pos + 1) % name_len;
            }
            _chime_update_display(state);
            break;

        case EVENT_ALARM_BUTTON_UP:
            // Stop if playing, advance to next tune
            if (state->playing) {
                watch_buzzer_abort_sequence();
                state->playing = false;
            }
            state->current_tune = (state->current_tune + 1) % SIGNAL_NUM_TUNES;
            state->scroll_pos = 0;
            // Apply immediately — this IS the selected tune
            movement_set_signal_tune_index(state->current_tune);
            _chime_update_display(state);
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Preview: play the current tune
            if (state->playing) {
                watch_buzzer_abort_sequence();
                state->playing = false;
                watch_clear_indicator(WATCH_INDICATOR_BELL);
            } else {
                state->playing = true;
                watch_buzzer_play_sequence((int8_t *)signal_tunes[state->current_tune].sequence,
                                          _chime_finished_callback);
                watch_set_indicator(WATCH_INDICATOR_BELL);
            }
            _chime_update_display(state);
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            // Suppress LED
            break;

        case EVENT_TIMEOUT:
            if (!state->playing) {
                movement_move_to_face(0);
            }
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void chime_face_resign(void *context) {
    chime_state_t *state = (chime_state_t *)context;
    if (state->playing) {
        watch_buzzer_abort_sequence();
        state->playing = false;
    }
    watch_clear_indicator(WATCH_INDICATOR_BELL);
    _active_chime_state = NULL;
    movement_request_tick_frequency(1);
}
