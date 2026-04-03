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
#include "chess_clock_face.h"
#include "watch.h"

// ── Internal helpers ──────────────────────────────────────────────────────────

static void _chess_clock_beep(void) {
    if (movement_button_should_sound())
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
}

/*
 * Render the current state to the display.
 *
 * Top row:
 *   - "CHESS" when not yet started (paused and both timers at zero)
 *   - "P 1  " or "P 2  " for the active player while running or paused mid-game
 *
 * Bottom row:
 *   - Elapsed time of the active player (or the other player while peeking),
 *     formatted as MM:SS. Times over 99:59 are clamped to 99:59.
 */
static void _chess_clock_draw(chess_clock_state_t *state) {
    // Determine which player's time to display
    uint8_t display_player = state->peeking
        ? (state->active_player ^ 1)
        : state->active_player;

    // Top label
    bool at_start = !state->running && state->time_cs[0] == 0 && state->time_cs[1] == 0;
    if (at_start && !state->peeking) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "CHESS", "CH");
    } else {
        if (display_player == 0) {
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "P 1  ", "P1");
        } else {
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "P 2  ", "P2");
        }
    }

    // Bottom: MM:SS of the chosen player's elapsed time
    // time_cs is in centiseconds; divide by 100 to get whole seconds.
    uint32_t total_seconds = state->time_cs[display_player] / 100;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;

    // Clamp to 99:59 so we never overflow the two bottom digit pairs
    if (minutes > 99) {
        minutes = 99;
        seconds = 59;
    }

    char buf[7];
    sprintf(buf, "%02lu%02lu  ", (unsigned long)minutes, (unsigned long)seconds);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    // Keep the colon lit to separate MM from SS
    watch_set_colon();
}

// ── Face callbacks ────────────────────────────────────────────────────────────

void chess_clock_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(chess_clock_state_t));
        memset(*context_ptr, 0, sizeof(chess_clock_state_t));
    }
}

void chess_clock_face_activate(void *context) {
    (void)context;
    // 1 Hz ticks are sufficient for MM:SS resolution
    movement_request_tick_frequency(1);
    watch_set_colon();
}

bool chess_clock_face_loop(movement_event_t event, void *context) {
    chess_clock_state_t *state = (chess_clock_state_t *)context;

    switch (event.event_type) {

        case EVENT_ACTIVATE:
            _chess_clock_draw(state);
            break;

        case EVENT_TICK:
            // Advance the active player's counter by 100 centiseconds (1 second)
            if (state->running) {
                state->time_cs[state->active_player] += 100;
            }
            // Peeking is a one-tick-only behaviour: clear it on each tick so
            // the display returns to the active player automatically.
            state->peeking = false;
            _chess_clock_draw(state);
            break;

        // ── ALARM: pass the clock / start ────────────────────────────────────
        case EVENT_ALARM_BUTTON_UP:
            if (!state->running) {
                // Start Player 1's clock from the initial paused state,
                // or resume whichever player's clock was active mid-game.
                state->running = true;
            } else {
                // Pass the clock to the other player
                state->active_player ^= 1;
            }
            state->peeking = false;
            _chess_clock_beep();
            _chess_clock_draw(state);
            break;

        // ── ALARM long press: reset both timers ───────────────────────────────
        case EVENT_ALARM_LONG_PRESS:
            state->running = false;
            state->active_player = 0;
            state->time_cs[0] = 0;
            state->time_cs[1] = 0;
            state->peeking = false;
            _chess_clock_beep();
            _chess_clock_draw(state);
            break;

        // ── LIGHT short press: peek at other player's time ────────────────────
        // Suppress the default LED behaviour on button-down so the LED only
        // fires on the long press.
        case EVENT_LIGHT_BUTTON_DOWN:
            break;

        case EVENT_LIGHT_BUTTON_UP:
            // Toggle peeking; the next tick clears it automatically
            state->peeking = true;
            _chess_clock_draw(state);
            break;

        // ── LIGHT long press: illuminate LED ─────────────────────────────────
        case EVENT_LIGHT_LONG_PRESS:
            movement_illuminate_led();
            break;

        // ── MODE: move to next face ───────────────────────────────────────────
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_next_face();
            break;

        // ── Timeout: only move away if the clock is not running ───────────────
        case EVENT_TIMEOUT:
            if (!state->running) {
                movement_move_to_face(0);
            }
            // If the clock is running we stay here indefinitely — no timeout.
            break;

        // ── Low-energy update (clock is stopped) ─────────────────────────────
        case EVENT_LOW_ENERGY_UPDATE:
            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CLASSIC) {
                watch_display_text(WATCH_POSITION_SECONDS, "  ");
            }
            if (!watch_sleep_animation_is_running())
                watch_start_sleep_animation(1000);
            break;

        default:
            movement_default_loop_handler(event);
            break;
    }

    // Return true to keep the face alive; the clock must never sleep while running.
    return true;
}

void chess_clock_face_resign(void *context) {
    chess_clock_state_t *state = (chess_clock_state_t *)context;
    // Clear peek state on resign so we come back clean
    state->peeking = false;
}
