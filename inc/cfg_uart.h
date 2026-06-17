/***************************************************************************
 * @file        cfg_uart.h
 * @brief       UART configuration — public interface.
 *              Declares the initialization function and the wave-parameter
 *              transmit function, along with the packet protocol constants.
 * @version     1.0
 * @date        31. May. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

#ifndef CFG_UART_H_
#define CFG_UART_H_

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_uart.h"
#include "waveform_gen.h"

/* ---------------------------- Public variables ---------------------------- */

/** Reference timestamp used for UART timeout tracking (milliseconds). */
extern volatile uint32_t uartRefTime;

/* ----------------------------- Protocol constants ------------------------- */

/** UART baud rate (bps). */
#define UART_BAUD_RATE 9600

/** First byte of every outgoing packet — marks the start of a frame. */
#define PKT_START 0xAA

/** Last byte of every outgoing packet — marks the end of a frame. */
#define PKT_END 0xFF

/** Total number of bytes in one wave-parameter packet. */
#define PKT_SIZE 7

/* ---------------------------- Public functions ---------------------------- */

/**
 * @brief  Initializes UART0 for wave-parameter transmission.
 *
 *         Configures the TX pin, sets 9600 8N1 format, resets the FIFOs,
 *         disables DMA mode, and enables the transmitter.
 *         Must be called once during system startup before any UART_Send call.
 */
void cfgUart(void);

/**
 * @brief  Transmits the current wave parameters over UART0.
 *
 *         Builds a 7-byte packet and sends it non-blocking:
 *
 *           Byte 0 : PKT_START  (0xAA — frame delimiter)
 *           Byte 1 : frequency MSB
 *           Byte 2 : frequency LSB
 *           Byte 3 : amplitude MSB
 *           Byte 4 : amplitude LSB
 *           Byte 5 : wave type  (0 = sine, 1 = square, 2 = triangle)
 *           Byte 6 : PKT_END    (0xFF — frame delimiter)
 *
 * @param  wave  Pointer to the WaveGen_t structure holding the parameters
 *               to transmit. Must not be NULL.
 */
void UART_SendWaveParams(WaveGen_t *wave);

#endif /* CFG_UART_H_ */
       /* ------------------------------ End Of File ------------------------------- */
