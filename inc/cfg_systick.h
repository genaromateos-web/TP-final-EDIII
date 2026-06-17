/***************************************************************************
 * @file        cfg_systick.h
 * @brief       Public API for the SysTick timer configuration.
 *              Provides a 1 ms tick counter and a non-blocking timeout
 *              utility for software delays throughout the application.
 * @version     1.0
 * @date        12. June. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

#ifndef CFG_SYSTICK_H_
#define CFG_SYSTICK_H_

/* -------------------------------- Includes -------------------------------- */
#include "stdint.h"

/* ------------------------------ Public Variables -------------------------- */

/** Running millisecond counter incremented by SysTick_Handler. Wraps at 2^32 (~49 days). */
extern volatile uint32_t globalTicks;

/** Snapshot of globalTicks used as a reference start time for timeout measurements. */
extern volatile uint32_t refTime;

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  Initializes and starts the SysTick timer with a 1 ms period.
 *
 * Configures the internal SysTick clock for a 1 ms interrupt interval,
 * enables the SysTick interrupt, and starts the counter. After this call
 * globalTicks is incremented once per millisecond by SysTick_Handler.
 *
 * @note   Must be called once during system initialization before any
 *         call to timeOut(). Assumes the core clock has already been
 *         configured to its final operating frequency.
 */
void cfgSysTick(void);

/**
 * @brief  Non-blocking elapsed-time check.
 *
 * Computes the time elapsed since @p tiempo_inicial using unsigned
 * subtraction, which correctly handles the 32-bit rollover of globalTicks
 * without requiring special wraparound logic in the caller.
 *
 * @param  initTime		   Tick value captured at the start of the timed
 *                         period (typically a snapshot of globalTicks).
 * @param  delay_ms        Desired timeout duration in milliseconds.
 * @return 1 if @p delay_ms milliseconds have elapsed since @p tiempo_inicial;
 *         0 if the timeout has not yet expired.
 */
uint8_t timeOut(uint32_t initTime, uint32_t delay_ms);

#endif /* CFG_SYSTICK_H_ */

/* ------------------------------ End Of File ------------------------------- */
