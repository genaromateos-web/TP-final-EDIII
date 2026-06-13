/***************************************************************************
 * @file            waveform_gen.h
 * @brief           Public API for the DDS (Direct Digital Synthesis) waveform
 *                  generator. Declares types, constants, global variables, and
 *                  function prototypes for generating and modifying sine,
 *                  square, and triangle waveforms.
 * @version         2.0
 * @date            25. May. 2026
 * @author          Signal Labs (Mateos & Flores)
 ***************************************************************************/

#ifndef WAVEFORM_GEN_H_
#define WAVEFORM_GEN_H_

/* -------------------------------- Includes -------------------------------- */
#include "stdint.h"

/* ------------------------------ Public Types ------------------------------ */

/**
 * @brief Number of samples in one full wave cycle stored in the lookup table.
 *        A power of two is chosen so that the DDS phase-to-index mapping
 *        (upper 8 bits of a 32-bit accumulator) is exact with no division.
 */
#define TABLE_SIZE 256

/**
 * @brief Number of samples written to the DAC output buffer per fillBuffer() call.
 *        Must be a divisor of TABLE_SIZE to avoid phase discontinuities when
 *        the buffer boundary falls mid-cycle.
 */
#define BUFFER_SIZE 64

/**
 * @brief Pi constant used for sine wave angle calculation (radians).
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
    WAVE_SQUARE,  /**< Square waveform: first half at maximum (1023), second half at minimum (0). */
    WAVE_TRIANGLE /**< Triangle waveform: linear ramp up from 0 to 1023, then back down to 0. */
} WaveType_t;

/**
 * @brief Holds all parameters that define a single DDS waveform channel.
 *
 * One instance per waveform shape is kept in memory. The active channel is
 * selected by pointing @c currentWave at the desired instance.
 */
typedef struct {
    uint16_t amplitude; /**< Output amplitude scale factor (0–1024). 1024 = full scale (3.3 V). */
    uint16_t frequency; /**< Output frequency in Hz (1–9999 Hz). */
    uint16_t *table;    /**< Pointer to the lookup table holding one full cycle (TABLE_SIZE samples, 10-bit values). */
    uint32_t
        phaseStep; /**< Phase increment added to the accumulator on every sample; computed by calculatePhaseStep(). */
    uint32_t phaseAccumulator; /**< Running phase state. Advances by phaseStep each sample; intentional uint32_t
                                  overflow provides seamless wraparound at the end of each cycle. */
    WaveType_t type;           /**< Waveform shape (WAVE_SINE, WAVE_SQUARE, or WAVE_TRIANGLE). */
} WaveGen_t;

/* ------------------------------ Public Variables -------------------------- */

/** Pointer to the waveform channel currently being output by fillBuffer(). */
extern volatile WaveGen_t *currentWave;

/** Lookup tables: one full cycle of each waveform, 10-bit samples (0–1023). */
extern uint16_t sineTable[TABLE_SIZE];
extern uint16_t squareTable[TABLE_SIZE];
extern uint16_t triangleTable[TABLE_SIZE];

/** WaveGen instances for each supported waveform shape. */
extern WaveGen_t sine;
extern WaveGen_t square;
extern WaveGen_t triangle;

/** Buffers Ping-Pong. */
extern uint16_t bufferA[BUFFER_SIZE];
extern uint16_t bufferB[BUFFER_SIZE];

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
 * @param  table_mem  Pointer to an external uint16_t array of at least TABLE_SIZE
 *                    elements that will be used as the lookup table.
 */
void initWaveTable(WaveGen_t *wave, WaveType_t type, uint16_t *table_mem);

/**
 * @brief  Computes the DDS phase step for a given output frequency.
 *
 * Uses the Direct Digital Synthesis formula: step = (f_wave × 2^32) / f_sample.
 * The precomputed constant 4294.967296 = 2^32 / f_sample avoids a run-time
 * division. Accumulating the returned value in a uint32_t causes a natural
 * overflow that is equivalent to completing exactly f_wave full table traversals
 * per second, producing the desired output frequency.
 *
 * @param  fWave  Desired output frequency in Hz. Must be in the range [1, 9999].
 *                Values outside this range cause the function to enter an
 *                infinite loop (hard fault indicator).
 * @return        Phase step value to store in WaveGen_t.phaseStep.
 */
uint32_t calculatePhaseStep(uint16_t fWave);

/**
 * @brief  Fills the output buffer with amplitude-scaled, DAC-ready samples.
 *
 * For each of the BUFFER_SIZE output samples the function:
 *   1. Advances the 32-bit phase accumulator by phaseStep. Overflow is
 *      intentional and provides seamless phase continuity across calls.
 *   2. Maps the upper 8 bits of the accumulator to a lookup-table index
 *      (0–255, matching TABLE_SIZE = 256).
 *   3. Centers the raw 10-bit table sample around zero by subtracting
 *      the midpoint (511), yielding the range [-511, 512].
 *   4. Scales the centered sample by amplitude/1024 using an arithmetic
 *      right-shift of 10 bits (integer equivalent of division by 1024).
 *   5. Restores the DAC mid-scale offset (+511) to return the value to
 *      the 10-bit range [0, 1023].
 *   6. Writes the result to the output buffer. The caller is responsible
 *      for applying the LPC1769 DAC register left-shift of 6 bits if needed.
 *
 * @param  buffer  Output buffer where DAC samples will be written.
 *                 Must hold at least BUFFER_SIZE uint16_t elements.
 * @param  wave    WaveGen_t instance providing the lookup table, phase
 *                 accumulator, phase step, and amplitude.
 *                 wave->amplitude must not exceed 1024; values above this
 *                 limit cause the function to enter an infinite loop.
 */
void fillBuffer(uint16_t *buffer, WaveGen_t *wave);

/**
 * @brief  Updates the generator frequency from a raw ADC reading.
 *
 * Discards the 2 noisiest LSBs of the 12-bit ADC value, then linearly maps
 * the resulting 10-bit value (0–1023) to the frequency range [1 Hz, 1000 Hz]:
 *   freq = 1 + (adc10 × 999) / 1023
 * The phase step is recalculated only when the mapped frequency changes,
 * avoiding unnecessary floating-point operations.
 *
 * Intended to be called once per ADC conversion complete interrupt (every 500 ms).
 *
 * @param  wave     Pointer to the WaveGen_t instance to update (modified in place).
 * @param  adcData  Raw 12-bit ADC register value (0–4095).
 */
void waveUpdateFrequency(WaveGen_t *wave, uint16_t adcData);

#endif /* WAVEFORM_GEN_H_ */

/* ------------------------------ End Of File ------------------------------- */
