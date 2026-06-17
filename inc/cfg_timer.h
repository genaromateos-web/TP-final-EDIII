/***************************************************************************
 * @file        cfg_timer.h
 * @brief       It contains the prototypes of the functions that configure TIMER0
 *              and MATCH0.1, and the generic functions to enable/disable any timer.
 * @version     1.0
 * @date        30. May. 2026
 * @author      Genaro
 ***************************************************************************/

#ifndef CFG_TIMER_H_
#define CFG_TIMER_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "stdint.h"

/* ------------------------------ Public Types ------------------------------ */
// Nothing here...

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  Configures TIMER0 with 1µs resolution and sets up MATCH channel 1.
 *
 * Configures MATCH1 to reset the counter and toggle the output on each match,
 * without generating an interrupt or stopping the timer.
 *
 * @param  matValue  Match value in microseconds.
 * @note   The timer is disabled after configuration. Call TIM_On(LPC_TIM0)
 *         to start it when needed.
 */
void cfgTimer0(uint32_t matValue); // matValue in µs. Directly determines the ADC sampling frequency.

void cfgTimer1(uint32_t matValue);

/**
 * @brief  Enables the peripheral clock of the given timer and starts it.
 *
 * @param  TIMx  Pointer to the timer peripheral to turn on.
 *               Valid values: LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3.
 */
void TIM_On(LPC_TIM_TypeDef *TIMx);

/**
 * @brief  Stops the given timer and disables its peripheral clock.
 *
 * @param  TIMx  Pointer to the timer peripheral to turn off.
 *               Valid values: LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3.
 */
void TIM_Off(LPC_TIM_TypeDef *TIMx);

#endif /* CFG_TIMER_H_ */

/* ------------------------------ End Of File ------------------------------- */
