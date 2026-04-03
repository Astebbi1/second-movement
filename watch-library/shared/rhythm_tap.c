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

#include "rhythm_tap.h"
#include "lis2dw.h"
#include "watch_rtc.h"

/* -------------------------------------------------------------------------
 * Constants
 * -------------------------------------------------------------------------
 *
 * The RTC counter runs at 128 Hz (confirmed in both hardware and simulator
 * implementations of watch_rtc.c).  All timing here is in RTC counter ticks.
 *
 * Timeout: 3 seconds with no new tap → reset the buffer.
 * We track this with a whole-second counter incremented by rhythm_tap_tick().
 */

#define RHYTHM_TAP_MAX_TAPS         8
#define RHYTHM_TAP_REQUIRED_TAPS    7
#define RHYTHM_TAP_TIMEOUT_SECS     3

/*
 * Pattern: "Shave and a Haircut, Two Bits"
 * 6 inter-tap intervals expressed as ratios × 100 (fixed-point, base 100).
 * The shortest gap is 50 (representing 0.5 units); all others are multiples.
 *
 *   Index  Syllables          Ratio  ×100
 *     0    shave → and          1.0   100
 *     1    and  → a             1.0   100
 *     2    a    → hair          0.5    50  ← quick double
 *     3    hair → cut           1.0   100
 *     4    cut  → two           2.0   200  ← the pause
 *     5    two  → bits          1.0   100
 *
 * Tolerance: ±40 % (TOLERANCE_PCT = 40).  A gap passes when:
 *   actual_ratio_x100 is within [target × 60/100, target × 140/100].
 */
#define RHYTHM_TAP_NUM_GAPS     6
#define TOLERANCE_PCT           40

static const uint16_t pattern_ratios[RHYTHM_TAP_NUM_GAPS] = {
    100,  /* gap 0: ~1 unit  */
    100,  /* gap 1: ~1 unit  */
     50,  /* gap 2: ~0.5 unit (quick double) */
    100,  /* gap 3: ~1 unit  */
    200,  /* gap 4: ~2 units (pause before "two bits") */
    100,  /* gap 5: ~1 unit  */
};

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */

typedef struct {
    /* Circular buffer of tap timestamps (RTC counter ticks, 128 Hz). */
    rtc_counter_t timestamps[RHYTHM_TAP_MAX_TAPS];
    uint8_t       count;            /* number of valid taps stored (0..8) */

    /* Seconds elapsed since the last tap, used for timeout reset. */
    uint8_t       idle_seconds;

    /* Whether the module is currently enabled. */
    bool          enabled;

    /* User-supplied callback. */
    rhythm_tap_callback_t callback;
} rhythm_tap_state_t;

static rhythm_tap_state_t _state;

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

/** Reset the tap buffer but leave everything else intact. */
static void _reset_buffer(void) {
    _state.count        = 0;
    _state.idle_seconds = 0;
}

/**
 * @brief Attempt to match the last RHYTHM_TAP_REQUIRED_TAPS timestamps
 *        against the target pattern.
 *
 * Algorithm:
 *  1. Extract the 6 inter-tap intervals (gaps) in RTC ticks.
 *  2. Find the minimum gap (it represents "1 unit" before any normalization,
 *     but we avoid the floating-point division by cross-multiplying instead).
 *  3. For each gap, check whether (gap / min_gap) is within ±TOLERANCE_PCT
 *     of the target ratio.  We rewrite that as:
 *       gap * target ∈ [min_gap * target * (100-tol)/100,
 *                       min_gap * target * (100+tol)/100]
 *     Using only 32-bit integer arithmetic (gaps are at most ~128*10 = 1280
 *     ticks for a 10-second inter-tap, well within uint32_t range after
 *     multiplication by 200 and 140).
 *
 * @return true if all 6 gaps match the pattern within tolerance.
 */
