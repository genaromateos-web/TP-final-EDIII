/***************************************************************************
 * @file        cfg_dma.c
 * @brief       GPDMA initialization for continuous double-buffered DAC output.
 *              Builds a two-entry linked-list (ping-pong) chain and starts
 *              an unattended memory-to-DAC stream on channel 0.
 * @version     1.0
 * @date        25. May. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_dma.h"
#include "LPC17xx.h"
#include "main.h"
#include "waveform_gen.h"

/* ------------------------------ Public Variables -------------------------- */

/* Linked-list descriptors for the ping-pong double-buffer scheme */
GPDMA_LLI_T lList1;
GPDMA_LLI_T lList2;

/** Tracks the buffer currently being transmitted by the DMA (0 = A, 1 = B). */
volatile uint8_t actBuffer = 0;

/* ---------------------------- Public Functions ---------------------------- */

void cfgDma(uint32_t srcMemAddrBufferA, uint32_t srcMemAddrBufferB) {

    /* --- Linked-list item 1: buffer A ? DAC, then chain to lList2 --- */
    lList1.srcAddr = srcMemAddrBufferA;
    lList1.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lList1.nextLLI = (uint32_t)&lList2;
    /* BUFFER_SIZE beats | 16-bit src width (bit 18-20) | 16-bit dst width (bit 21-23)
     * | src-increment enable (bit 26) | terminal-count interrupt enable (bit 31) */
    lList1.control = BUFFER_SIZE | (GPDMA_HALFWORD << 18) | (GPDMA_HALFWORD << 21) | (1U << 26) | (1U << 31);

    /* --- Linked-list item 2: buffer B ? DAC, then chain back to lList1 --- */
    lList2.srcAddr = srcMemAddrBufferB;
    lList2.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lList2.nextLLI = (uint32_t)&lList1;
    /* Same control word as lList1; destination address and flags are identical */
    lList2.control = BUFFER_SIZE | (GPDMA_HALFWORD << 18) | (GPDMA_HALFWORD << 21) | (1U << 26) | (1U << 31);

    /* Power up and reset the GPDMA peripheral */
    GPDMA_Init();

    /* --- Channel configuration --- */
    GPDMA_Channel_CFG_T dmaCfg;

    /* CH_0 has the highest hardware priority; waveform output is time-critical */
    dmaCfg.channelNum = GPDMA_CH_0;
    /* Transfer one full buffer per linked-list entry */
    dmaCfg.transferSize = BUFFER_SIZE;
    /* Memory-to-peripheral: RAM buffer ? DAC register */
    dmaCfg.type = GPDMA_M2P;
    dmaCfg.srcMemAddr = srcMemAddrBufferA;
    dmaCfg.dstMemAddr = 0; /* Unused for M2P transfers */
    dmaCfg.srcConn = 0;    /* Source is plain memory, no peripheral connection */
    dmaCfg.dstConn = GPDMA_DAC;

    /* 16-bit halfword width: DAC DACR.VALUE field is 10 bits, packed in a 16-bit write */
    dmaCfg.src.width = GPDMA_HALFWORD;
    dmaCfg.src.burst = GPDMA_BSIZE_1;
    /* Increment source address to walk through the sample buffer */
    dmaCfg.src.increment = ENABLE;

    dmaCfg.dst.width = GPDMA_HALFWORD;
    dmaCfg.dst.burst = GPDMA_BSIZE_1;
    /* Destination is a fixed register; do not increment */
    dmaCfg.dst.increment = DISABLE;

    /* Raise an interrupt after each buffer is fully transferred (terminal count) */
    dmaCfg.intTC = ENABLE;
    /* Also interrupt on bus or configuration errors for fault detection */
    dmaCfg.intErr = ENABLE;
    /* Start the ping-pong loop: after buffer A, hardware follows this pointer to lList2 */
    dmaCfg.linkedList = (uint32_t)&lList2;

    GPDMA_SetupChannel(&dmaCfg);
    GPDMA_ChannelStart(GPDMA_CH_0);

    /* Clear any stale interrupt flags before enabling the IRQ */
    GPDMA_ClearIntPending(GPDMA_CLR_INTERR, GPDMA_CH_0);
    GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);
    NVIC_ClearPendingIRQ(DMA_IRQn);
    /* Enable the DMA IRQ so the ISR can refill the idle buffer on each transfer-complete event */
    NVIC_EnableIRQ(DMA_IRQn);
    /* Priority 0 (highest) ensures the refill ISR is never delayed by other interrupts */
    NVIC_SetPriority(DMA_IRQn, 0);
}

/* ---------------------------- Interrupt Handler --------------------------- */

void DMA_IRQHandler(void) {
    /* Any DMA error is unrecoverable; halt here to allow debugger inspection */
    if (GPDMA_IntGetStatus(GPDMA_INTERR, GPDMA_CH_0)) {
        while (1) {}
    }

    /* Clear the terminal-count interrupt flag for the next transfer */
    GPDMA_ClearIntPending(GPDMA_CLR_INTTC, GPDMA_CH_0);

    /* Advance the active-buffer index so the main loop knows which buffer
     * is now idle and safe to refill with new samples */
    if (actBuffer == 0) {
        /* Buffer A just finished transmitting; buffer B is now active */
        actBuffer = 1;
    } else if (actBuffer == 1) {
        /* Buffer B just finished transmitting; buffer A is now active */
        actBuffer = 0;
    }

    /* Signal the main loop to refill the idle buffer (bit 0 of waveControl) */
    waveControl |= bitMask(0);
}

/* ------------------------------ End Of File ------------------------------- */
