/***************************************************************************
 * @file        	waveform_gen.h
 * @brief       It contains the prototypes of the functions that generate
 * 				and modify waves.
 * @version     2.0
 * @date        25. May. 2026
 * @author      Genaro
 ***************************************************************************/

#ifndef WAVEFORM_GEN_H_
#define WAVEFORM_GEN_H_

/* -------------------------------- Includes -------------------------------- */
#include "stdint.h"

/* ------------------------------ Public Types ------------------------------ */

/**
 * @brief Number of samples in one full wave cycle stored in the lookup table.
 */
#define TABLE_SIZE 256

/**
 * @brief Number of samples written to the DAC output buffer per fill call.
 */
#define BUFFER_SIZE 64

/**
 * @brief Pi constant used for sine wave angle calculation.
 */
#define PI 3.14159265f

/**
 * @brief Default output frequency assigned on initialization (Hz).
 */
#define defaultFrequency 60

/**
 * @brief Default amplitude assigned on initialization.
 *        A value of 1024 represents full scale (3.3 V output swing).
 *        Valid range: 0 – 1024.
 */
#define defaultAmplitude 1024

/**
 * @brief Selects the waveform shape stored in the lookup table.
 */
typedef enum {
    WAVE_SINE,    /**< Sinusoidal waveform. */
    WAVE_SQUARE,  /**< Square waveform: first half at maximum, second half at minimum. */
    WAVE_TRIANGLE /**< Triangle waveform: linear ramp up then linear ramp down. */
} WaveType_t;

/**
 * @brief Holds all parameters that define a single DDS waveform channel.
 */
typedef struct {
    uint16_t amplitude; /**< Output amplitude (0–1024). 1024 = full scale (3.3 V). */
    uint16_t frequency; /**< Output frequency in Hz. (1Hz–9999Hz) */
    uint16_t *table;    /**< Pointer to the lookup table that stores one full wave cycle (TABLE_SIZE samples). */
    uint32_t phaseStep; /**< Phase increment per sample, computed by calculatePhaseStep(). */
    uint32_t
        phaseAccumulator; /**< Persistent accumulator that keeps phase state between successive fillBuffer() calls. */
    WaveType_t type;      /**< Waveform shape (sine, square or triangle). */
} WaveGen_t;

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  Initializes a WaveGen_t instance and pre-fills its lookup table.
 *
 * Sets amplitude and frequency to their default values, links the external
 * memory block as the lookup table, resets the phase accumulator to zero,
 * and populates the table with one full cycle of the requested waveform:
 *   - WAVE_SINE:     samples follow sin(?) mapped to the 10-bit range [0, 1023].
 *   - WAVE_SQUARE:   first half of table = 1023, second half = 0.
 *   - WAVE_TRIANGLE: linearly rises from 0 to 1023, then falls back to 0.
 *
 * @param  wave       Pointer to the WaveGen_t structure to initialize.
 * @param  type       Desired waveform shape (WAVE_SINE, WAVE_SQUARE or WAVE_TRIANGLE).
 * @param  table_mem  Pointer to an external uint16_t array of at least TABLE_SIZE elements
 *                    that will be used as the lookup table.
 */
void initWaveTable(WaveGen_t *wave, WaveType_t type, uint16_t *table_mem);

/**
 * @brief  Computes the DDS phase step for a given output frequency.
 *
 * Uses the Direct Digital Synthesis formula: step = (f_wave * 2^32) / f_sample.
 * The constant 4294.9672 = 2^32 / f_sample is precomputed for efficiency.
 * The result is used by fillBuffer() to advance through the lookup table
 * at the correct rate to produce the desired output frequency.
 *
 * @param  fWave  Desired output wave frequency in Hz. It must be less than 9999 Hz
 * and greater than 1 Hz. If this range is not respected, the function enters an
 * infinite loop.
 * @return        Phase step value to be stored in WaveGen_t.phaseStep.
 */
uint32_t calculatePhaseStep(uint16_t fWave);

/**
 * @brief  Fills the output buffer with amplitude-scaled, DAC-ready samples.
 *
 * For each of the BUFFER_SIZE output samples the function:
 *   1. Advances the 32-bit phase accumulator by phaseStep (wraps on overflow,
 *      providing seamless phase continuity across successive calls).
 *   2. Maps the upper 8 bits of the accumulator to a lookup-table index
 *      (range 0–255, matching TABLE_SIZE = 256).
 *   3. Centers the raw table sample around zero (subtracts the midpoint 511).
 *   4. Scales the centered sample by amplitude/1024 using an arithmetic
 *      right-shift of 10 bits (equivalent to integer division by 1024).
 *   5. Restores the DAC mid-scale offset (adds 511) to keep the output
 *      in the 10-bit range [0, 1023].
 *   6. Left-shifts the result by 6 bits to match the LPC1769 DAC register format.
 *
 * @param  buffer  Pointer to the output buffer where DAC samples will be written
 *                 (must hold at least BUFFER_SIZE uint16_t elements).
 * @param  wave    Pointer to the WaveGen_t instance that provides the lookup table,
 *                 phase accumulator, phase step, and amplitude.
 *                 wave->amplitude must be in the range 0–1024; the function enters
 *                 an infinite loop if this limit is exceeded.
 */
void fillBuffer(uint16_t *buffer, WaveGen_t *wave);

/**
 * @brief  Actualiza la frecuencia del generador a partir de una lectura ADC.
 *
 * Descarta los 2 LSBs del valor crudo de 12 bits (reducción de ruido), mapea
 * linealmente los 10 bits restantes al rango [1 Hz, 1000 Hz] y recalcula el
 * paso de fase DDS solo si la frecuencia resultante cambió.
 *
 * Debe llamarse cada vez que el ADC complete una conversión (cada 500 ms).
 *
 * @param waveGen   Puntero al struct del generador (se modifica en el lugar).
 * @param adcRaw    Valor crudo de 12 bits leído del registro del ADC (0–4095).
 */
void waveUpdateFrequency(WaveGen_t *wave, uint16_t adcData);

#endif /* WAVEFORM_GEN_H_ */

/* ------------------------------ End Of File ------------------------------- */
