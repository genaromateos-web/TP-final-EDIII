/***************************************************************************
 * @file        cfg_timer.c
 * @brief       It contains the definition of the functions that configure TIMER0
 *              and MATCH0.1, and the generic functions to enable/disable any timer.
 * @version     1.0
 * @date        30. May. 2026
 * @author      Genaro
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_timer.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_timer.h"
#include "lpc_types.h"
#include "main.h"
#include "../../MCUXpressoIDE_25.6.136/workspace/cfg_display.h"

/* ---------------------------- Public Functions ---------------------------- */

void cfgTimer0(uint32_t matValue) {
    TIM_TIMERCFG_T timerCfg;
    timerCfg.prescaleOpt = TIM_US; // Prescaler unit: microseconds
    timerCfg.prescaleValue = 1;    // T_TC = 1µs -> counter increments every 1µs

    // Initializes TIMER0 with the prescaler configuration defined above
    TIM_InitTimer(LPC_TIM0, &timerCfg);

    TIM_MATCHCFG_T matchCfg;
    matchCfg.channel = TIM_MATCH_1; // Comparison channel: MATCH1
    matchCfg.intEn = DISABLE;       // No interrupt on match
    matchCfg.stopEn = DISABLE;      // Timer does not stop on match
    matchCfg.resetEn = ENABLE;      // Counter resets to 0 on match
    matchCfg.extOpt = TIM_TOGGLE;   // MAT0.1 output toggles on each match
    matchCfg.matchValue = matValue; // Match value in µs (e.g. 250000 ? 250ms)

    // A match occurs every matValue µs, toggling the output each time.
    // The ADC samples once per full toggle cycle, i.e. every 2 × matValue µs.
    // Example: matValue = 250000 µs -> toggle every 250ms -> ADC samples every 500ms.
    // The counter resets on each match, keeping the period constant.
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    // Timer is left disabled after configuration; turned on only when needed
    TIM_Off(LPC_TIM0);
}

void cfgTimer1(uint32_t matValue){
	TIM_TIMERCFG_T timerCfg;
	timerCfg.prescaleOpt   = TIM_US;
	timerCfg.prescaleValue = 1;   /* 1 µs por tick */
	TIM_InitTimer(LPC_TIM1, &timerCfg);

	TIM_MATCHCFG_T matchCfg;
	matchCfg.channel    = TIM_MATCH_0;
	matchCfg.intEn      = ENABLE;
	matchCfg.stopEn     = DISABLE;
	matchCfg.resetEn    = ENABLE;
	matchCfg.extOpt     = TIM_NOTHING;
	matchCfg.matchValue = matValue;
	TIM_ConfigMatch(LPC_TIM1, &matchCfg);

	/* Prioridad debajo de DMA y ADC */
	NVIC_SetPriority(TIMER1_IRQn, 2);
	NVIC_ClearPendingIRQ(TIMER1_IRQn);
	NVIC_EnableIRQ(TIMER1_IRQn);

	TIM_Enable(LPC_TIM1);
}

void TIM_On(LPC_TIM_TypeDef *TIMx) {
    // Enable the peripheral clock for the corresponding timer in the PCONP register
    if (TIMx == LPC_TIM0) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM0, ENABLE);
    } else if (TIMx == LPC_TIM1) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM1, ENABLE);
    } else if (TIMx == LPC_TIM2) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM2, ENABLE);
    } else if (TIMx == LPC_TIM3) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM3, ENABLE);
    }

    // Start the timer counter
    TIM_Enable(TIMx);
}

void TIM_Off(LPC_TIM_TypeDef *TIMx) {
    // Stop the counter before cutting the clock to avoid undefined behavior
    TIM_Disable(TIMx);

    // Disable the peripheral clock for the corresponding timer in the PCONP register
    if (TIMx == LPC_TIM0) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM0, DISABLE);
    } else if (TIMx == LPC_TIM1) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM1, DISABLE);
    } else if (TIMx == LPC_TIM2) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM2, DISABLE);
    } else if (TIMx == LPC_TIM3) {
        CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCTIM3, DISABLE);
    }
}

/* ------------------------- Interrupt Handler ------------------------- */
void TIMER1_IRQHandler(void) {
    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    // Levanto flag
    waveControl |= bitMask(4);
}

/* ------------------------------ End Of File ------------------------------- */
