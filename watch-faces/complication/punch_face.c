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

// Glitchy animation sound played twice for ~0.75 s of erratic staccato.
static const int8_t _punch_glitch_seq[] = {
    BUZZER_NOTE_C6,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A3,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G5,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D3,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B4,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F3,  2, BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7,  2, BUZZER_NOTE_REST, 1,
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

// Insert score into history sorted descending (best first). Keeps top PUNCH_HISTORY_SIZE scores.
static void _punch_record_score(punch_state_t *state, uint16_t score) {
    uint8_t n = state->history_count;
    // Find insertion point
    uint8_t ins = n < PUNCH_HISTORY_SIZE ? n : PUNCH_HISTORY_SIZE;
    for (uint8_t i = 0; i < (n < PUNCH_HISTORY_SIZE ? n : PUNCH_HISTORY_SIZE); i++) {
        if (score > state->history[i]) { ins = i; break; }
    }
    // If buffer full and score doesn't beat last entry, bail
    if (n >= PUNCH_HISTORY_SIZE && ins >= PUNCH_HISTORY_SIZE) return;
    // Shift right (drop last if full)
    uint8_t limit = n < PUNCH_HISTORY_SIZE ? n : PUNCH_HISTORY_SIZE - 1;
    for (int8_t i = (int8_t)limit; i > (int8_t)ins; i--)
        state->history[i] = state->history[i - 1];
    state->history[ins] = score;
    if (state->history_count < PUNCH_HISTORY_SIZE) state->history_count++;
}

// ─── Display ─────────────────────────────────────────────────────────────────

// Write a 0–1000 score into the 4 large HHMM digits.
static void _punch_show_score(uint16_t score) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%2u", score / 100);
    watch_display_text(WATCH_POSITION_HOURS, buf);
    snprintf(buf, sizeof(buf), "%02u", score % 100);
    watch_display_text(WATCH_POSITION_MINUTES, buf);
    watch_display_text(WATCH_POSITION_SECONDS, "  ");
}

