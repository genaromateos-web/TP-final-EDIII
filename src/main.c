#include "main.h"
#include "LPC17xx.h"
#include "cfg_adc.h"
#include "cfg_dac.h"
#include "cfg_dma.h"
#include "cfg_systick.h"
#include "cfg_timer.h"
#include "cfg_uart.h"
#include "keyboard.h"
#include "keyboard_fsm.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_systick.h"
#include "waveform_gen.h"

/*
 * Bit 0: Si es 1 algun buffer debe ser rellenado
 * Bit 1: Si es 1 se debe actualizar la frecuencia determinada por la lectura en el ADC
 * Bit 2: Si es 1 se esta presionando una tecla
 * Bit 3: Si es 1 se debe transmitir un paquete a traves del UART
 * Bit 4: Se debe manejar el multiplexado de displays
 */
volatile uint8_t waveControl = 0;

int main(void) {
    // Se configura el DAC
    cfgDac(DAC_700uA, DIV_4);
    /* --- Deshabilitar periféricos antes de configurar para evitar comportamiento
     *     inesperado durante la inicialización --- */
    dacCounterDisable();
    SYSTICK_Cmd(DISABLE);
    TIM_Off(LPC_TIM0);

    // Se iniciaizan con sus valores por defecto las onda y se digitilizan sus muestras en la RAM
    initWaveTable(&sine, WAVE_SINE, sineTable);
    initWaveTable(&square, WAVE_SQUARE, squareTable);
    initWaveTable(&triangle, WAVE_TRIANGLE, triangleTable);
    // Rellenamos ambos buffers inicialmente para que el DMA tenga datos listos. Por defecto seno
    fillBuffer(bufferA, (WaveGen_t *)currentWave);
    fillBuffer(bufferB, (WaveGen_t *)currentWave);

    /*Inicializar la máquina de estados del teclado */
    fsmInit();
    // Se configura el teclado
    cfgKeyboard();
    // Se configura el ADC
    cfgAdc(CHANNEL_ADC);
    // MR = T_M / T_TC - 1. MR = 250000us / 1us - 1 = 249999
    cfgTimer0(249999);
    // Se configura la UART para el monitoreo por PC
    cfgUart();
    // La primera transferencia (Buffer A -> DAC)
    cfgDma((uint32_t)bufferA, (uint32_t)bufferB);
    // se configura systick
    cfgSysTick();

    while (1) {
        if (waveControl & bitMask(0)) {
            // limpio flag
            waveControl &= ~bitMask(0);
            // CPU rellena el buffer que se acaba de liberar
            if (actBuffer == 0) {
                // El DMA está leyendo el A, la CPU rellena el B
                fillBuffer(bufferB, (WaveGen_t *)currentWave);
            } else if (actBuffer == 1) {
                // El DMA está leyendo el B, por ende la CPU rellena el A
                fillBuffer(bufferA, (WaveGen_t *)currentWave);
            }
        } else if (waveControl & bitMask(1)) {
            // limpio flag
            waveControl &= ~bitMask(1);
            // se actualiza la frecuencia de la onda
            waveUpdateFrequency((WaveGen_t *)currentWave, adcData);
            /* Notificar al display la frecuencia actualizada*/
            displayOnRunningMode2(((WaveGen_t *)currentWave)->frequency);
        } else if (waveControl & bitMask(2)) {
            // retardo 300ms
            if (capturedKey == KEY_NONE && timeOut(refTime, 50)) {
                // Escanear la matriz para saber exactamente qué tecla fue presionada
                capturedKey = keyboardScan();
                /* Procesar la tecla en la máquina de estados */
                if (capturedKey != KEY_NONE) {
                    fsmProcessKey(capturedKey);
                }
            }
            /* * Al estar todas las filas en BAJO, si todas las columnas vuelven a ALTO (1),
             * significa que el usuario soltó por completo la tecla.*/
            if ((GPIO_ReadValue(PORT_2) & ALL_COLS) == ALL_COLS) {
                // Limpio flag
                waveControl &= ~bitMask(2);
                // No hay tecla presionada
                capturedKey = KEY_NONE;
                /* Limpiar rebotes remanentes en los registros de interrupción */
                GPIO_ClearInt(PORT_2, ALL_COLS);
                NVIC_ClearPendingIRQ(EINT3_IRQn);
                /* Re-habilitar la interrupción externa en el NVIC para la próxima pulsación física */
                NVIC_EnableIRQ(EINT3_IRQn);
            }
        } else if (waveControl & bitMask(3) && timeOut(uartRefTime, 250)) {
            // Limpio flag
            waveControl &= ~bitMask(3);
            // Envio datos de la onda
            UART_SendWaveParams((WaveGen_t *)currentWave);
            // Guardo valor de referencia para proximo envio
            uartRefTime = globalTicks;
        }
    }

    return 0;
}
