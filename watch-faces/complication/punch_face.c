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
#include "punch_face.h"

// ─── Sound sequences ──────────────────────────────────────────────────────────
// Duration units ≈ 1/64 second (≈15.6 ms each).

// Mario Kart-style countdown: 4 low doots (G4) then DEEET (E5, higher).
// ~1 second per beat: 20 on + 44 rest = 64 units ≈ 1 s; final note holds.
static const int8_t _punch_countdown_seq[] = {
    BUZZER_NOTE_G4, 20, BUZZER_NOTE_REST, 44,
    BUZZER_NOTE_G4, 20, BUZZER_NOTE_REST, 44,
    BUZZER_NOTE_G4, 20, BUZZER_NOTE_REST, 44,
    BUZZER_NOTE_G4, 20, BUZZER_NOTE_REST, 44,
    BUZZER_NOTE_E5, 40,
    0
};

// Glitchy animation sound: rapid staccato at erratic pitches
static const int8_t _punch_glitch_seq[] = {
    BUZZER_NOTE_C6,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A3,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G5,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D3,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B4,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F3,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7,  2, BUZZER_NOTE_REST, 1,
    0
};

// Score chimes — ascending melodies, more notes = higher score
// Tier 0: 0–99 (sad descending)
static const int8_t _punch_chime_0[] = {
    BUZZER_NOTE_B4, 8, BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_G4, 8, BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_E4, 18,
    0
};
// Tier 1: 100–299 (one rise)
static const int8_t _punch_chime_1[] = {
    BUZZER_NOTE_C5, 8, BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_E5, 14,
    0
};
// Tier 2: 300–499 (two notes)
static const int8_t _punch_chime_2[] = {
    BUZZER_NOTE_C5, 6, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E5, 6, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G5, 14,
    0
};
// Tier 3: 500–699 (three notes)
static const int8_t _punch_chime_3[] = {
    BUZZER_NOTE_C5, 5, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E5, 5, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G5, 5, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C6, 14,
    0
};
// Tier 4: 700–899 (four notes)
static const int8_t _punch_chime_4[] = {
    BUZZER_NOTE_C5, 4, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E5, 4, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G5, 4, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C6, 4, BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E6, 14,
    0
};
// Tier 5: 900–999 (five notes)
static const int8_t _punch_chime_5[] = {
    BUZZER_NOTE_C5, 4, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E5, 4, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G5, 4, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C6, 4, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 4, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 16,
    0
};
// Tier 6: 1000 — perfect! Victory arpeggio
static const int8_t _punch_chime_perfect[] = {
    BUZZER_NOTE_C5, 3, BUZZER_NOTE_E5, 3, BUZZER_NOTE_G5, 3,
    BUZZER_NOTE_C6, 3, BUZZER_NOTE_E6, 3, BUZZER_NOTE_G6, 3,
    BUZZER_NOTE_C7, 22,
    0
};

