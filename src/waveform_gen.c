/***************************************************************************
 * @file        	waveform_gen.c
 * @brief       It contains the definition of the functions that generate and
 * 				modify waves.
 * @version     2.0
 * @date        25. May. 2026
 * @author      Genaro
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "waveform_gen.h"
#include <math.h>

/* ---------------------------- Public Functions ---------------------------- */

void initWaveTable(WaveGen_t *wave, WaveType_t type, uint16_t *table_mem) {
    // Set default initialization values
    wave->amplitude = defaultAmplitude;
    wave->frequency = defaultFrequency;
    wave->table = table_mem;
    wave->phaseStep = calculatePhaseStep(wave->frequency);
    wave->phaseAccumulator = 0;
    wave->type = type;

    // Fill the lookup table with one full wave cycle depending on the waveform type
    switch (type) {

    case WAVE_SINE: {
        float theta, seno;

        for (uint16_t i = 0; i < TABLE_SIZE; i++) {
            // Angle between 0 and 2pi, evenly distributed across TABLE_SIZE steps
            theta = (2.0f * PI * i) / TABLE_SIZE;
            // Sine of theta, result is in range [-1.0, 1.0]
            seno = sinf(theta);
            // Map [-1, 1] to [0, 1023] for the 10-bit DAC:
            // add 1 to shift to [0, 2], then multiply by 511.5
            wave->table[i] = (uint16_t)((seno + 1.0f) * 511.5f);
        }
        break;
    }

    case WAVE_SQUARE: {
        // First half of the table at maximum value (high level of the square wave)
        for (int i = 0; i < TABLE_SIZE / 2; i++) {
            wave->table[i] = 1023;
        }
        // Second half of the table at minimum value (low level of the square wave)
        for (int i = TABLE_SIZE / 2; i < TABLE_SIZE; i++) {
            wave->table[i] = 0;
        }
        break;
    }

    case WAVE_TRIANGLE: {
        // First half: linear ascending ramp from 0 to 1023
        for (int i = 0; i < TABLE_SIZE / 2; i++) {
            wave->table[i] = (uint16_t)((i * 1023) / (TABLE_SIZE / 2 - 1));
        }
        // Second half: linear descending ramp from 1023 to 0
        for (int i = TABLE_SIZE / 2; i < TABLE_SIZE; i++) {
            wave->table[i] = (uint16_t)(((TABLE_SIZE - 1 - i) * 1023) / (TABLE_SIZE / 2 - 1));
        }
        break;
    }
    }
}

uint32_t calculatePhaseStep(uint16_t fWave) {
    if (fWave < 1 || fWave > 9999) {
        while (1) {} // Error: frequency out of allowed range (1Hz–9999Hz)
    }
    // DDS phase step formula: step = (f_wave * 2^32) / f_sample
    // The constant 4294.9672 = 2^32 / f_sample is precomputed for efficiency.
    // Accumulating this value in a uint32_t, the natural overflow is equivalent
    // to traversing the full lookup table exactly f_wave times per second.
    return fWave * 4294.9672f;
}

void fillBuffer(uint16_t *buffer, WaveGen_t *wave) {
    uint16_t indexTable = 0, sample;
    int16_t centered, scaled;

    if (wave->amplitude > 1024) {
        while (1) {} // Error: amplitude out of allowed range (maximum 1024)
    }

    for (int i = 0; i < BUFFER_SIZE; i++) {
        // Advance the phase accumulator; uint32_t overflow is intentional and
        // ensures phase continuity across successive calls to fillBuffer()
        wave->phaseAccumulator += wave->phaseStep;

        // Use the upper 8 bits as the table index (maps [0, 2^32) to [0, TABLE_SIZE = 256))
        indexTable = wave->phaseAccumulator >> 24;

        // Center the sample by subtracting the 10-bit midpoint (511),
        // shifting the range from [0, 1023] to [-511, 512]
        centered = (int16_t)wave->table[indexTable] - 511;

        // Scale the centered sample by (amplitude / 1024):
        // >> 10 is equivalent to dividing by 1024, avoiding floating-point arithmetic
        scaled = (int16_t)(((int32_t)centered * wave->amplitude) >> 10);

        // Restore the midpoint offset (511) to bring the sample back to [0, 1023]
        sample = (uint16_t)(scaled + 511);

        // The LPC1769 DAC expects the 10-bit value left-shifted by 6 bits
        buffer[i] = sample << 6;
    }
}

void waveUpdateFrequency(WaveGen_t *wave, uint16_t adcData) {
    /* 1. Quedarse con los 10 MSBs descartando los 2 LSBs ruidosos */
    uint16_t adc10 = adcData >> 2; /* Rango resultante: 0 – 1023 */

    /* 2. Mapeo lineal [0, 1023] ? [1 Hz, 1000 Hz]
     *    freq = 1 + (adc10 × 999) / 1023
     *    Cast a uint32_t obligatorio: 1023 × 999 = 1.021.977 > UINT16_MAX  */
    uint32_t newFreq = 1UL + ((uint32_t)adc10 * 999UL) / 1023UL;

    /* 3. Recalcular solo si el valor cambió (evita FP innecesario) */
    if (newFreq == wave->frequency) {
        return;
    }

    wave->frequency = newFreq;
    wave->phaseStep = calculatePhaseStep(newFreq);
}

/* ------------------------------ End Of File ------------------------------- */
