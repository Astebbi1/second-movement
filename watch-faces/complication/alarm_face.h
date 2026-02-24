/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger
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

#ifndef ALARM_FACE_H_
#define ALARM_FACE_H_

/*
 * ALARM face
 *
 * Implements up to 16 alarm slots on the sensor watch with melody support.
 *
 * Usage:
 *    - In normal mode, the alarm button cycles through all 16 alarms.
 *    - Pressing the alarm button long in normal mode toggles the alarm on or off.
 *      (Pressing the alarm button extra long brings you back to alarm no. 1.)
 *    - Pressing the light button enters setting mode and cycles through alarm settings.
 *      (Long pressing the light button enters setting mode without illuminating the led.)
 *    - In setting mode an alarm slot is selected by pressing the alarm button when the
 *      slot number in the upper right corner is blinking.
 *    - For each alarm slot, you can select the day:
 *        - ED = the alarm rings every day
 *        - 1t = the alarm fires only one time and is erased afterwards
 *        - MF = the alarm fires Mondays to Fridays
 *        - WN = the alarm fires on weekends (Sa/Su)
 *        - MO to SO = the alarm fires only on the given day of week
 *    - You can fast cycle through hour or minute setting via long press of the alarm button.
 *    - You can select the tone in which the alarm is played. (Three pitch levels + melody mode.)
 *    - You can select how many "beep rounds" are played for each alarm. 1 to 9 rounds, plus
 *      extra long ('L') and extra short ('o') alarms. In melody mode, selects which melody.
 */

// Re-use shared types and constants from advanced_alarm_face to avoid duplicate definitions.
#include "advanced_alarm_face.h"
#include "melody_library.h"

void alarm_face_setup(uint8_t watch_face_index, void **context_ptr);
void alarm_face_activate(void *context);
bool alarm_face_loop(movement_event_t event, void *context);
void alarm_face_resign(void *context);
movement_watch_face_advisory_t alarm_face_advise(void *context);

#define alarm_face ((const watch_face_t){ \
    alarm_face_setup, \
    alarm_face_activate, \
    alarm_face_loop, \
    alarm_face_resign, \
    alarm_face_advise, \
})

#endif // ALARM_FACE_H_
