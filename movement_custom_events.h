/*
 * movement_custom_events.h — yearly event list for nxtup_face
 *
 * Edit this file to add holidays, birthdays, half-birthdays, etc.
 * Events are automatically sorted by days remaining at runtime.
 *
 * NAME TIPS:
 *  - The watch bottom row is 6 chars; longer names scroll automatically.
 *  - Segment limitations: use mixed case to improve readability.
 *    E.g. "AndrEW" renders better than all-caps "ANDREW" in some positions.
 *  - Avoid characters that look identical on 7-segment: Q, K, W, X, Z
 *    are tricky — test on device and tweak spelling if needed.
 *
 * FLOATING HOLIDAYS (Easter, Mother's Day, Thanksgiving…):
 *  - These are intentionally excluded; only fixed-date events are listed.
 */

#ifndef MOVEMENT_CUSTOM_EVENTS_H_
#define MOVEMENT_CUSTOM_EVENTS_H_

#include <stdint.h>

typedef struct {
    const char *name;   // display name — any length, scrolls if > 6 chars
    uint8_t     month;  // 1–12
    uint8_t     day;    // 1–31
} yearly_event_t;

static const yearly_event_t yearly_events[] = {
    // ── January ──────────────────────────────────────────────────
    { "nEW YEAR",          1,  1 },
    { "MAX bdAY",          1, 14 },
    // ── February ─────────────────────────────────────────────────
    { "GROUndhOG dAY",     2,  2 },
    { "dAViS bdAY",        2, 11 },
    { "VAL+JOSH bdAY",     2, 14 },   // Valentine's Day + Josh's birthday
    { "nEiL bdAY",         2, 17 },
    { "mArA 1-2",          2, 23 },   // half-bday (born 08-23)
    // ── March ────────────────────────────────────────────────────
    { "OLLiE 1-2",         3,  1 },   // half-bday (born 09-01)
    { "ROb+JiMMY bdAY",    3,  4 },   // Robynn & Jimmy share this date
    { "PAt bdAY",          3,  7 },
    { "dAVE bdAY",         3, 10 },
    { "Pi dAY",             3, 14 },
    { "BECKY 1-2",         3, 14 },   // half-bday (born 09-14)
    { "St PAtRiCk dAY",    3, 17 },
    { "JEFFErY 1-2",       3, 19 },   // half-bday (born 09-19)
    { "SPrinG EQnoX",      3, 20 },
    { "KiKiSH nEW Yr",     3, 21 },
    { "brAndOn 1-2",       3, 21 },   // half-bday (born 09-21)
    { "LEiGH bdAY",        3, 26 },
    { "JULiA 1-2",         3, 30 },   // half-bday (born 09-30)
    // ── April ────────────────────────────────────────────────────
    { "APriL FOOLS",       4,  1 },
    { "KYLE 1-2",          4,  4 },   // half-bday (born 10-04)
    { "KYLE Jr bdAY",      4,  9 },
    { "tAX dAY",           4, 15 },
    // ── May ──────────────────────────────────────────────────────
    { "AndrEW 1-2",        5,  1 },   // half-bday (born 11-01)
    { "CinCO mAYO",        5,  5 },
    { "MikE 1-2",          5, 19 },   // half-bday (born 11-19)
    // ── June ─────────────────────────────────────────────────────
    { "SHArOn 1-2",        6, 10 },   // half-bday (born 12-10)
    { "FLAg dAY",          6, 14 },
    { "RYkEr 1-2",         6, 18 },   // half-bday (born 12-18)
    { "JUnEtEEntH",        6, 19 },
    { "SUmr SOLStiCE",     6, 21 },
    { "RitA bdAY",         6, 27 },
    // ── July ─────────────────────────────────────────────────────
    { "InDEP dAY",         7,  4 },
    { "MAX 1-2",           7, 14 },   // half-bday (born 01-14)
    { "WinG dAY",          7, 29 },
    // ── August ───────────────────────────────────────────────────
    { "dAViS 1-2",         8, 11 },   // half-bday (born 02-11)
    { "JOSH 1-2",          8, 14 },   // half-bday (born 02-14)
    { "nEiL 1-2",          8, 17 },   // half-bday (born 02-17)
    { "mArA bdAY",         8, 23 },
    // ── September ────────────────────────────────────────────────
    { "OLLiE bdAY",        9,  1 },
    { "ROb+Ji 1-2",        9,  4 },   // half-bday (born 03-04)
    { "PAt 1-2",           9,  7 },   // half-bday (born 03-07)
    { "dAVE 1-2",          9, 10 },   // half-bday (born 03-10)
    { "AnniVErSArY",       9, 11 },
    { "BECKY bdAY",        9, 14 },
    { "JEFFErY bdAY",      9, 19 },
    { "brAndOn bdAY",      9, 21 },
    { "AUt EQnoX",         9, 23 },
    { "LEiGH 1-2",         9, 26 },   // half-bday (born 03-26)
    { "JULiA bdAY",        9, 30 },
    // ── October ──────────────────────────────────────────────────
    { "KYLE bdAY",        10,  4 },
    { "KYLE Jr 1-2",      10,  9 },   // half-bday (born 04-09)
    { "HALLoWEEn",        10, 31 },
    // ── November ─────────────────────────────────────────────────
    { "AndrEW bdAY",      11,  1 },
    { "VEtERANS dAY",     11, 11 },
    { "MikE bdAY",        11, 19 },
    // ── December ─────────────────────────────────────────────────
    { "SHArOn bdAY",      12, 10 },
    { "RYkEr bdAY",       12, 18 },
    { "Wint SOLStiCE",    12, 21 },
    { "CHRiStMAS",        12, 25 },
    { "RitA 1-2",         12, 27 },   // half-bday (born 06-27)
    { "nEW YR EVE",       12, 31 },
};

#define YEARLY_EVENT_COUNT  \
    ((uint8_t)(sizeof(yearly_events) / sizeof(yearly_event_t)))

#endif // MOVEMENT_CUSTOM_EVENTS_H_