static bool _try_match(void) {
    if (_state.count < RHYTHM_TAP_REQUIRED_TAPS) return false;

    /* We always operate on the most-recent RHYTHM_TAP_REQUIRED_TAPS taps. */
    uint8_t base = _state.count - RHYTHM_TAP_REQUIRED_TAPS;

    /* Compute the 6 inter-tap gaps (always positive; counter is monotone). */
    uint32_t gaps[RHYTHM_TAP_NUM_GAPS];
    uint32_t min_gap = UINT32_MAX;
    for (int i = 0; i < RHYTHM_TAP_NUM_GAPS; i++) {
        gaps[i] = _state.timestamps[base + i + 1] - _state.timestamps[base + i];
        if (gaps[i] < min_gap) min_gap = gaps[i];
    }

    /* Reject degenerate inputs (two identical timestamps). */
    if (min_gap == 0) return false;

    /*
     * Sanity check: the shortest gap should correspond to the half-unit in
     * the pattern (~0.5 units).  The longest should be ~2 units, so at most
     * 4× the shortest.  Reject sequences where the range is wildly off.
     * This also guards against overflow in the multiplication below.
     *
     * We allow up to 6× to give generous tolerance for the pause gap.
     */
    for (int i = 0; i < RHYTHM_TAP_NUM_GAPS; i++) {
        if (gaps[i] > min_gap * 6) return false;
    }

    /*
     * Core match: for each gap g[i] with target ratio r[i] (×100) and
     * minimum gap m, check:
     *
     *   g[i] / m  ≈  r[i] / 100
     *
     * Rewritten without division:
     *
     *   g[i] * 100 * (100 - TOLERANCE_PCT)  <=  m * r[i] * 100   AND
     *   g[i] * 100 * (100 + TOLERANCE_PCT)  >=  m * r[i] * 100
     *
     * Simplified:
     *
     *   g[i] * (100 - TOLERANCE_PCT)  <=  m * r[i]
     *   g[i] * (100 + TOLERANCE_PCT)  >=  m * r[i]
     */
    for (int i = 0; i < RHYTHM_TAP_NUM_GAPS; i++) {
        uint32_t lhs_lo = gaps[i] * (uint32_t)(100 - TOLERANCE_PCT);
        uint32_t lhs_hi = gaps[i] * (uint32_t)(100 + TOLERANCE_PCT);
        uint32_t rhs    = min_gap  * (uint32_t)pattern_ratios[i];

        if (lhs_lo > rhs) return false;  /* gap is too large relative to min */
        if (lhs_hi < rhs) return false;  /* gap is too small relative to min */
    }

    return true;
}

/** Append a tap timestamp, overwriting the oldest if the buffer is full. */
static void _record_tap(rtc_counter_t ts) {
    if (_state.count < RHYTHM_TAP_MAX_TAPS) {
        _state.timestamps[_state.count++] = ts;
    } else {
        /* Shift buffer left by one, dropping the oldest tap. */
        for (int i = 0; i < RHYTHM_TAP_MAX_TAPS - 1; i++) {
            _state.timestamps[i] = _state.timestamps[i + 1];
        }
        _state.timestamps[RHYTHM_TAP_MAX_TAPS - 1] = ts;
        /* count stays at RHYTHM_TAP_MAX_TAPS */
    }
    _state.idle_seconds = 0;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

void rhythm_tap_enable(rhythm_tap_callback_t callback) {
    _reset_buffer();
    _state.callback = callback;
    _state.enabled  = true;
    movement_enable_tap_detection_if_available();
}

void rhythm_tap_disable(void) {
    movement_disable_tap_detection_if_available();
    _reset_buffer();
    _state.callback = NULL;
    _state.enabled  = false;
}

bool rhythm_tap_process_event(void) {
    if (!_state.enabled) return false;

    /*
     * Read the interrupt source register to confirm this is a single-tap
     * event.  We check both the TAP_SRC register (which has per-axis detail)
     * and the ALL_INT_SRC register.  Either a single-tap bit is sufficient;
     * we do not require double-tap here because the pattern consists of
     * individual taps separated by deliberate gaps.
     */
    lis2dw_interrupt_source_t int_src = lis2dw_get_interrupt_source();
    if (!(int_src & LIS2DW_REG_ALL_INT_SRC_SINGLE_TAP)) {
        /*
         * Not a single-tap according to ALL_INT_SRC.  Check TAP_SRC for
         * the TAP_IA bit as a fallback (covers cases where only TAP_SRC
         * is asserted before ALL_INT_SRC is latched).
         */
        uint8_t tap_src = watch_i2c_read8(LIS2DW_ADDRESS, LIS2DW_REG_TAP_SRC);
        if (!(tap_src & (LIS2DW_TAP_SRC_VAL_TAP_IA | LIS2DW_TAP_SRC_VAL_SINGLE_TAP))) {
            return false;
        }
    }

    rtc_counter_t now = watch_rtc_get_counter();
    _record_tap(now);

    bool matched = _try_match();
    if (matched) {
        _reset_buffer();  /* clear so a new pattern can start immediately */
        if (_state.callback) _state.callback();
    }

    return matched;
}

void rhythm_tap_tick(void) {
    if (!_state.enabled) return;
    if (_state.count == 0) return;

    _state.idle_seconds++;
    if (_state.idle_seconds >= RHYTHM_TAP_TIMEOUT_SECS) {
        _reset_buffer();
    }
}