static const int8_t *_punch_chimes[] = {
    _punch_chime_0, _punch_chime_1, _punch_chime_2,
    _punch_chime_3, _punch_chime_4, _punch_chime_5,
    _punch_chime_perfect
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

static uint8_t _punch_chime_index(uint16_t score) {
    if (score >= 1000) return 6;
    if (score >= 900)  return 5;
    if (score >= 700)  return 4;
    if (score >= 500)  return 3;
    if (score >= 300)  return 2;
    if (score >= 100)  return 1;
    return 0;
}

// Galois LFSR — fast 16-bit pseudo-random for animation noise.
static uint16_t _punch_rng(punch_state_t *state) {
    state->rng ^= state->rng >> 1;
    state->rng ^= state->rng << 11;
    state->rng ^= state->rng >> 7;
    return state->rng;
}

// Push a new score into the circular history, shifting old entries down.
static void _punch_record_score(punch_state_t *state, uint16_t score) {
    uint8_t n = (state->history_count < PUNCH_HISTORY_SIZE)
                ? state->history_count : PUNCH_HISTORY_SIZE - 1;
    for (int8_t i = (int8_t)n; i > 0; i--)
        state->history[i] = state->history[i - 1];
    state->history[0] = score;
    if (state->history_count < PUNCH_HISTORY_SIZE) state->history_count++;
}

// ─── Display ─────────────────────────────────────────────────────────────────

static void _punch_update_display(punch_state_t *state) {
    char buf[7];

    switch (state->mode) {
        case PUNCH_MODE_IDLE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
            watch_display_text(WATCH_POSITION_BOTTOM, "rEAdY ");
            watch_clear_colon();
            break;

        case PUNCH_MODE_COUNTDOWN: {
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
            uint8_t n = (uint8_t)(5 - state->tick_count);
            if (n == 0) n = 1;
            snprintf(buf, sizeof(buf), "  %u   ", n);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            watch_clear_colon();
            break;
        }

        case PUNCH_MODE_READY:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
            watch_display_text(WATCH_POSITION_BOTTOM, " HIt! ");
            watch_clear_colon();
            break;

        case PUNCH_MODE_ANIMATING: {
            uint8_t t = state->tick_count;
            uint16_t rnd = _punch_rng(state);

            if (t < 8) {
                // Glitch top row too in early frames (alternate between face label and garbage)
                if (t % 2 == 0)
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                else
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "88888", "88");

                // Bottom: fully random 4-digit number
                snprintf(buf, sizeof(buf), "%04u  ", rnd % 10000);

            } else if (t < 12) {
                // Top settles back to label
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                // Bottom: last 2 digits correct, first 2 still glitching
                uint16_t prefix = _punch_rng(state) % 100;
                uint16_t suffix = state->score % 100;
                snprintf(buf, sizeof(buf), "%02u%02u  ", prefix, suffix);

            } else {
                // Top settled, bottom: full score (slot machine stopped)
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                snprintf(buf, sizeof(buf), "%4u  ", state->score);
            }

            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            watch_clear_colon();
            break;
        }

        case PUNCH_MODE_SCORE: {
            uint16_t disp_score;
            if (state->view_idx == 0) {
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                disp_score = state->history[0];
            } else {
                // Show history index in top
                snprintf(buf, sizeof(buf), "H%u", state->view_idx);
                watch_display_text_with_fallback(WATCH_POSITION_TOP, buf, buf);
                disp_score = (state->view_idx < state->history_count)
                             ? state->history[state->view_idx] : 0;
            }
            snprintf(buf, sizeof(buf), "%4u  ", disp_score);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            watch_clear_colon();
            break;
        }
    }
}

// ─── Face callbacks ───────────────────────────────────────────────────────────

void punch_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(punch_state_t));
        memset(*context_ptr, 0, sizeof(punch_state_t));
        punch_state_t *state = (punch_state_t *)*context_ptr;
        state->mode       = PUNCH_MODE_IDLE;
        state->no_sensor  = !movement_has_lis2dw();
        state->rng        = 0xACE1; // LFSR seed
    }
}

void punch_face_activate(void *context) {
    punch_state_t *state = (punch_state_t *)context;
    state->view_idx = 0;
    movement_request_tick_frequency(1);
    _punch_update_display(state);
}

