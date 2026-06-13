/***************************************************************************
 * @file        cfg_adc.c
 * @brief       It contains the definition of the functions that configure the ADC.
 * @version     1.0
 * @date        30. May. 2026
 * @author      Genaro
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "cfg_adc.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_timer.h"

/* ------------------------------ Public Variables -------------------------- */
volatile uint16_t adcData;

/* ---------------------------- Public Functions ---------------------------- */

void cfgAdc(ADC_CHANNEL channel) {
    // Set ADC clock to achieve the maximum sampling rate of 200 kHz
    ADC_Init(200000);
    // Trigger a new conversion on each TIMER0 MATCH1 event
    ADC_StartCmd(ADC_START_ON_MAT01);
    // Conversion starts on the rising edge of the match signal
    ADC_EdgeStartConfig(ADC_START_ON_RISING);

    // Enable sampling for the selected channel
    ADC_ChannelEnable(channel);
    // Configure the physical pin associated with the selected channel
    ADC_PinConfig(channel);

    // Enable end-of-conversion interrupt for the selected channel only.
    // Adding 'channel' to ADC_INT_CH0 gives the interrupt flag of that channel.
    ADC_IntEnable(ADC_INT_CH0 + channel);
    // Disable the global interrupt so only the selected channel can trigger the ISR
    ADC_IntDisable(ADC_INT_GLOBAL);
    // Clear pending flags and enable the ADC IRQ in the NVIC
    ADC_EnableIRQ(channel);
    // Leave the ADC powered off until it is actually needed
    ADC_Off();
}

void ADC_EnableIRQ(ADC_CHANNEL channel) {
    // Read global data register to clear the global DONE and OVERRUN flags
    ADC_GlobalGetData();
    // Read channel data register to clear the channel-specific DONE and OVERRUN flags
    ADC_ChannelGetData(channel);
    // Clear any ADC interrupt already pending in the NVIC before enabling
    NVIC_ClearPendingIRQ(ADC_IRQn);
    // High priority
    NVIC_SetPriority(ADC_IRQn, 0);
    // Enable the ADC interrupt line in the NVIC
    NVIC_EnableIRQ(ADC_IRQn);
}

void ADC_On(void) {
    // Enable the ADC peripheral clock via PCONP
    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD, ENABLE);
    // Bring the ADC out of power-down mode
    ADC_PowerUp();
}

void ADC_Off(void) {
    // Put the ADC into power-down mode before cutting its clock
    ADC_PowerDown();
    // Disable the ADC peripheral clock via PCONP to reduce power consumption
    CLKPWR_ConfigPPWR(CLKPWR_PCONP_PCAD, DISABLE);
}

/* ------------------------------ End Of File ------------------------------- */
