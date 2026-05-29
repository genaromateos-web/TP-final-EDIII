/***************************************************************************
* @file        	waveform_gen.c
 * @brief       It contains the definition of the functions that generate and
 * 				modify waves .
 * @version     1.0
 * @date        25. May. 2026
 * @author      Genaro
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "waveform_gen.h"
#include <math.h>

/* ---------------------------- Public Functions ---------------------------- */
void initSineTable(uint16_t *table){
	float theta, seno;

    for(uint16_t i = 0; i < TABLE_SIZE; i++){
    	// Angle between 0 and 2π, evenly distributed across TABLE_SIZE steps
    	theta = (2.0f * PI * i) / TABLE_SIZE;
    	// Sine of theta, result is in range [-1.0, 1.0]
    	seno = sinf(theta);
        // Shift sine range from [-1, 1] to [0, 1023] for the 10-bit DAC
    	table[i] = (uint16_t) ((seno + 1.0f) * 511.5f);
    }
}

uint32_t calculatePhaseStep(uint16_t fWave){
    // Phase step = (f_wave * 2^32) / f_sample
    // The constant 4294.9672 = 2^32 / f_sample, precomputed for efficiency
    return fWave * 4294.9672f;
}

void fillBuffer(const uint16_t *table, uint16_t *buffer, WaveGen_t *waveGen){
	uint16_t indexTable = 0, centered, scaled, sample;
	//chech parametro. si falla se queda en bucle infinito
	if(waveGen->amplitude > 1024) while(1);

	for (int i = 0; i < BUFFER_SIZE; i++){
		// Accumulate phase — wraps around automatically on uint32_t overflow
		waveGen->phaseAccumulator += waveGen->phaseStep;
		// Use the upper 8 bits as the table index (maps 2^32 range to TABLE_SIZE = 256)
		indexTable = waveGen->phaseAccumulator >> 24;
		// Se lleva la muestra al centro, punto medio = 511
		centered = (int16_t)table[indexTable] - 511;
		// >> 10 equivale a dividir por 1024
		scaled   = (int16_t)(((int32_t)centered * waveGen->amplitude) >> 10);
		// Se devuelve la muestra al rango 0-1023 sumando el punto medio
		sample  = (uint16_t)(scaled + 511);
		// The LPC1769 DAC expects the 10-bit value left-shifted by 6 bits
		buffer[i] = sample << 6;
	}
}

/* ------------------------------ End Of File ------------------------------- */
