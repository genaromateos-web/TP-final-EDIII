/***************************************************************************
 * @file        cfg_keyboard.c
 * @brief       Definitions for keyboard configuration, matrix scanning,
 *              and GPIO interrupt handling.
 * @version     2.0
 * @date        04. Jun. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "keyboard.h"
#include "cfg_systick.h"
#include "core_cmInstr.h"
#include "lpc17xx_common.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "main.h"

/* ------------------------------ Private Macros ------------------------------ */
/* NOP loop count for a ~3 µs settling delay at 100 MHz (1 NOP ? 10 ns) */
#define clockCycles 300

/* Last key captured by the ISR + debounce flow; read by the application layer */
Key_t capturedKey = KEY_NONE;

/* ---------------------------- Public Functions ---------------------------- */

void cfgKeyboard(void) {
    PINSEL_CFG_T pinCfg;
    pinCfg.port = PORT_2;
    pinCfg.func = PINSEL_FUNC_00; /* GPIO function (no alternate function) */
    pinCfg.mode = PINSEL_PULLUP;
    pinCfg.openDrain = DISABLE;

    /* Columns: GPIO inputs with internal pull-up resistors.
     * When a key is pressed, the driven-LOW row pulls the column line LOW. */
    PINSEL_ConfigMultiplePins(&pinCfg, ALL_COLS);
    GPIO_SetDir(PORT_2, ALL_COLS, GPIO_INPUT);

    /* Rows: GPIO outputs in tri-state (no pull resistor) to avoid bus contention
     * when multiple rows are driven simultaneously during scanning. */
    pinCfg.mode = PINSEL_TRISTATE;
    PINSEL_ConfigMultiplePins(&pinCfg, ALL_ROWS);
    GPIO_SetDir(PORT_2, ALL_ROWS, GPIO_OUTPUT);

    /* Drive all rows LOW so that any key press pulls its column LOW,
     * generating a falling edge that triggers the EINT3 interrupt. */
    GPIO_ClearPins(PORT_2, ALL_ROWS);

    /* Enable falling-edge interrupt on all column pins */
    GPIO_IntConfigPort(PORT_2, ALL_COLS, GPIO_INT_FALLING);

    /* Clear any pending EINT3 flag before enabling to avoid a spurious ISR */
    NVIC_ClearPendingIRQ(EINT3_IRQn);
    NVIC_EnableIRQ(EINT3_IRQn);

    /* Lower priority than DMA, ADC, and display-multiplex timer,
     * so those time-critical ISRs are never delayed by a key event. */
    NVIC_SetPriority(EINT3_IRQn, 3);
}

Key_t keyboardScan(void) {
    /* Pin masks for each row and column, indexed 0–3 */
    const uint32_t rowPin[MAX_NROWS] = {ROW_0_PIN, ROW_1_PIN, ROW_2_PIN, ROW_3_PIN};
    const uint32_t colPin[MAX_NCOLS] = {COL_0_PIN, COL_1_PIN, COL_2_PIN, COL_3_PIN};

    /* Logical key map: keyMap[row][col] */
    const Key_t keyMap[MAX_NROWS][MAX_NCOLS] = {{KEY_1, KEY_2, KEY_3, KEY_A},
                                                {KEY_4, KEY_5, KEY_6, KEY_B},
                                                {KEY_7, KEY_8, KEY_9, KEY_C},
                                                {KEY_ASTERISK, KEY_0, KEY_HASH, KEY_D}};

    Key_t detectedKey = KEY_NONE;

    /* Raise all rows HIGH before scanning so only one row is driven LOW at a time */
    GPIO_SetPins(PORT_2, ALL_ROWS);

    for (uint8_t row = 0; row < MAX_NROWS; row++) {
        /* Assert the current row LOW to activate it */
        GPIO_ClearPins(PORT_2, rowPin[row]);

        /* Settling delay: allows parasitic capacitances on the lines to discharge
         * before sampling. At 100 MHz each NOP ? 10 ns ? 300 NOPs ? 3 µs. */
        for (uint16_t i = 0; i < clockCycles; i++) {
            __NOP();
        }

        /* Sample all column pins and mask out non-column bits */
        uint32_t cols = GPIO_ReadValue(PORT_2) & ALL_COLS;

        /* Deassert the row before moving on to prevent cross-talk */
        GPIO_SetPins(PORT_2, rowPin[row]);

        for (uint8_t col = 0; col < MAX_NCOLS; col++) {
            /* A LOW column level indicates a key press at (row, col) */
            if (!(cols & colPin[col])) {
                detectedKey = keyMap[row][col];
                break; /* Only one key per row is expected */
            }
        }

        /* Early exit once a pressed key has been identified */
        if (detectedKey != KEY_NONE) {
            break;
        }
    }

    /* Restore all rows LOW so column lines are ready to detect the next key press */
    GPIO_ClearPins(PORT_2, ALL_ROWS);

    return detectedKey;
}

/* ------------------------- Interrupt Handler ------------------------- */
void EINT3_IRQHandler(void) {
    /* Clear the GPIO interrupt flags for all column pins to acknowledge the event */
    GPIO_ClearInt(PORT_2, ALL_COLS);

    /* Disable EINT3 to prevent interrupt storms caused by mechanical switch bounce.
     * The interrupt will be re-enabled after the debounce period expires. */
    NVIC_DisableIRQ(EINT3_IRQn);

    /* Signal the main loop that a key press event is pending (bit 2 of waveControl) */
    waveControl |= bitMask(2);

    /* Snapshot the current tick count as the debounce time reference */
    refTime = globalTicks;
}

/* ------------------------------ End Of File ------------------------------- */
