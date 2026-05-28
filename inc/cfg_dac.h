/***************************************************************************
 * @file 		cfg_dac.h
 * @brief		Contains all enum definitions and function prototypes to
 * 				configure the DAC
 * @version     1.0
 * @date        26 may 2026
 * @author      Genaro
 ***************************************************************************/

#ifndef CFG_DAC_H_
#define CFG_DAC_H_

/* -------------------------------- Includes -------------------------------- */
#include "lpc_types.h"
#include "lpc17xx_dac.h"


/* ------------------------------ Public Types ------------------------------ */

/**
 * @brief Frequency division for the periphic
 */
typedef enum{
	DIV_4 = 0,
	DIV_1 = 1,
	DIV_2 = 2

}CLKPWR_PCLKSEL_CCLK_DIV;

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  	Initializes the DAC peripheral and its associated pin.
 *
 * @param  	maxCurr Power mode selection (DAC_700uA or DAC_350uA).
 * @param  	cclkDiv Selection of frequency division for the periphic (DIV_x)
 *
 * @note:
 * - The DAC pin is configured for analog output.
 */
void cfgDac(DAC_MAX_CURRENT maxCurr,CLKPWR_PCLKSEL_CCLK_DIV cclkDiv);

/**
 * @brief	Set and enable the timer for DAC dma request
 *
 * @param  timeOut 16-bit timeout value in PCLK cycles.
 * @param  doubBuffer Enabling double buffer
 *
 * @note
 * -The DMA transfer request is enabled
 */
void cfgDmaCounterEnable(uint16_t timeOut, FunctionalState doubBuffer);

#endif /* CFG_DAC_H_ */

/* ------------------------------ End Of File ------------------------------- */
