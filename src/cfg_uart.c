/***************************************************************************
 * @file        cfg_uart.c
 * @brief       UART configuration — function definitions.
 *              Implements UART0 initialization and wave-parameter transmission.
 * @version     1.0
 * @date        31. May. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_uart.h"

/* ---------------------------- Public variables ---------------------------- */

volatile uint32_t uartRefTime = 0;

/* ---------------------------- Public functions ---------------------------- */

void cfgUart(void) {
    /* Configure the physical TX pin (P0.2 → UART0 TX) */
    UART_PinConfig(UART_TX0_P0_2);

    /* Set communication format: 9600 baud, no parity, 8 data bits, 1 stop bit */
    UART_CFG_T uartCfg;
    uartCfg.baudRate = UART_BAUD_RATE;
    uartCfg.parity   = UART_PARITY_NONE;
    uartCfg.dataBits = UART_DBITS_8;
    uartCfg.stopBits = UART_STOPBIT_1;

    UART_Init((LPC_UART_TypeDef *)LPC_UART0, &uartCfg);

    /* Configure FIFOs: flush both RX and TX buffers, disable DMA,
     * trigger RX interrupt at the 1-byte threshold (TRGLEV0)           */
    UART_FIFO_CFG_T fifoCfg;
    fifoCfg.resetRxBuf = ENABLE;
    fifoCfg.resetTxBuf = ENABLE;
    fifoCfg.dmaMode    = DISABLE;
    fifoCfg.level      = UART_FIFO_TRGLEV0;

    UART_FIFOConfig((LPC_UART_TypeDef *)LPC_UART0, &fifoCfg);

    /* Enable the UART transmitter */
    UART_TxEnable((LPC_UART_TypeDef *)LPC_UART0);
}

void UART_SendWaveParams(WaveGen_t *wave) {
    uint8_t txbuf[PKT_SIZE];

    txbuf[0] = PKT_START;                        /* Frame start delimiter      */
    txbuf[1] = (uint8_t)(wave->frequency >> 8);  /* Frequency high byte        */
    txbuf[2] = (uint8_t)(wave->frequency & 0xFF);/* Frequency low byte         */
    txbuf[3] = (uint8_t)(wave->amplitude >> 8);  /* Amplitude high byte        */
    txbuf[4] = (uint8_t)(wave->amplitude & 0xFF);/* Amplitude low byte         */
    txbuf[5] = (uint8_t)(wave->type);            /* Wave type (0=sine, 1=square, 2=triangle) */
    txbuf[6] = PKT_END;                          /* Frame end delimiter        */

    UART_Send((LPC_UART_TypeDef *)LPC_UART0, txbuf, PKT_SIZE, NONE_BLOCKING);
}

/* ------------------------------ End Of File ------------------------------- */
