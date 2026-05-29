/***************************************************************************
* @file        	waveform_gen.h
 * @brief       It contains the prototypes of the functions that generate
 * 				and modify waves.
 * @version     1.0
 * @date        25. May. 2026
 * @author      Genaro
 ***************************************************************************/

#ifndef WAVEFORM_GEN_H_
#define WAVEFORM_GEN_H_

/* -------------------------------- Includes -------------------------------- */
#include "stdint.h"


/* ------------------------------ Public Types ------------------------------ */

/**
 * @brief Number of samples in one full sine wave cycle stored in the lookup table.
 */
#define TABLE_SIZE 256

/**
 * @brief Number of samples written to the DAC output buffer per fill call.
 */
#define BUFFER_SIZE 64

/**
 * @brief Pi constant used for sine wave angle calculation.
 */
#define PI 3.1415
//Almacena los parametros de la onda
typedef struct {
	uint16_t frequency;
	uint16_t amplitude;
    uint32_t phaseStep;
    uint32_t phaseAccumulator; // Persistent accumulator that keeps phase state between calls
} WaveGen_t;

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  Precomputes a full sine wave cycle and stores it as 10-bit DAC values.
 *
 * Fills the lookup table with TABLE_SIZE evenly spaced samples of a sine wave.
 * Each sample is scaled and shifted from the [-1, 1] range to [0, 1023],
 * making it ready for direct use with a 10-bit DAC.
 *
 * @param  table  Pointer to a uint16_t array of at least TABLE_SIZE elements
 *                where the computed samples will be stored.
 * @note   Must be called once before any call to fillBuffer().
 */
void initSineTable(uint16_t *table);

/**
 * @brief  Computes the DDS phase step for a given output frequency.
 *
 * Uses the Direct Digital Synthesis formula: step = (f_wave * 2^32) / f_sample.
 * The result is used by fillBuffer() to advance through the sine lookup table
 * at the correct rate to produce the desired output frequency.
 *
 * @param  fWave  Desired output wave frequency in Hz.
 * @return        Phase step value to be passed to fillBuffer().
 */
uint32_t calculatePhaseStep(uint16_t fWave);

/**
 * @brief  Fills the output buffer with DAC-ready samples from the sine lookup table.
 *
 * Uses a static phase accumulator to advance through the lookup table by phaseStep
 * on each sample. The accumulator wraps around naturally on uint32_t overflow,
 * providing continuous phase-coherent output across successive calls.
 * Each sample is left-shifted by 6 bits to match the LPC1769 DAC input format.
 *
 * @param  table      Pointer to the precomputed sine lookup table (TABLE_SIZE elements).
 * @param  buffer     Pointer to the output buffer where DAC samples will be written
 *                    (must hold at least BUFFER_SIZE elements).
 * @param  phaseStep  Phase increment per sample, obtained from calculatePhaseStep().
 * @note   The phase accumulator is static, so frequency changes take effect
 *         immediately but phase continuity is preserved between calls.
 */
void fillBuffer(const uint16_t *table, uint16_t *buffer, WaveGen_t *waveGen);

#endif /* WAVEFORM_GEN_H_ */

/* ------------------------------ End Of File ------------------------------- */
