/*
 * MIT License
 *
 * Copyright (c) 2024 Second Movement Contributors
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

#pragma once

/**
 * @file rhythm_tap.h
 * @brief Rhythm tap recognition module for Second Movement.
 *
 * This module detects the "Shave and a Haircut, Two Bits" rhythm pattern
 * using the LIS2DW single-tap interrupt.  It is a standalone utility that
 * any watch face can opt into; it does not register a face of its own.
 *
 * PATTERN: 7 taps with these inter-tap interval ratios (shortest gap = 1 unit):
 *   Gap  1 (tap 1→2): ~1.0
 *   Gap  2 (tap 2→3): ~1.0
 *   Gap  3 (tap 3→4): ~0.5  (the quick double-tap)
 *   Gap  4 (tap 4→5): ~1.0
 *   Gap  5 (tap 5→6): ~2.0  (the pause before "two bits")
 *   Gap  6 (tap 6→7): ~1.0
 *
 * USAGE — in your watch face:
 *
 *   void my_face_activate(void *context) {
 *       movement_request_tick_frequency(8);   // or whatever you need
 *       rhythm_tap_enable(my_tap_callback);
 *   }
 *
 *   bool my_face_loop(movement_event_t event, void *context) {
 *       switch (event.event_type) {
 *           case EVENT_TICK:
 *               // call once per second for timeout housekeeping
 *               if (event.subsecond == 0) rhythm_tap_tick();
 *               break;
 *           case EVENT_ACCELEROMETER_WAKE:
 *           case EVENT_SINGLE_TAP:
 *               rhythm_tap_process_event();
 *               break;
 *           ...
 *       }
 *       return true;
 *   }
 *
 *   void my_face_resign(void *context) {
 *       rhythm_tap_disable();
 *   }
 */

#include <stdbool.h>
#include "movement.h"

/// Callback invoked when the "Shave and a Haircut" pattern is detected.
typedef void (*rhythm_tap_callback_t)(void);

/**
 * @brief Enable tap detection and register a pattern-match callback.
 *
 * Calls movement_enable_tap_detection_if_available() to configure the LIS2DW
 * for single-tap interrupts and begins recording tap timestamps.  Replaces any
 * previously registered callback.  Safe to call multiple times (re-enables and
 * resets state on each call).
 *
 * @param callback  Function to call when the pattern is matched.  May be NULL
 *                  to disable the callback while still tracking taps.
 */
void rhythm_tap_enable(rhythm_tap_callback_t callback);

/**
 * @brief Disable tap detection and stop tracking taps.
 *
 * Calls movement_disable_tap_detection_if_available() and clears internal
 * state.  Should be called from the face's resign handler.
 */
void rhythm_tap_disable(void);

/**
 * @brief Record a tap and attempt a pattern match.
 *
 * Call this from your face loop whenever the accelerometer fires a tap event
 * (EVENT_ACCELEROMETER_WAKE, EVENT_SINGLE_TAP, or similar).  Reads the LIS2DW
 * interrupt source to confirm a single-tap, records the current RTC counter
 * timestamp, and then tries to match the stored tap sequence against the
 * "Shave and a Haircut" pattern.
 *
 * @return true if this call completed a successful pattern match (the callback
 *         will also have been invoked), false otherwise.
 */
bool rhythm_tap_process_event(void);

/**
 * @brief Advance the inactivity timeout counter by one second.
 *
 * Call this once per second (e.g. when event.subsecond == 0 inside
 * EVENT_TICK).  If no tap has been received for 3 consecutive seconds the tap
 * buffer is cleared so stale partial patterns do not pollute a new attempt.
 */
void rhythm_tap_tick(void);
