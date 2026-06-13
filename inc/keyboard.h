/***************************************************************************
 * @file        cfg_keyboard.h
 * @brief       Types and prototypes for the 4x4 matrix keyboard
 *              using GPIO Interrupts (EINT3).
 * @version     2.0
 * @date        04. Jun. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

#ifndef CFG_KEYBOARD_H_
#define CFG_KEYBOARD_H_

/* -------------------------------- Includes -------------------------------- */
#include "LPC17xx.h"
#include "stdint.h"

/* ------------------------------ Public Types ------------------------------ */

/* --- Port 2 row pin masks (outputs, driven LOW to activate a row) --- */
#define ROW_0_PIN (1U << 0) /* P2.0 – Row 0 */
#define ROW_1_PIN (1U << 1) /* P2.1 – Row 1 */
#define ROW_2_PIN (1U << 2) /* P2.2 – Row 2 */
#define ROW_3_PIN (1U << 3) /* P2.3 – Row 3 */

/* --- Port 2 column pin masks (inputs with pull-up, read to detect a key press) --- */
#define COL_0_PIN (1U << 4) /* P2.4 – Column 0 */
#define COL_1_PIN (1U << 5) /* P2.5 – Column 1 */
#define COL_2_PIN (1U << 6) /* P2.6 – Column 2 */
#define COL_3_PIN (1U << 7) /* P2.7 – Column 3 */

/* Composite masks used to configure or drive all rows/columns at once */
#define ALL_ROWS (ROW_0_PIN | ROW_1_PIN | ROW_2_PIN | ROW_3_PIN)
#define ALL_COLS (COL_0_PIN | COL_1_PIN | COL_2_PIN | COL_3_PIN)

/* Matrix dimensions – used as loop bounds in the scan routine */
#define MAX_NROWS 4
#define MAX_NCOLS 4

/**
 * @brief  Symbolic identifiers for every key on the 4x4 matrix keyboard.
 *
 * Numeric keys (KEY_0–KEY_9) carry their face value for direct use in
 * arithmetic. Letter keys (KEY_A–KEY_D) and special keys are mapped to
 * sequential values above 9. KEY_NONE indicates the idle / no-press state
 * and is returned by keyboardScan() when no key is detected.
 */
typedef enum {
    KEY_0 = 0,
    KEY_1 = 1,
    KEY_2 = 2,
    KEY_3 = 3,
    KEY_4 = 4,
    KEY_5 = 5,
    KEY_6 = 6,
    KEY_7 = 7,
    KEY_8 = 8,
    KEY_9 = 9,
    KEY_A = 10,
    KEY_B = 11,
    KEY_C = 12,
    KEY_D = 13,
    KEY_ASTERISK = 14, /* '*' – Start / Stop action */
    KEY_HASH = 15,     /* '#' – Confirm / Enter action */
    KEY_NONE = 16      /* No key pressed / idle state */
} Key_t;

/* Last key captured by the ISR + debounce flow; written in cfg_keyboard.c,
 * read by the application layer to retrieve the validated key press. */
extern Key_t capturedKey;

/* ------------------------- Public Function Prototypes ------------------------- */

/**
 * @brief  Initializes the 4x4 matrix keyboard on Port 2.
 *
 * - Rows  (P2.0–P2.3): configured as GPIO outputs in tri-state mode, then
 *   driven LOW so any key press pulls a column line LOW.
 * - Columns (P2.4–P2.7): configured as GPIO inputs with internal pull-ups;
 *   a falling edge on any column triggers the EINT3 ISR.
 * - EINT3 is cleared, enabled, and assigned a priority lower than DMA,
 *   ADC, and the display-multiplex timer.
 */
void cfgKeyboard(void);

/**
 * @brief  Scans the full 4x4 key matrix and identifies the pressed key.
 *
 * Activates each row LOW in turn, samples all column lines after a short
 * settling delay, then restores the row HIGH before moving to the next.
 * Exits early on the first pressed key found (single-key detection).
 * All rows are driven LOW again before returning so the columns are ready
 * to trigger the next EINT3 interrupt.
 *
 * @return Key_t  Identifier of the key currently pressed, or KEY_NONE if
 *                no key is detected.
 */
Key_t keyboardScan(void);

#endif /* CFG_KEYBOARD_H_ */
       /* ------------------------------ End Of File ------------------------------- */
