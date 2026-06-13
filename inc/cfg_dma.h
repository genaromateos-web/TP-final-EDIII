/***************************************************************************
 * @file        cfg_dma.h
 * @brief       Public API for the GPDMA configuration used to stream DAC
 *              samples. Declares the linked-list descriptors, the active-buffer
 *              flag, and the setup function that enables double-buffered,
 *              continuous memory-to-DAC transfers on the LPC1769.
 * @version     1.0
 * @date        25. May. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

#ifndef CFG_DMA_H_
#define CFG_DMA_H_

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_gpdma.h"

/* ------------------------------ Public Variables -------------------------- */

/**
 * @brief  GPDMA linked-list item for buffer A.
 *         Points to buffer A as source and chains to lList2 on completion,
 *         implementing the double-buffer ping-pong loop.
 */
extern GPDMA_LLI_T lList1;

/**
 * @brief  GPDMA linked-list item for buffer B.
 *         Points to buffer B as source and chains back to lList1 on completion,
 *         closing the double-buffer ping-pong loop.
 */
extern GPDMA_LLI_T lList2;

/**
 * @brief  Tracks which buffer the DMA is currently transmitting.
 *         0 = buffer A is active (being sent); 1 = buffer B is active.
 *         Updated by DMA_IRQHandler on each terminal-count interrupt.
 *         The idle buffer (the one NOT indicated by this flag) is safe to refill.
 */
extern volatile uint8_t actBuffer;

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  Configures and starts a continuous double-buffered GPDMA transfer
 *         from two memory buffers to the LPC1769 DAC register (DACR).
 *
 * Sets up a ping-pong linked-list chain: lList1 transfers buffer A, then
 * hands off to lList2 which transfers buffer B, which in turn hands back to
 * lList1. This keeps the DAC fed without CPU intervention between buffers.
 * A terminal-count interrupt fires after each buffer completes, allowing the
 * ISR to refill the idle buffer while the other is being transmitted.
 *
 * Transfer properties:
 *   - Channel  : GPDMA_CH_0 (highest hardware priority)
 *   - Direction: memory -> DAC (GPDMA_M2P)
 *   - Width    : 16-bit halfword on both source and destination (DAC uses 10 bits)
 *   - Burst    : single-beat (GPDMA_BSIZE_1)
 *   - Source address increments after each beat; destination address is fixed (DACR).
 *
 * @param  srcMemAddrBufferA  Start address of DAC sample buffer A
 *                            (must hold at least BUFFER_SIZE uint16_t elements).
 * @param  srcMemAddrBufferB  Start address of DAC sample buffer B
 *                            (must hold at least BUFFER_SIZE uint16_t elements).
 *
 * @note   DMA_IRQn is enabled at priority 0 (highest) so the refill ISR
 *         runs without latency. Call this function once during system
 *         initialization, after both buffers have been pre-filled.
 */
void cfgDma(uint32_t srcMemAddrBufferA, uint32_t srcMemAddrBufferB);

#endif /* CFG_DMA_H_ */
       /* ------------------------------ End Of File ------------------------------- */
