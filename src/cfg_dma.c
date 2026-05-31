/***************************************************************************
 * @file        	cfg_dma.h
 * @brief       Contains all the function definitions to configure the DMA
 * @version     1.0
 * @date        25. May. 2026
 * @author      Genaro
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_dma.h"
#include "LPC17xx.h"
#include "waveform_gen.h"

GPDMA_LLI_T lList1;
GPDMA_LLI_T lList2;

/* ---------------------------- Public Functions ---------------------------- */
void cfgDma(uint32_t srcMemAddrBufferA, uint32_t srcMemAddrBufferB) {
    lList1.srcAddr = srcMemAddrBufferA;
    lList1.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lList1.nextLLI = (uint32_t)&lList2;
    lList1.control = BUFFER_SIZE | (GPDMA_HALFWORD << 18) | (GPDMA_HALFWORD << 21) | (1U << 26) | (1U << 31);

    lList2.srcAddr = srcMemAddrBufferB;
    lList2.dstAddr = (uint32_t)&LPC_DAC->DACR;
    lList2.nextLLI = (uint32_t)&lList1;
    lList2.control = BUFFER_SIZE | (GPDMA_HALFWORD << 18) | (GPDMA_HALFWORD << 21) | (1U << 26) | (1U << 31);

    GPDMA_Init();

    GPDMA_Channel_CFG_T dmaCfg;
    // Se usa el canal es mas prioridad pues es altamente prioritario transmitir la onda
    dmaCfg.channelNum = GPDMA_CH_0;
    // Se transmite el buffer completo
    dmaCfg.transferSize = BUFFER_SIZE;
    // Transferencia de memoria a DAC
    dmaCfg.type = GPDMA_M2P;
    dmaCfg.srcMemAddr = srcMemAddrBufferA;
    dmaCfg.dstMemAddr = 0;
    dmaCfg.srcConn = 0;
    dmaCfg.dstConn = GPDMA_DAC;
    // El ancho de palabra en la fuente y en el destino es de media palabra pues el DAC utiliza 10 bits
    dmaCfg.src.width = GPDMA_HALFWORD;
    dmaCfg.src.burst = GPDMA_BSIZE_1;
    // Incremento en la fuente para recorrer la tabla
    dmaCfg.src.increment = ENABLE;
    dmaCfg.dst.width = GPDMA_HALFWORD;
    dmaCfg.dst.burst = GPDMA_BSIZE_1;
    dmaCfg.dst.increment = DISABLE;
    // Interrupcion al finalizar la transferencia
    dmaCfg.intTC = ENABLE;
    dmaCfg.intErr = DISABLE;
    dmaCfg.linkedList = (uint32_t)&lList2;

    GPDMA_SetupChannel(&dmaCfg);
    GPDMA_ChannelStart(GPDMA_CH_0);
    // Se habilita interrupcion del dma para que entrar al handler al terminar la transferencia
    NVIC_EnableIRQ(DMA_IRQn);
    // Maxima prioridad pues es altamente prioritario transmitir la onda
    NVIC_SetPriority(DMA_IRQn, 0);
}
/* ------------------------------ End Of File ------------------------------- */
