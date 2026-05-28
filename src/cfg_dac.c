/***************************************************************************
* @file        	cfg_dac.c
 * @brief       Contains the definitions of the functions to configure the DAC
 * @version     1.0
 * @date        26 may 2026
 * @author      Genaro
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_dac.h"
#include "lpc17xx_clkpwr.h"

/* ---------------------------- Public Functions ---------------------------- */
void cfgDac(DAC_MAX_CURRENT maxCurr,CLKPWR_PCLKSEL_CCLK_DIV cclkDiv){
	LPC_PINCON->PINSEL1  = (LPC_PINCON->PINSEL1 & ~(0x3 << 20)) | (0x2 << 20);
	LPC_PINCON->PINMODE1 = (LPC_PINCON->PINMODE1 & ~(0x3 << 20)) | (0x2 << 20);

	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_DAC, (uint32_t)cclkDiv);

	DAC_SetBias(maxCurr);
}

void cfgDmaCounterEnable(uint16_t timeOut, FunctionalState doubBuffer){
	DAC_SetDMATimeOut(timeOut);

	DAC_CONVERTER_CFG_T dacCfg;

	dacCfg.dmaCounter = ENABLE;
	dacCfg.dmaRequest = ENABLE;
	dacCfg.doubleBuffer = doubBuffer;

	DAC_ConfigDAConverterControl(&dacCfg);
}

/* ------------------------------ End Of File ------------------------------- */