static void _punch_update_display(punch_state_t *state) {
    char buf[7];

    switch (state->mode) {
        case PUNCH_MODE_IDLE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
            watch_display_text(WATCH_POSITION_HOURS, "rE");
            watch_display_text(WATCH_POSITION_MINUTES, "Ad");
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
            watch_clear_colon();
            break;

        case PUNCH_MODE_COUNTDOWN: {
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
            uint8_t n = (uint8_t)(5 - state->tick_count);
            if (n == 0) n = 1;
            watch_display_text(WATCH_POSITION_HOURS, "  ");
            snprintf(buf, sizeof(buf), " %u", n);
            watch_display_text(WATCH_POSITION_MINUTES, buf);
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
            watch_clear_colon();
            break;
        }

        case PUNCH_MODE_READY:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
            watch_display_text(WATCH_POSITION_HOURS, "HI");
            watch_display_text(WATCH_POSITION_MINUTES, "t!");
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
            watch_clear_colon();
            break;

        case PUNCH_MODE_ANIMATING: {
            uint8_t t = state->tick_count;
            uint16_t rnd = _punch_rng(state);

            if (t < 8) {
                // Glitch: alternate top label, fully random bottom digits
                if (t % 2 == 0)
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                else
                    watch_display_text_with_fallback(WATCH_POSITION_TOP, "88888", "88");
                uint16_t rv = rnd % 10000;
                snprintf(buf, sizeof(buf), "%2u", rv / 100);
                watch_display_text(WATCH_POSITION_HOURS, buf);
                snprintf(buf, sizeof(buf), "%02u", rv % 100);
                watch_display_text(WATCH_POSITION_MINUTES, buf);
            } else if (t < 12) {
                // Settling: top label stable, HOURS still glitching, MINUTES locked to score
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                snprintf(buf, sizeof(buf), "%2u", (uint16_t)(_punch_rng(state) % 100));
                watch_display_text(WATCH_POSITION_HOURS, buf);
                snprintf(buf, sizeof(buf), "%02u", state->score % 100);
                watch_display_text(WATCH_POSITION_MINUTES, buf);
            } else {
                // Locked: full score
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                _punch_show_score(state->score);
            }
            watch_display_text(WATCH_POSITION_SECONDS, "  ");
            watch_clear_colon();
            break;
        }

        case PUNCH_MODE_SCORE: {
            uint16_t disp_score;
            if (state->view_idx == 0) {
                // Current round — clear top-right in case rank digit is lingering
                watch_display_text_with_fallback(WATCH_POSITION_TOP, "PUnCH", "PU");
                watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
                disp_score = state->score;
            } else {
                // Ranked history: "SC" top-left, " N" top-right
                watch_display_text(WATCH_POSITION_TOP_LEFT, "SC");
                snprintf(buf, sizeof(buf), " %u", state->view_idx);
                watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
                disp_score = (state->view_idx <= state->history_count)
                             ? state->history[state->view_idx - 1] : 0;
            }
            _punch_show_score(disp_score);
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
                        state->mode            = PUNCH_MODE_READY;
                        state->tick_count      = 0;
                        state->baseline_mag2   = 0;
                        state->peak_mag2       = 0;
                        state->impact_detected = false;
                        state->post_peak_ticks = 0;
                        movement_request_tick_frequency(16);
                    }
                    _punch_update_display(state);
                    break;

                case PUNCH_MODE_READY: {
                    // Sample accelerometer each tick (16 Hz)
                    if (!state->no_sensor) {
                        lis2dw_reading_t r = lis2dw_get_raw_reading();
                        int64_t x = (int64_t)r.x;
                        int64_t y = (int64_t)r.y;
                        int64_t z = (int64_t)r.z;
                        int64_t mag2 = x*x + y*y + z*z;

                        if (state->tick_count == 0) {
                            // First sample: arm at rest — establish baseline
                            state->baseline_mag2 = mag2;
                            state->peak_mag2     = mag2;
                        } else if (mag2 > state->peak_mag2) {
                            state->peak_mag2       = mag2;
                            state->post_peak_ticks = 0;
                            // Onset: spike > 20% of full-scale impact
                            if ((state->peak_mag2 - state->baseline_mag2) >
                                (int64_t)(PUNCH_SCALE_FACTOR / 5)) {
                                state->impact_detected = true;
                            }
                        } else if (state->impact_detected) {
                            state->post_peak_ticks++;
                        }
                    }

                    state->tick_count++;

                    // Exit: timeout OR one-shot (peak detected + 4 decay ticks)
                    bool done = (state->tick_count >= PUNCH_MEASURE_TICKS) ||
                                (state->impact_detected && state->post_peak_ticks >= 4);

                    if (done) {
                        // ── Compute score ──
                        uint16_t score;
                        if (!state->no_sensor) {
                            int64_t impact = state->peak_mag2 - state->baseline_mag2;
                            if (impact < 0) impact = 0;
                            uint64_t sc = (impact >= (int64_t)(PUNCH_SCALE_FACTOR))
                                          ? 1000ULL
                                          : (uint64_t)(impact * 1000ULL / PUNCH_SCALE_FACTOR);
                            score = (uint16_t)sc;
                        } else {
                            // No hardware sensor: fun random score for simulator
                            uint16_t r1 = _punch_rng(state);
                            uint16_t r2 = _punch_rng(state);
                            score = (uint16_t)(((uint32_t)r1 * r2) % 1001);
                        }

                        state->score = score;
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
                // Cycle 0 (current) → 1..history_count (ranked best-first) → 0
                state->view_idx = (state->view_idx + 1) % (state->history_count + 1);
                _punch_update_display(state);
            }
            break;

        case EVENT_LIGHT_BUTTON_DOWN:
            break; // suppress LED on short press

        case EVENT_LIGHT_BUTTON_UP:
            if (state->mode == PUNCH_MODE_SCORE && state->history_count > 0) {
                state->view_idx = (state->view_idx == 0)
                                  ? state->history_count
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
