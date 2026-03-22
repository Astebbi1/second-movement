#include "count_steps.h"
#include "stdint.h"
#include "stdio.h"
#include <stdlib.h>

#define DEBUG_PRINT false

#define SIMPLE_THRESHOLD               2000
#define SIMPLE_THRESHOLD_MULT           1.3
#define SIMPLE_SAMP_IGNORE_STEP         3
#define USE_WINDOW_AVG                  true
#define AVG_WINDOW_SIZE_SHIFT           7
#define AVG_WINDOW_SIZE                 ((1 << AVG_WINDOW_SIZE_SHIFT) - 1)
#define MAX_FIFO_SIZE_SIMPLE            13
#define MAX_SIMPLE_STEPS                (MAX_FIFO_SIZE_SIMPLE / SIMPLE_SAMP_IGNORE_STEP)

static uint32_t step_counter_threshold = SIMPLE_THRESHOLD;

uint32_t count_steps_approx_l2_norm(lis2dw_reading_t reading) {
    uint32_t ax = abs(reading.x);
    uint32_t ay = abs(reading.y);
    uint32_t az = abs(reading.z);
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    if (ay < az) { uint32_t t = ay; ay = az; az = t; }
    if (ax < ay) { uint32_t t = ax; ax = ay; ay = t; }
    return ax + ((15 * ay) >> 4) + ((3 * az) >> 3);
}

uint8_t count_steps_simple(lis2dw_fifo_t *fifo_data) {
    uint8_t new_steps = 0;
#if USE_WINDOW_AVG
    uint8_t samples_processed = 0;
    uint32_t samples_sum = 0;
#endif
    for (uint8_t i = 0; i < fifo_data->count; i++) {
        if (i >= MAX_FIFO_SIZE_SIMPLE) break;
        uint32_t magnitude = count_steps_approx_l2_norm(fifo_data->readings[i]) >> 3;
        if (magnitude == 0) continue;
        if (magnitude >= step_counter_threshold) {
            new_steps += 1;
            i += SIMPLE_SAMP_IGNORE_STEP;
        }
#if USE_WINDOW_AVG
        samples_processed += 1;
        samples_sum += magnitude;
#endif
    }
#if USE_WINDOW_AVG
    if (samples_processed > 0) {
        samples_sum /= samples_processed;
        samples_sum *= SIMPLE_THRESHOLD_MULT;
        step_counter_threshold = ((step_counter_threshold * AVG_WINDOW_SIZE) + samples_sum) >> AVG_WINDOW_SIZE_SHIFT;
    }
#endif
    if (new_steps > MAX_SIMPLE_STEPS) new_steps = MAX_SIMPLE_STEPS;
    return new_steps;
}

uint32_t get_steps_simple_threshold(void) {
    return step_counter_threshold;
}

// Espruino step counter algorithm
// Based on https://github.com/gfwilliams/step-count

#define ACCELFILTER_TAP_NUM 7

typedef struct {
    int8_t history[ACCELFILTER_TAP_NUM];
    unsigned int last_index;
} AccelFilter;

static const int8_t filter_taps[ACCELFILTER_TAP_NUM] = {
    -11, -15, 44, 68, 44, -15, -11
};

static void AccelFilter_init(AccelFilter* f) {
    for (int i = 0; i < ACCELFILTER_TAP_NUM; ++i)
        f->history[i] = 0;
    f->last_index = 0;
}

static void AccelFilter_put(AccelFilter* f, int8_t input) {
    f->history[f->last_index++] = input;
    if (f->last_index == ACCELFILTER_TAP_NUM)
        f->last_index = 0;
}

static int AccelFilter_get(AccelFilter* f) {
    int acc = 0;
    int index = f->last_index, i;
    for (i = 0; i < ACCELFILTER_TAP_NUM; ++i) {
        index = index != 0 ? index - 1 : ACCELFILTER_TAP_NUM - 1;
        acc += (int)f->history[index] * (int)filter_taps[i];
    }
    return acc >> 2;
}

static AccelFilter accelFilter;

#define NSAMPLE 12

static int DCFilter_sample_avg_total = 8192 * NSAMPLE;