bool punch_face_loop(movement_event_t event, void *context) {
    punch_state_t *state = (punch_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            _punch_update_display(state);
            break;

        case EVENT_TICK:
            switch (state->mode) {
                case PUNCH_MODE_IDLE:
                case PUNCH_MODE_SCORE:
                    break;

                case PUNCH_MODE_COUNTDOWN:
                    state->tick_count++;
                    if (state->tick_count >= PUNCH_COUNTDOWN_TICKS) {
                        // Open the punch window
                        state->mode          = PUNCH_MODE_READY;
                        state->tick_count    = 0;
                        state->baseline_mag2 = 0;
                        state->peak_mag2     = 0;
                        movement_request_tick_frequency(16);
                    }
                    _punch_update_display(state);
                    break;

                case PUNCH_MODE_READY: {
                    // Sample accelerometer each tick (16 Hz → 1 second window)
                    if (!state->no_sensor) {
                        lis2dw_reading_t r = lis2dw_get_raw_reading();
                        int64_t x = (int64_t)r.x;
                        int64_t y = (int64_t)r.y;
                        int64_t z = (int64_t)r.z;
                        int64_t mag2 = x*x + y*y + z*z;

                        if (state->tick_count == 0) {
                            // First sample establishes baseline (arm at rest)
                            state->baseline_mag2 = mag2;
                            state->peak_mag2     = mag2;
                        } else if (mag2 > state->peak_mag2) {
                            state->peak_mag2 = mag2;
                        }
                    }

                    state->tick_count++;

                    if (state->tick_count >= PUNCH_MEASURE_TICKS) {
                        // ── Compute score ──
                        uint16_t score;
                        if (!state->no_sensor) {
                            int64_t impact = state->peak_mag2 - state->baseline_mag2;
                            if (impact < 0) impact = 0;
                            uint64_t sc = (uint64_t)((impact >= (int64_t)(PUNCH_SCALE_FACTOR * 1000ULL))
                                          ? 1000ULL
                                          : (uint64_t)(impact * 1000ULL / PUNCH_SCALE_FACTOR));
                            score = (uint16_t)sc;
                        } else {
                            // No hardware sensor: give a fun random score for simulator use
                            uint16_t r1 = _punch_rng(state);
                            uint16_t r2 = _punch_rng(state);
                            score = (uint16_t)(((uint32_t)r1 * r2) % 1001);
                        }

                        _punch_record_score(state, score);

                        // ── Start animation ──
                        state->mode       = PUNCH_MODE_ANIMATING;
                        state->tick_count = 0;
                        movement_request_tick_frequency(8);
                        movement_play_sequence((int8_t *)_punch_glitch_seq, BUZZER_PRIORITY_ALARM);
                    }
                    _punch_update_display(state);
                    break;
                }

                case PUNCH_MODE_ANIMATING:
                    state->tick_count++;
                    if (state->tick_count >= PUNCH_ANIM_TICKS) {
                        // Animation complete → show score and play chime
                        state->mode       = PUNCH_MODE_SCORE;
                        state->view_idx   = 0;
                        state->tick_count = 0;
                        movement_request_tick_frequency(1);
                        movement_play_sequence(
                            (int8_t *)_punch_chimes[_punch_chime_index(state->history[0])],
                            BUZZER_PRIORITY_ALARM);
                    }
                    _punch_update_display(state);
                    break;
            }
            break;

        // ── Buttons ───────────────────────────────────────────────────────────

        case EVENT_ALARM_BUTTON_UP:
            if (state->mode == PUNCH_MODE_SCORE && state->history_count > 0) {
                state->view_idx = (state->view_idx + 1) % state->history_count;
                _punch_update_display(state);
            }
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            break; // suppress LED on short press

        case EVENT_LIGHT_BUTTON_UP:
            if (state->mode == PUNCH_MODE_SCORE && state->history_count > 0) {
                state->view_idx = (state->view_idx == 0)
                                  ? (state->history_count - 1)
                                  : (state->view_idx - 1);
                _punch_update_display(state);
            }
            break;

        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;

        case EVENT_ALARM_LONG_PRESS:
            // Start new measurement sequence from any state
            state->mode       = PUNCH_MODE_COUNTDOWN;
            state->tick_count = 0;
            state->view_idx   = 0;
            movement_request_tick_frequency(1);
            movement_play_sequence((int8_t *)_punch_countdown_seq, BUZZER_PRIORITY_ALARM);
            _punch_update_display(state);
            break;

        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void punch_face_resign(void *context) {
    (void) context;
    movement_request_tick_frequency(1);
}
