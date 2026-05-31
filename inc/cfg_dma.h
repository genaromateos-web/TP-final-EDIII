/***************************************************************************
 * @file        	cfg_dma.h
 * @brief       Contains all the prototype functions to configure the DMA
 * @version     1.0
 * @date        25. May. 2026
 * @author      Genaro
 ***************************************************************************/

#ifndef CFG_DMA_H_
#define CFG_DMA_H_
/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_gpdma.h"

/* ------------------------------ Public Types ------------------------------ */

/**
 * @brief [Descripción breve de lo que agrupa o define este enum/struct].
 */
extern GPDMA_LLI_T lList1;
extern GPDMA_LLI_T lList2;

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  [Descripción breve de lo que hace la función].
 *
 * [Descripción detallada opcional si el comportamiento es complejo].
 *
 * @param  [nombre_param] [Descripción del parámetro, rango o valores válidos].
 * @return [Tipo de retorno] [Qué significa el valor devuelto].
 * @note   [Notas importantes, precondiciones o advertencias de uso].
 */
void cfgDma(uint32_t srcMemAddrBufferA, uint32_t srcMemAddrBufferB);

#endif /* CFG_DMA_H_ */
/* ------------------------------ End Of File ------------------------------- */