static int DCFilter(int sample) {
    DCFilter_sample_avg_total += (sample - DCFilter_sample_avg_total / NSAMPLE);
    return sample - DCFilter_sample_avg_total / NSAMPLE;
}

#define STEPCOUNTERTHRESHOLD_DEFAULT  600
#define stepCounterThreshold STEPCOUNTERTHRESHOLD_DEFAULT

typedef enum {
    S_STILL = 0,
    S_STEP_1 = 1,
    S_STEP_22N = 2,
    S_STEPPING = 3,
} StepState;

#define T_MIN_STEP    4
#define T_MAX_STEP   16
#define X_STEPS       6
#define RAW_THRESHOLD 22
#define N_ACTIVE_SAMPLES 4

static StepState stepState;
static unsigned char holdSteps;
static unsigned char stepLength;
static int active_sample_count = 0;
static bool gate_open = false;

static int16_t accFiltered;
static int16_t accFilteredHist[2];

void count_steps_espruino_init(void) {
    AccelFilter_init(&accelFilter);
    DCFilter_sample_avg_total = 8192 * NSAMPLE;
    accFiltered = 0;
    accFilteredHist[0] = 0;
    accFilteredHist[1] = 0;
    stepState = S_STILL;
    holdSteps = 0;
    stepLength = 0;
    active_sample_count = 0;
    gate_open = false;
}

static int stepcount_had_step(void) {
    StepState st = stepState;
    switch (st) {
    case S_STILL:
        stepState = S_STEP_1;
        holdSteps = 1;
        return 0;
    case S_STEP_1:
        holdSteps = 1;
        if (stepLength <= T_MAX_STEP && stepLength >= T_MIN_STEP) {
            stepState = S_STEP_22N;
            holdSteps = 2;
        }
        return 0;
    case S_STEP_22N:
        if (stepLength <= T_MAX_STEP && stepLength >= T_MIN_STEP) {
            holdSteps++;
            if (holdSteps >= X_STEPS) {
                stepState = S_STEPPING;
                return X_STEPS;
            }
        } else {
            stepState = S_STEP_1;
        }
        return 0;
    case S_STEPPING:
        if (stepLength <= T_MAX_STEP && stepLength >= T_MIN_STEP) {
            stepState = S_STEPPING;
            return 1;
        } else {
            stepState = S_STEP_1;
        }
        return 0;
    }
    return 0;
}

uint8_t count_steps_espruino_sample(uint32_t accMag) {
    int v = DCFilter(accMag) >> 5;
    if (v > 127) v = 127;
    if (v < -128) v = -128;

    AccelFilter_put(&accelFilter, v);
    accFilteredHist[0] = accFilteredHist[1];
    accFilteredHist[1] = accFiltered;
    int a = AccelFilter_get(&accelFilter);
    if (a > 32767) a = 32767;
    if (a < -32768) a = -32768;
    accFiltered = a;

    if (v > RAW_THRESHOLD || v < -RAW_THRESHOLD) {
        if (active_sample_count < N_ACTIVE_SAMPLES) active_sample_count++;
        if (active_sample_count == N_ACTIVE_SAMPLES) gate_open = true;
    } else {
        if (active_sample_count > 0) active_sample_count--;
        if (active_sample_count == 0) gate_open = false;
    }

    if (stepLength < 255) stepLength++;

    int stepsCounted = 0;
    if (gate_open && accFilteredHist[1] > accFilteredHist[0] && accFilteredHist[1] > accFiltered) {
        stepsCounted = stepcount_had_step();
        stepLength = 0;
    }
    return stepsCounted;
}

uint8_t count_steps_espruino(lis2dw_fifo_t *fifo_data) {
    uint8_t new_steps = 0;
    for (uint8_t i = 0; i < fifo_data->count; i++) {
        uint32_t magnitude = count_steps_approx_l2_norm(fifo_data->readings[i]);
        if (magnitude == 0) continue;
        new_steps += count_steps_espruino_sample(magnitude << 1);
    }
    if (new_steps > MAX_SIMPLE_STEPS) new_steps = MAX_SIMPLE_STEPS;
    return new_steps;
}
