/***************************************************************************
 * @file        keyboard_fsm.h
 * @brief       Finite state machine for signal generator control via
 *              matrix keypad.
 *
 *              Implements two operating modes:
 *
 *              Mode 1 — frequency entered via keypad (1–9999 Hz)
 *              Mode 2 — frequency controlled by potentiometer (ADC)
 *
 *              Each mode has its own context (waveform, frequency, amplitude)
 *              that is preserved independently when switching modes with key D.
 *
 * @version     1.0
 * @date        13. Jun. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

#ifndef KEYBOARD_FSM_H_
#define KEYBOARD_FSM_H_

/* -------------------------------- Includes -------------------------------- */
#include "keyboard.h"
#include "stdint.h"
#include "waveform_gen.h"

/* ------------------------------ Public types ------------------------------ */

/**
 * @brief Generator operating modes.
 */
typedef enum {
    GEN_MODE_1, /**< Frequency entered via keypad.          */
    GEN_MODE_2  /**< Frequency controlled by potentiometer. */
} GeneratorMode_t;

/**
 * @brief Configuration flow steps within each mode.
 *
 *        Mode 1: WAVEFORM ? FREQUENCY ? AMPLITUDE ? RUNNING ? (back to WAVEFORM)
 *        Mode 2: WAVEFORM ? AMPLITUDE ? RUNNING   ? (back to WAVEFORM)
 */
typedef enum {
    STEP_WAVEFORM,  /**< Waiting for waveform selection (A/B/C then #).        */
    STEP_FREQUENCY, /**< Waiting for frequency input in Hz (Mode 1 only).      */
    STEP_AMPLITUDE, /**< Waiting for amplitude input in tenths of a volt.      */
    STEP_RUNNING    /**< Output active. Only * and D keys are processed.       */
} ConfigStep_t;

/**
 * @brief Configuration context for one operating mode.
 */
typedef struct {
    WaveType_t waveType;  /**< Saved waveform type.                              */
    uint16_t frequency;   /**< Saved frequency in Hz.                            */
    uint16_t amplitude;   /**< Saved amplitude in Q10 units (0–1024).            */
    ConfigStep_t step;    /**< Current step in the configuration flow.           */
    uint16_t inputBuffer; /**< Digits accumulated during the current entry.      */
    uint8_t digitCount;   /**< Number of digits entered so far (max 4).          */
} ModeContext_t;

/* ---------------------------- Public variables ---------------------------- */

/**
 * @brief Currently active operating mode (Mode 1 or Mode 2).
 */
extern GeneratorMode_t activeMode;

/**
 * @brief Contexts for both modes. Index 0 = Mode 1, index 1 = Mode 2.
 */
extern ModeContext_t modeCtx[2];

/* ---------------------------- Public functions ---------------------------- */

/**
 * @brief  Initializes both mode contexts with default values.
 *
 *         Must be called once during system startup, after the waveform
 *         lookup tables have been initialized.
 *
 *         Default values:
 *           - Waveform  : Sine
 *           - Frequency : 60 Hz  (Mode 1 only)
 *           - Amplitude : 1024   (full scale = 3.3 V = 33 tenths of a volt)
 */
void fsmInit(void);

/**
 * @brief  Processes a captured key and advances the state machine.
 *
 *         Must be called from main once keyboardScan() returns a valid key
 *         (any key other than KEY_NONE).
 *
 *         Updates the active mode context, applies changes to currentWave,
 *         and calls the appropriate display hooks.
 *
 * @param  key  Key captured by keyboardScan().
 */
void fsmProcessKey(Key_t key);

/* ----------------------- Display hooks (weak) ----------------------------- */
/*
 * These functions are called by the FSM whenever the state changes.
 * They are declared __attribute__((weak)) so they do nothing by default
 * and can be overridden by the display module without modifying this file.
 *
 * Common state available inside every hook:
 *   - activeMode          : current operating mode (GEN_MODE_1 / GEN_MODE_2)
 *   - modeCtx[activeMode] : full context of the active mode
 */

/**
 * @brief  Called when the system enters STEP_WAVEFORM (stopped state).
 *         The display should show the waveform drawing currently saved.
 */
void displayOnWaveformStep(void);

/**
 * @brief  Called when A, B, or C is pressed during STEP_WAVEFORM.
 *         The display should update the waveform drawing for the selected type
 *         (preview only — not yet confirmed).
 *
 * @param  type  Waveform type just selected.
 */
void displayOnWaveformPreview(WaveType_t type);

/**
 * @brief  Called when the system enters STEP_FREQUENCY (Mode 1 only).
 *         The display should show 0 on D1 (empty field, ready for input).
 */
void displayOnFrequencyStep(void);

/**
 * @brief  Called when the system enters STEP_AMPLITUDE.
 *         The display should show 0 on D1 (empty field, ready for input).
 */
void displayOnAmplitudeStep(void);

/**
 * @brief  Called each time a digit is entered in STEP_FREQUENCY or STEP_AMPLITUDE.
 *         The display should show the accumulated value left-aligned.
 *
 * @param  currentValue  Numeric value accumulated so far.
 * @param  digitCount    Number of digits entered so far.
 */
void displayOnDigitInput(uint16_t currentValue, uint8_t digitCount);

/**
 * @brief  Called when the system enters STEP_RUNNING in Mode 1.
 *         The display should show "- - - -".
 */
void displayOnRunningMode1(void);

/**
 * @brief  Called periodically (from the ADC flag) while in STEP_RUNNING in Mode 2.
 *         The display should show the current frequency read from the potentiometer.
 *
 * @param  freqHz  Current frequency in Hz (1–1000).
 */
void displayOnRunningMode2(uint16_t freqHz);

#endif /* KEYBOARD_FSM_H_ */
       /* ------------------------------ End Of File ------------------------------- */
