/***************************************************************************
 * @file        cfg_systick.c
 * @brief       SysTick timer initialization and non-blocking timeout utility.
 *              Maintains a 1 ms global tick counter used as the application
 *              time base for software delays and event scheduling.
 * @version     1.0
 * @date        12. June. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_systick.h"
#include "lpc17xx_systick.h"
#include "lpc_types.h"

/* ------------------------------ Public Variables -------------------------- */
/** Millisecond tick counter; incremented by SysTick_Handler every 1 ms. */
volatile uint32_t globalTicks = 0;

/** Reference timestamp for timeout measurements; set by the caller before invoking timeOut(). */
volatile uint32_t refTime;

/* ---------------------------- Public Functions ---------------------------- */

void cfgSysTick(void) {
    SYSTICK_InternalInit(1); /* Configure SysTick for a 1 ms interrupt period */
    SYSTICK_IntCmd(ENABLE);  /* Enable the SysTick interrupt                  */
    SYSTICK_Cmd(ENABLE);     /* Start the SysTick counter                     */
}

uint8_t timeOut(uint32_t initTime, uint32_t delay_ms) {
    /* Unsigned subtraction handles globalTicks rollover correctly:
     * even when globalTicks wraps past 2^32, the difference remains valid
     * as long as the timeout period is shorter than ~49 days.
     * Returns 1 when the elapsed time meets or exceeds delay_ms, 0 otherwise. */
    return (globalTicks - initTime) >= delay_ms;
}

/* ------------------------- Interrupt Handler ------------------------- */
void SysTick_Handler(void) { globalTicks++; /* Increment the 1 ms tick counter on every SysTick interrupt */ }

/* ------------------------------ End Of File ------------------------------- */
