/***************************************************************************
 * @file        cfg_adc.h
 * @brief       It contains the prototypes of the functions that configure the ADC.
 * @version     1.0
 * @date        30. May. 2026
 * @author      Genaro
 ***************************************************************************/

#ifndef CFG_ADC_H_
#define CFG_ADC_H_

/* -------------------------------- Includes -------------------------------- */
#include "lpc17xx_adc.h"

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  Configures the ADC for hardware-triggered sampling on the selected channel.
 *
 * @param  channel  ADC channel to configure and enable.
 *                  Valid values: ADC_CHANNEL_0 … ADC_CHANNEL_7.
 * @note   The ADC is disabled after configuration. Call ADC_On() to power it up
 *         when a conversion is required.
 */
void cfgAdc(ADC_CHANNEL channel);

/**
 * @brief  Clears any pending ADC flags and enables the ADC IRQ in the NVIC.
 *
 * @param  channel  ADC channel whose data register will be cleared.
 *                  Valid values: ADC_CHANNEL_0 … ADC_CHANNEL_7.
 */
void ADC_EnableIRQ(ADC_CHANNEL channel);

/**
 * @brief  Enables the ADC peripheral clock and powers it up.
 */
void ADC_On(void);

/**
 * @brief  Powers down the ADC and disables its peripheral clock.
 */
void ADC_Off(void);

#endif /* CFG_ADC_H_ */

/* ------------------------------ End Of File ------------------------------- */
