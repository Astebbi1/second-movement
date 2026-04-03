/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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

#ifndef MOVEMENT_CONFIG_H_
#define MOVEMENT_CONFIG_H_

#include "movement_faces.h"
#include "zones.h"

const watch_face_t watch_faces[] = {
    // --- primary faces (indices 0-6) ---
    clock_face,
    awake_face,
    step_counter_face,
    sunrise_sunset_face,
    nxtup_face,
    day_one_face,
    wyoscan_face,
    // --- secondary faces / tools (indices 7-14, long-press Mode from face 0) ---
    stebbs_face,
    stebbs_countdown_face,
    active_countdown_face,
    fast_stopwatch_face,
    bubble_level_face,
    tap_tempo_face,
    temperature_display_face,
    tally_face,
    toss_up_face,
    // --- tertiary faces / settings+games (indices 15-22, really-long-press Mode from face 0) ---
    melody_face,
    chime_face,
    alarm_face,
    set_time_face,
    settings_face,
    voltage_face,
    butterfly_game_face,
    endless_runner_face,
    chess_clock_face,
    dice_face,
    cook_face,
};

const char *watch_face_names[] = {
    // primary
    "Clock",
    "Awake",
    "Step Counter",
    "Sunrise/Sunset",
    "Next Up",
    "Day One",
    "Wyo Scan",
    // secondary
    "Stebbs",
    "Countdown",
    "Life Left",
    "Stopwatch",
    "Bubble Level",
    "Tap Tempo",
    "Temperature",
    "Tally",
    "Coins",
    // tertiary
    "Melody",
    "Chime",
    "Alarm",
    "Set Time",
    "Settings",
    "Battery",
    "Butterfly",
    "Runner",
    "Chess Clock",
    "Dice",
    "Cook Temps",
};

#define MOVEMENT_NUM_FACES (sizeof(watch_faces) / sizeof(watch_face_t))

/* Index of the first secondary (tools) face. Long-press Mode jumps here from face 0. */
#define MOVEMENT_SECONDARY_FACE_INDEX 7

/* Index of the first tertiary (games) face. Really-long-press Mode jumps here from face 0. */
#define MOVEMENT_TERTIARY_FACE_INDEX 15

/* Default timezone for first boot. UTZ_NEW_YORK = America/New_York (Eastern, includes DST) */
#define MOVEMENT_DEFAULT_TIMEZONE UTZ_NEW_YORK


/* Determines the intensity of the led colors
 * Set a hex value 0-15 with 0x0 being off and 0xF being max intensity
 */
#define MOVEMENT_DEFAULT_RED_COLOR 0x0
#define MOVEMENT_DEFAULT_GREEN_COLOR 0xF
#define MOVEMENT_DEFAULT_BLUE_COLOR 0x0

/* Set to true for 24h mode or false for 12h mode */
#define MOVEMENT_DEFAULT_24H_MODE false

/* Enable or disable the sound on mode button press */
#define MOVEMENT_DEFAULT_BUTTON_SOUND true

#define MOVEMENT_DEFAULT_BUTTON_VOLUME WATCH_BUZZER_VOLUME_LOUD
#define MOVEMENT_DEFAULT_SIGNAL_VOLUME WATCH_BUZZER_VOLUME_LOUD
#define MOVEMENT_DEFAULT_ALARM_VOLUME WATCH_BUZZER_VOLUME_LOUD

/* Set the timeout before switching back to the main watch face
 * Valid values are:
 * 0: 60 seconds
 * 1: 2 minutes
 * 2: 5 minutes
 * 3: 30 minutes
 */
#define MOVEMENT_DEFAULT_TIMEOUT_INTERVAL 3

/* Set the timeout before switching to low energy mode
 * Valid values are:
 * 0: Never
 * 1: 10 minutes
 * 2: 1 hour
 * 3: 2 hours
 * 4: 6 hours
 * 5: 12 hours
 * 6: 1 day
 * 7: 7 days
 */
#define MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL 0

/* Set the led duration
 * Valid values are:
 * 0: No LED
 * 1: 1 second
 * 2: 3 seconds
 * 3: 5 seconds
 * 4: Rainbow (4-second gradient)
 * 5: Toggle (press on/off)
 */
#define MOVEMENT_DEFAULT_LED_DURATION 4

/* Optionally debounce button presses (disable by default).
 * A value of 4 is a good starting point if you have issues
 * with multiple button presses firing.
*/
#define MOVEMENT_DEBOUNCE_TICKS 0

#endif // MOVEMENT_CONFIG_H_
