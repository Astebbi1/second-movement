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

#include "melody_library.h"

// --- Melody 1: "The Gambler" by fun. - opening piano riff ---
static const int8_t _gambler_sequence[] = {
    BUZZER_NOTE_A4SHARP_B4FLAT, 9,
    BUZZER_NOTE_D5, 9,
    BUZZER_NOTE_F5, 9,

    BUZZER_NOTE_A4, 9,
    BUZZER_NOTE_D5, 9,
    BUZZER_NOTE_F5, 9,

    BUZZER_NOTE_G4, 9,
    BUZZER_NOTE_A4SHARP_B4FLAT, 9,
    BUZZER_NOTE_D5, 9,

    BUZZER_NOTE_F4, 9,
    BUZZER_NOTE_A4, 9,
    BUZZER_NOTE_C5, 9,

    BUZZER_NOTE_D4SHARP_E4FLAT, 9,
    BUZZER_NOTE_G4, 9,
    BUZZER_NOTE_A4SHARP_B4FLAT, 9,

    BUZZER_NOTE_F4, 9,
    BUZZER_NOTE_A4, 9,
    BUZZER_NOTE_C5, 9,

    BUZZER_NOTE_A4SHARP_B4FLAT, 9,
    BUZZER_NOTE_D5, 9,
    BUZZER_NOTE_A4, 9,
    BUZZER_NOTE_A4SHARP_B4FLAT, 14,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 2: "Who Wants to Be a Millionaire" - tension sting ---
// Transcribed from MIDI (milli.mid track 1), 0.7x tempo
static const int8_t _millionaire_sequence[] = {
    // Phrase 1: G5, pause, F5 F5, dramatic pause
    BUZZER_NOTE_G5, 9,
    BUZZER_NOTE_REST, 18,
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_REST, 46,

    // F5-G5-G#5 ascending
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G5, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G5SHARP_A5FLAT, 9,
    BUZZER_NOTE_REST, 9,

    // Phrase 2: G5, pause, F5 F5, dramatic pause
    BUZZER_NOTE_G5, 9,
    BUZZER_NOTE_REST, 18,
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_REST, 46,

    // Escalating: 3 ascending triplets + Bb5-C6 climax
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G5, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G5SHARP_A5FLAT, 9,
    BUZZER_NOTE_REST, 9,

    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_REST, 18,
    BUZZER_NOTE_G5, 9,
    BUZZER_NOTE_REST, 18,
    BUZZER_NOTE_G5SHARP_A5FLAT, 9,
    BUZZER_NOTE_REST, 9,

    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G5, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G5SHARP_A5FLAT, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_A5SHARP_B5FLAT, 9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_C6, 9,
    BUZZER_NOTE_REST, 55,

    // Trickle down
    BUZZER_NOTE_C6SHARP_D6FLAT, 9,
    BUZZER_NOTE_C6, 9,
    BUZZER_NOTE_G5SHARP_A5FLAT, 9,
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_B5, 9,
    BUZZER_NOTE_A5SHARP_B5FLAT, 9,
    BUZZER_NOTE_G5SHARP_A5FLAT, 9,
    BUZZER_NOTE_F5, 9,
    BUZZER_NOTE_C5, 30,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 3: "Pokemon RBY Intro 2" - title screen melody ---
static const int8_t _pokemon_sequence[] = {
    BUZZER_NOTE_D4, 4,
    BUZZER_NOTE_F4SHARP_G4FLAT, 4,

    BUZZER_NOTE_G4, 10,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G4, 15,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G4, 10,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G4, 10,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G4, 10,
    BUZZER_NOTE_REST, 2,

    BUZZER_NOTE_F4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F4, 4,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F4SHARP_G4FLAT, 4,
    BUZZER_NOTE_REST, 1,

    BUZZER_NOTE_G4, 15,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_B4, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_D5, 18,
    BUZZER_NOTE_REST, 4,

    BUZZER_NOTE_C4, 14,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_F4, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_F5, 14,
    BUZZER_NOTE_REST, 2,

    BUZZER_NOTE_E5, 4,
    BUZZER_NOTE_D5SHARP_E5FLAT, 4,
    BUZZER_NOTE_D5, 22,
    BUZZER_NOTE_REST, 4,

    BUZZER_NOTE_F4, 14,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E4, 4,
    BUZZER_NOTE_D4SHARP_E4FLAT, 4,
    BUZZER_NOTE_D4, 22,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 4: Tetris Theme (Korobeiniki) ---
static const int8_t _tetris_sequence[] = {
    BUZZER_NOTE_E5, 12,
    BUZZER_NOTE_B4, 6,
    BUZZER_NOTE_C5, 6,
    BUZZER_NOTE_D5, 12,
    BUZZER_NOTE_C5, 6,
    BUZZER_NOTE_B4, 6,

    BUZZER_NOTE_A4, 12,
    BUZZER_NOTE_A4, 6,
    BUZZER_NOTE_C5, 6,
    BUZZER_NOTE_E5, 12,
    BUZZER_NOTE_D5, 6,
    BUZZER_NOTE_C5, 6,

    BUZZER_NOTE_B4, 18,
    BUZZER_NOTE_C5, 6,
    BUZZER_NOTE_D5, 12,
    BUZZER_NOTE_E5, 12,

    BUZZER_NOTE_C5, 12,
    BUZZER_NOTE_A4, 12,
    BUZZER_NOTE_A4, 12,
    BUZZER_NOTE_REST, 12,

    BUZZER_NOTE_D5, 18,
    BUZZER_NOTE_F5, 6,
    BUZZER_NOTE_A5, 12,
    BUZZER_NOTE_G5, 6,
    BUZZER_NOTE_F5, 6,

    BUZZER_NOTE_E5, 18,
    BUZZER_NOTE_C5, 6,
    BUZZER_NOTE_E5, 12,
    BUZZER_NOTE_D5, 6,
    BUZZER_NOTE_C5, 6,

    BUZZER_NOTE_B4, 12,
    BUZZER_NOTE_B4, 6,
    BUZZER_NOTE_C5, 6,
    BUZZER_NOTE_D5, 12,
    BUZZER_NOTE_E5, 12,

    BUZZER_NOTE_C5, 12,
    BUZZER_NOTE_A4, 12,
    BUZZER_NOTE_A4, 12,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 5: Jeopardy Think Music ---
// Transcribed from MIDI: opening phrase (C5-F5 bounce + descending run)
static const int8_t _jeopardy_sequence[] = {
    BUZZER_NOTE_C5, 29,
    BUZZER_NOTE_F5, 29,
    BUZZER_NOTE_C5, 29,
    BUZZER_NOTE_F4, 29,
    BUZZER_NOTE_C5, 29,
    BUZZER_NOTE_F5, 29,
    BUZZER_NOTE_C5, 57,
    BUZZER_NOTE_C5, 29,
    BUZZER_NOTE_F5, 29,
    BUZZER_NOTE_C5, 29,
    BUZZER_NOTE_F5, 29,
    BUZZER_NOTE_A5, 43,
    BUZZER_NOTE_G5, 14,
    BUZZER_NOTE_F5, 14,
    BUZZER_NOTE_E5, 14,
    BUZZER_NOTE_D5, 14,
    BUZZER_NOTE_C5, 57,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 6: Nickelodeon Jingle ---
// Transcribed from MIDI, transposed up 1 octave
static const int8_t _nickelodeon_sequence[] = {
    BUZZER_NOTE_C5,   9,
    BUZZER_NOTE_C5,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_C5,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G4,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_G4,   9,
    BUZZER_NOTE_C5,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_E5,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_C5,   9,
    BUZZER_NOTE_REST, 26,
    BUZZER_NOTE_A4,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_A4,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_B4,   9,
    BUZZER_NOTE_REST, 9,
    BUZZER_NOTE_B4,   9,
    BUZZER_NOTE_C5,   79,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 7: Carmen Sandiego Theme ---
// Transcribed from MIDI (newcarmen.mid)
static const int8_t _carmen_sequence[] = {
    // Opening riff: da-da, DAH-DAH-DAH-DAH-DUH
    BUZZER_NOTE_C4SHARP_D4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_C4SHARP_D4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_A4SHARP_B4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_A4SHARP_B4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_A4SHARP_B4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_A4SHARP_B4FLAT, 6,
    BUZZER_NOTE_B4, 6,
    BUZZER_NOTE_REST, 24,

    // Response: B4 triplet into descending run
    BUZZER_NOTE_B4, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_B4, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G4SHARP_A4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_F4SHARP_G4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G4SHARP_A4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_F4SHARP_G4FLAT, 6,

    // High point + descent
    BUZZER_NOTE_C5SHARP_D5FLAT, 6,
    BUZZER_NOTE_REST, 12,
    BUZZER_NOTE_A4SHARP_B4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G4SHARP_A4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_F4SHARP_G4FLAT, 6,
    BUZZER_NOTE_REST, 6,

    // Bridge: F#4 settle into G#4
    BUZZER_NOTE_F4SHARP_G4FLAT, 6,
    BUZZER_NOTE_REST, 18,
    BUZZER_NOTE_F4SHARP_G4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_F4SHARP_G4FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G4SHARP_A4FLAT, 6,
    BUZZER_NOTE_REST, 18,
    BUZZER_NOTE_G4SHARP_A4FLAT, 6,
    BUZZER_NOTE_REST, 18,

    // Bass ending
    BUZZER_NOTE_E2, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_E2, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_E2, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_E2, 6,
    BUZZER_NOTE_F2SHARP_G2FLAT, 6,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_F2SHARP_G2FLAT, 6,

    BUZZER_NOTE_REST, 16,

    0 // end
};

// --- Melody 8: Jurassic Park (alarm-eligible, index 7) ---
static const int8_t _jurassic_sequence[] = {
    BUZZER_NOTE_B5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_B5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5SHARP_G5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_E5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_B5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_B5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5SHARP_G5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_E5, 13,
    BUZZER_NOTE_REST, 16,
    0
};

// --- Melody_face only entries (indices 8-16): signal chimes playable from melody_face ---

// Index 8: Mario 1-Up
static const int8_t _mario_1up_sequence[] = {
    BUZZER_NOTE_E6, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G6, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E7, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_D7, 4,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G7, 4,
    BUZZER_NOTE_REST, 16,
    0
};

// Index 9: Mario Power-Up
static const int8_t _mario_pup_sequence[] = {
    BUZZER_NOTE_C5, 2,
    BUZZER_NOTE_C5, 2,
    BUZZER_NOTE_E5, 2,
    BUZZER_NOTE_G5, 2,
    BUZZER_NOTE_C6, 2,
    BUZZER_NOTE_G5, 2,
    BUZZER_NOTE_C6, 2,
    BUZZER_NOTE_E6, 2,
    BUZZER_NOTE_G6, 2,
    BUZZER_NOTE_C7, 2,
    BUZZER_NOTE_G6, 2,
    BUZZER_NOTE_G5SHARP_A5FLAT, 2,
    BUZZER_NOTE_C6, 2,
    BUZZER_NOTE_D6SHARP_E6FLAT, 2,
    BUZZER_NOTE_G6SHARP_A6FLAT, 2,
    BUZZER_NOTE_E6, 2,
    BUZZER_NOTE_A6, 2,
    BUZZER_NOTE_C7, 2,
    BUZZER_NOTE_D7SHARP_E7FLAT, 2,
    BUZZER_NOTE_G7SHARP_A7FLAT, 2,
    BUZZER_NOTE_E7, 2,
    BUZZER_NOTE_A5SHARP_B5FLAT, 2,
    BUZZER_NOTE_D6, 2,
    BUZZER_NOTE_F6, 2,
    BUZZER_NOTE_A6SHARP_B6FLAT, 2,
    BUZZER_NOTE_F6, 2,
    BUZZER_NOTE_B6, 2,
    BUZZER_NOTE_D7, 2,
    BUZZER_NOTE_F7, 2,
    BUZZER_NOTE_A7SHARP_B7FLAT, 2,
    BUZZER_NOTE_F7, 2,
    BUZZER_NOTE_REST, 16,
    0
};

// Index 10: Windows XP
static const int8_t _windows_xp_sequence[] = {
    BUZZER_NOTE_D7SHARP_E7FLAT, 4,
    BUZZER_NOTE_REST, 12,
    BUZZER_NOTE_D6SHARP_E6FLAT, 4,
    BUZZER_NOTE_REST, 4,
    BUZZER_NOTE_A6SHARP_B6FLAT, 4,
    BUZZER_NOTE_REST, 20,
    BUZZER_NOTE_G6SHARP_A6FLAT, 4,
    BUZZER_NOTE_REST, 12,
    BUZZER_NOTE_D6SHARP_E6FLAT, 4,
    BUZZER_NOTE_REST, 12,
    BUZZER_NOTE_D7SHARP_E7FLAT, 4,
    BUZZER_NOTE_REST, 12,
    BUZZER_NOTE_A6SHARP_B6FLAT, 4,
    BUZZER_NOTE_REST, 16,
    0
};

// Index 11: Game Boy Startup
static const int8_t _gameboy_sequence[] = {
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_C7, 8,
    BUZZER_NOTE_REST, 16,
    0
};

// Index 12: Game Boy Pause
static const int8_t _gameboy_pause_sequence[] = {
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_REST, 5,
    -4, 1,
    0
};

// Index 13: Westminster Chimes
static const int8_t _westminster_sequence[] = {
    BUZZER_NOTE_G7SHARP_A7FLAT, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_E7, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_F7SHARP_G7FLAT, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_B6, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 74,
    BUZZER_NOTE_B6, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_F7SHARP_G7FLAT, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_G7SHARP_A7FLAT, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    BUZZER_NOTE_REST, 34,
    BUZZER_NOTE_E7, 1,
    BUZZER_NOTE_REST, 1,
    -2, 2,
    0
};

// Index 14: Power Rangers
static const int8_t _power_rangers_sequence[] = {
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_C8, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_F8, 6,
    BUZZER_NOTE_REST, 8,
    BUZZER_NOTE_D8, 6,
    BUZZER_NOTE_REST, 16,
    0
};

// Index 15: Kim Possible
static const int8_t _kim_sequence[] = {
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_A7SHARP_B7FLAT, 6,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_G7, 6,
    BUZZER_NOTE_G4, 2,
    BUZZER_NOTE_REST, 16,
    0
};

// Index 16: MGS Codec
static const int8_t _mgs_sequence[] = {
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_G5SHARP_A5FLAT, 1,
    BUZZER_NOTE_C6, 1,
    BUZZER_NOTE_REST, 16,
    0
};

// --- Tune table ---
// Indices 0-7 (ALARM_MELODY_NUM_TUNES) are available in alarm_face for per-alarm selection.
// Indices 8-16 are melody_face-only extras.
const melody_tune_t melody_tunes[MELODY_NUM_TUNES] = {
    { "      GAMblr", 12, 'G', _gambler_sequence },       // 0 - alarm
    { "      MIllNR", 12, 'M', _millionaire_sequence },   // 1 - alarm
    { "      POkEMN", 12, 'P', _pokemon_sequence },       // 2 - alarm
    { "      tEtrIS", 12, 'T', _tetris_sequence },        // 3 - alarm
    { "      JPRDY ", 12, 'J', _jeopardy_sequence },      // 4 - alarm
    { "      nICk  ", 12, 'N', _nickelodeon_sequence },   // 5 - alarm
    { "      CARMN ", 12, 'C', _carmen_sequence },        // 6 - alarm
    { "      JurASS", 12, 'X', _jurassic_sequence },      // 7 - alarm
    { "       1 uP ", 11, '1', _mario_1up_sequence },     // 8 - melody_face only
    { "      P  uP ", 12, '2', _mario_pup_sequence },     // 9 - melody_face only
    { "      WiNxP ", 12, 'W', _windows_xp_sequence },   // 10 - melody_face only
    { "      Gb StA", 12, 'B', _gameboy_sequence },       // 11 - melody_face only
    { "      Gb PAu", 12, 'b', _gameboy_pause_sequence }, // 12 - melody_face only
    { "      WEStmi", 12, 'Q', _westminster_sequence },   // 13 - melody_face only
    { "      rAngEr", 12, 'R', _power_rangers_sequence }, // 14 - melody_face only
    { "      KiM   ", 12, 'K', _kim_sequence },           // 15 - melody_face only
    { "      MGS   ", 12, 'S', _mgs_sequence },           // 16 - melody_face only
};
