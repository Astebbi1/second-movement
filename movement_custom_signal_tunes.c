/*
 * MIT License
 *
 * Copyright (c) 2023 Jeremy O'Brien
 * Copyright (c) 2026 Stebbs (runtime table, 6 new tunes)
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

#include "movement_custom_signal_tunes.h"

// --- Tune 0: Default ---
static const int8_t _signal_tune_default[] = {
    BUZZER_NOTE_C8, 5,
    BUZZER_NOTE_REST, 6,
    BUZZER_NOTE_C8, 5,
    0
};

// --- Tune 1: Zelda Secret ---
static const int8_t _signal_tune_zelda[] = {
    BUZZER_NOTE_G5, 8,
    BUZZER_NOTE_F5SHARP_G5FLAT, 8,
    BUZZER_NOTE_D5SHARP_E5FLAT, 8,
    BUZZER_NOTE_A4, 8,
    BUZZER_NOTE_G4SHARP_A4FLAT, 8,
    BUZZER_NOTE_E5, 8,
    BUZZER_NOTE_G5SHARP_A5FLAT, 8,
    BUZZER_NOTE_C6, 20,
    0
};

// --- Tune 2: Mario Theme ---
static const int8_t _signal_tune_mario[] = {
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 2,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 10,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 11,
    BUZZER_NOTE_C6, 7,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 7,
    BUZZER_NOTE_REST, 10,
    BUZZER_NOTE_G6, 8,
    BUZZER_NOTE_REST, 30,
    BUZZER_NOTE_G5, 8,
    0
};

// --- Tune 3: MGS Codec ---
static const int8_t _signal_tune_mgs[] = {
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
    0
};

// --- Tune 4: Kim Possible ---
static const int8_t _signal_tune_kim[] = {
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
    0
};

// --- Tune 5: Power Rangers ---
static const int8_t _signal_tune_rangers[] = {
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
    0
};

// --- Tune 6: Layla ---
static const int8_t _signal_tune_layla[] = {
    BUZZER_NOTE_A6, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_C7, 5,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D7, 20,
    0
};

// --- Tune 7: Harry Potter (short) ---
static const int8_t _signal_tune_harry_short[] = {
    BUZZER_NOTE_B5, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A6, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 24,
    0
};

// --- Tune 8: Harry Potter (long) ---
static const int8_t _signal_tune_harry_long[] = {
    BUZZER_NOTE_B5, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_A6, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 24,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_E6, 12,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_G6, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6SHARP_G6FLAT, 6,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_D6SHARP_E6FLAT, 16,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_F6, 8,
    BUZZER_NOTE_REST, 1,
    BUZZER_NOTE_B5, 24,
    0
};

// --- Tune 9: Jurassic Park ---
static const int8_t _signal_tune_jurassic[] = {
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
    0
};

// --- Tune 10: Evangelion ---
static const int8_t _signal_tune_evangelion[] = {
    BUZZER_NOTE_C5, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_D5SHARP_E5FLAT, 13,
    BUZZER_NOTE_REST, 13,
    BUZZER_NOTE_F5, 13,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_D5SHARP_E5FLAT, 13,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_A5SHARP_B5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5SHARP_A5FLAT, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5, 3,
    BUZZER_NOTE_REST, 3,
    BUZZER_NOTE_F5, 7,
    BUZZER_NOTE_REST, 7,
    BUZZER_NOTE_G5, 13,
    0
};

// --- Tune 11: Mario 1-Up (new) ---
static const int8_t _signal_tune_mario_1up[] = {
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
    BUZZER_NOTE_REST, 2,
    0
};

// --- Tune 12: Mario Power-Up (new) ---
static const int8_t _signal_tune_mario_pup[] = {
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
    0
};

// --- Tune 13: Windows XP (new) ---
static const int8_t _signal_tune_windows_xp[] = {
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
    0
};

// --- Tune 14: Game Boy Startup (new) ---
static const int8_t _signal_tune_gameboy[] = {
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_C7, 8,
    BUZZER_NOTE_REST, 4,
    0
};

// --- Tune 15: Game Boy Pause (new) ---
// Uses repeat encoding: -4,1 means go back 4 bytes and repeat once
static const int8_t _signal_tune_gameboy_pause[] = {
    BUZZER_NOTE_C6, 4,
    BUZZER_NOTE_REST, 5,
    BUZZER_NOTE_C7, 4,
    BUZZER_NOTE_REST, 5,
    -4, 1,
    0
};

// --- Tune 16: Westminster Chimes (new) ---
// Uses repeat encoding: -2,2 means go back 2 bytes and repeat twice
static const int8_t _signal_tune_westminster[] = {
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

// --- Lookup table ---
const signal_tune_t signal_tunes[SIGNAL_NUM_TUNES] = {
    { "dEfAul", _signal_tune_default     },  //  0
    { "ZEldA ", _signal_tune_zelda       },  //  1
    { "MAriO ", _signal_tune_mario       },  //  2
    { "MGS   ", _signal_tune_mgs         },  //  3
    { "KiM   ", _signal_tune_kim         },  //  4
    { "rAngEr", _signal_tune_rangers     },  //  5
    { "lAylA ", _signal_tune_layla       },  //  6
    { "HArry ", _signal_tune_harry_short },  //  7
    { "HArryL", _signal_tune_harry_long  },  //  8
    { "JurASS", _signal_tune_jurassic    },  //  9
    { "EvAngL", _signal_tune_evangelion  },  // 10
    { " 1 uP ", _signal_tune_mario_1up   },  // 11
    { "P  uP ", _signal_tune_mario_pup   },  // 12
    { "WiNxP ", _signal_tune_windows_xp  },  // 13
    { "Gb StA", _signal_tune_gameboy     },  // 14
    { "Gb PAu", _signal_tune_gameboy_pause }, // 15
    { "WEStmi", _signal_tune_westminster },  // 16
};
