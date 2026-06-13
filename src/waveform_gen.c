/***************************************************************************
 * @file            waveform_gen.c
 * @brief           Implementation of the DDS waveform generator.
 *                  Provides lookup-table initialization, phase-step
 *                  calculation, DAC buffer filling, and ADC-driven frequency
 *                  updates for sine, square, and triangle waveforms.
 * @version         2.0
 * @date            25. May. 2026
 * @author          Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "waveform_gen.h"
#include <math.h>

/* ---------------------------- Public Variables ---------------------------- */

/** Active waveform channel; defaults to sine on startup. */
volatile WaveGen_t *currentWave = &sine;

/** Lookup tables: one full cycle per waveform (TABLE_SIZE × 10-bit samples). */
uint16_t sineTable[TABLE_SIZE];
uint16_t squareTable[TABLE_SIZE];
uint16_t triangleTable[TABLE_SIZE];

/** WaveGen parameter structs for each waveform shape. */
WaveGen_t sine;
WaveGen_t square;
WaveGen_t triangle;

/** Buffers Ping-Pong. */
uint16_t bufferA[BUFFER_SIZE];
uint16_t bufferB[BUFFER_SIZE];

/* ---------------------------- Public Functions ---------------------------- */

void initWaveTable(WaveGen_t *wave, WaveType_t type, uint16_t *table_mem) {
    /* Set default initialization values */
    wave->amplitude = defaultAmplitude; /* 1024 = full scale             */
    wave->frequency = defaultFrequency; /* 60 Hz                         */
    wave->table = table_mem;
    wave->phaseStep = calculatePhaseStep(wave->frequency);
    wave->phaseAccumulator = 0;
    wave->type = type;

    /* Populate the lookup table with one complete waveform cycle */
    switch (type) {

    case WAVE_SINE: {
        float theta, seno;

        for (uint16_t i = 0; i < TABLE_SIZE; i++) {
            /* Evenly distribute TABLE_SIZE angles across [0, 2?) */
            theta = (2.0f * PI * i) / TABLE_SIZE;

            /* sin() returns [-1.0, 1.0] */
            seno = sinf(theta);

            /* Map [-1, 1] ? [0, 1023]: shift to [0, 2] then scale by 511.5 */
            wave->table[i] = (uint16_t)((seno + 1.0f) * 511.5f);
        }
        break;
    }

    case WAVE_SQUARE: {
        /* First half: logic high (maximum DAC value) */
        for (int i = 0; i < TABLE_SIZE / 2; i++) {
            wave->table[i] = 1023;
        }
        /* Second half: logic low (minimum DAC value) */
        for (int i = TABLE_SIZE / 2; i < TABLE_SIZE; i++) {
            wave->table[i] = 0;
        }
        break;
    }

    case WAVE_TRIANGLE: {
        /* First half: ascending linear ramp from 0 to 1023 */
        for (int i = 0; i < TABLE_SIZE / 2; i++) {
            wave->table[i] = (uint16_t)((i * 1023) / (TABLE_SIZE / 2 - 1));
        }
        /* Second half: descending linear ramp from 1023 back to 0 */
        for (int i = TABLE_SIZE / 2; i < TABLE_SIZE; i++) {
            wave->table[i] = (uint16_t)(((TABLE_SIZE - 1 - i) * 1023) / (TABLE_SIZE / 2 - 1));
        }
        break;
    }
    }
}

uint32_t calculatePhaseStep(uint16_t fWave) {
    if (fWave < 1 || fWave > 9999) {
        while (1) {} /* Hard fault: frequency out of allowed range (1–9999 Hz) */
    }

    /* DDS formula: step = (f_wave × 2^32) / f_sample
     * 4294.967296 = 2^32 / f_sample is precomputed to avoid run-time division.
     * uint32_t overflow during accumulation is equivalent to wrapping the full
     * table, so the output completes exactly f_wave cycles per second.         */
    return (uint32_t)(fWave * 4294.967296f);
}

void fillBuffer(uint16_t *buffer, WaveGen_t *wave) {
    /* Cache wave parameters in locals to reduce pointer dereferences per sample */
    const uint32_t lphaseStep = wave->phaseStep;
    const uint16_t lAmplitude = wave->amplitude;
    const uint16_t *lTable = wave->table;

    uint16_t indexTable;
    uint16_t sample;
    int16_t centered, scaled;

    if (wave->amplitude > 1024) {
        while (1) {} /* Hard fault: amplitude exceeds maximum (1024) */
    }

    for (int i = 0; i < BUFFER_SIZE; i++) {
        /* Advance the phase accumulator; uint32_t overflow is intentional and
         * ensures seamless phase continuity across successive fillBuffer() calls */
        wave->phaseAccumulator += lphaseStep;

        /* Upper 8 bits select the table index, mapping [0, 2^32) ? [0, 255] */
        indexTable = wave->phaseAccumulator >> 24;

        /* Subtract the 10-bit midpoint (511) to center the sample around zero,
         * shifting the range from [0, 1023] to [-511, 512]                     */
        centered = (int16_t)lTable[indexTable] - 511;

        /* Scale by amplitude/1024 using a right-shift (avoids floating-point).
         * Cast to int32_t before the multiply to prevent 16-bit overflow.      */
        scaled = (int16_t)(((int32_t)centered * lAmplitude) >> 10);

        /* Re-apply the midpoint offset to restore the 10-bit DAC range [0, 1023] */
        sample = (uint16_t)(scaled + 511);

        /* The LPC1769 DAC expects the 10-bit value left-shifted by 6 bits
         * into the DACR register VALUE field                                    */
        buffer[i] = sample;
    }
}

void waveUpdateFrequency(WaveGen_t *wave, uint16_t adcData) {
    /* Drop the 2 noisiest LSBs, reducing the 12-bit raw value to 10 bits (0–1023) */
    uint16_t adc10 = adcData >> 2;

    /* Linear map: [0, 1023] ? [1 Hz, 1000 Hz]
     * freq = 1 + (adc10 × 999) / 1023
     * Intermediate product 1023 × 999 = 1,021,977 exceeds UINT16_MAX,
     * so a uint32_t cast is required before the multiplication.               */
    uint32_t newFreq = 1UL + ((uint32_t)adc10 * 999UL) / 1023UL;

    /* Skip the phase-step recalculation if the frequency has not changed,
     * avoiding an unnecessary floating-point multiply                         */
    if (newFreq == wave->frequency) {
        return;
    }

    wave->frequency = newFreq;
    wave->phaseStep = calculatePhaseStep(newFreq);
}

/* ------------------------------ End Of File ------------------------------- */
