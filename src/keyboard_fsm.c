/***************************************************************************
 * @file        keyboard_fsm.c
 * @brief       Finite state machine implementation for signal generator
 *              control via matrix keypad.
 *
 * @version     1.0
 * @date        13. Jun. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "keyboard_fsm.h"
#include "LPC17xx.h"
#include "cfg_adc.h"
#include "cfg_dac.h"
#include "cfg_timer.h"
#include "main.h"
#include "waveform_gen.h"

/* ------------------------------ Private macros ---------------------------- */

/** Default frequency on first system startup (Hz). */
#define DEFAULT_FREQUENCY 60U

/**
 * @brief Default amplitude: 1024 Q10 units = full scale 3.3 V.
 *        Corresponds to 33 tenths of a volt entered via keypad.
 */
#define DEFAULT_AMPLITUDE 1024U

/** Minimum valid amplitude in tenths of a volt (0.1 V). */
#define AMPLITUDE_MIN_TENTHS 0U

/** Maximum valid amplitude in tenths of a volt (3.3 V). */
#define AMPLITUDE_MAX_TENTHS 33U

/** Maximum number of digits accepted by any numeric input field. */
#define MAX_DIGITS 4U

/* ---------------------------- Public variables ---------------------------- */

GeneratorMode_t activeMode = GEN_MODE_1;

ModeContext_t modeCtx[2];

/* ---------------------------- Private helpers ----------------------------- */

/**
 * @brief  Converts tenths of a volt (0–33) to the internal Q10 scale (0–1024).
 *
 *         Formula: amplitude_q10 = (tenths * 1024) / 33
 *         tenths = 33 ? 1024 (full scale, 3.3 V)
 *         tenths =  0 ? 0    (0 V)
 *
 * @param  tenths  Value in tenths of a volt (0–33).
 * @return         Amplitude in Q10 scale.
 */
static uint16_t tenthsToQ10(uint16_t tenths) { return (uint16_t)(((uint32_t)tenths * 1024U) / 33U); }

/**
 * @brief  Applies the saved context of the active mode to currentWave.
 *
 *         Updates the currentWave pointer, waveform type, amplitude,
 *         and recalculates the phase step.
 *         In Mode 2 the frequency is not updated here — the ADC handles it.
 */
static void applyContextToWave(void) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    /* Select the WaveGen_t structure that matches the saved waveform type */
    switch (ctx->waveType) {
    case WAVE_SINE:
        currentWave = &sine;
        break;
    case WAVE_SQUARE:
        currentWave = &square;
        break;
    case WAVE_TRIANGLE:
        currentWave = &triangle;
        break;
    default:
        currentWave = &sine;
        break;
    }

    /* Apply saved amplitude */
    currentWave->amplitude = ctx->amplitude;

    /* Mode 1: fixed frequency set by the user; Mode 2: ADC updates it */
    if (activeMode == GEN_MODE_1) {
        currentWave->frequency = ctx->frequency;
        currentWave->phaseStep = calculatePhaseStep(ctx->frequency);
    }

    /* Raise the data-transmission flag */
    waveControl |= bitMask(3);
}

/**
 * @brief  Resets the digit input buffer of the active mode.
 */
static void clearInputBuffer(void) {
    modeCtx[activeMode].inputBuffer = 0;
    modeCtx[activeMode].digitCount = 0;
}

/**
 * @brief  Appends a digit to the active mode's input buffer.
 *
 *         If MAX_DIGITS has already been reached the digit is discarded
 *         (field locked).
 *
 * @param  digit  Digit to append (0–9).
 * @return        1 if the digit was accepted; 0 if the field is locked.
 */
static uint8_t pushDigit(uint8_t digit) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    if (ctx->digitCount >= MAX_DIGITS) {
        /* Field locked: maximum of 4 digits already entered */
        return 0;
    }

    /* Shift existing value left one decimal place and append the new digit */
    ctx->inputBuffer = ctx->inputBuffer * 10U + digit;
    ctx->digitCount++;

    return 1;
}

/* -------------------- FSM step handlers ----------------------------------- */

/**
 * @brief  Handles key presses during STEP_WAVEFORM.
 *
 *         A / B / C  : preview the selected waveform on the display.
 *         #          : confirm the selection and advance to the next step.
 *         *          : jump straight to STEP_RUNNING with saved parameters.
 *         D          : handled by the global dispatcher before this function.
 */
static void handleWaveformStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    switch (key) {

    case KEY_A:
        ctx->waveType = WAVE_SINE;
        displayOnWaveformPreview(WAVE_SINE);
        break;

    case KEY_B:
        ctx->waveType = WAVE_SQUARE;
        displayOnWaveformPreview(WAVE_SQUARE);
        break;

    case KEY_C:
        ctx->waveType = WAVE_TRIANGLE;
        displayOnWaveformPreview(WAVE_TRIANGLE);
        break;

    case KEY_HASH:
        /* Confirm the selected waveform and advance to the next field */
        clearInputBuffer();

        if (activeMode == GEN_MODE_1) {
            /* Mode 1: next step is frequency entry */
            ctx->step = STEP_FREQUENCY;
            displayOnFrequencyStep();
        } else {
            /* Mode 2: no frequency entry — go directly to amplitude */
            ctx->step = STEP_AMPLITUDE;
            displayOnAmplitudeStep();
        }
        break;

    case KEY_ASTERISK:
        /*
         * User pressed * before selecting a waveform.
         * Start output using the previously saved parameters.
         */
        applyContextToWave();
        dacCounterEnable(24, DISABLE);
        ctx->step = STEP_RUNNING;

        if (activeMode == GEN_MODE_1) {
            dacCounterEnable(24, DISABLE);
            displayOnRunningMode1();
        } else {
            /* Enable ADC and timer so the potentiometer controls frequency */
            ADC_On();
            TIM_On(LPC_TIM0);
            displayOnRunningMode2(ctx->frequency);
        }
        break;

    default:
        /* Digits and all other keys are ignored in this step */
        break;
    }
}

/**
 * @brief  Handles key presses during STEP_FREQUENCY (Mode 1 only).
 *
 *         0–9  : accumulate digit.
 *         #    : confirm frequency and advance to STEP_AMPLITUDE.
 *         *    : start output using the previously saved frequency.
 */
static void handleFrequencyStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    switch (key) {

    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9: {
        uint8_t digit = (uint8_t)key; /* Numeric Key_t value equals the digit */
        if (pushDigit(digit)) {
            displayOnDigitInput(ctx->inputBuffer, ctx->digitCount);
        }
        /* If pushDigit returns 0, the field is locked — no display update */
        break;
    }

    case KEY_HASH: {
        /*
         * Confirm the entered frequency.
         * If the buffer is 0 (empty field or explicit "0" entered),
         * keep the previously saved frequency unchanged.
         */
        if (ctx->inputBuffer > 0) {
            ctx->frequency = ctx->inputBuffer;
        }

        clearInputBuffer();
        ctx->step = STEP_AMPLITUDE;
        displayOnAmplitudeStep();
        break;
    }

    case KEY_ASTERISK:
        /*
         * User pressed * without confirming frequency.
         * Discard the partial entry and start with the saved frequency.
         */
        clearInputBuffer();
        applyContextToWave();
        dacCounterEnable(24, DISABLE);
        ctx->step = STEP_RUNNING;
        displayOnRunningMode1();
        break;

    default:
        /* A, B, C, D are ignored in this step */
        break;
    }
}

/**
 * @brief  Handles key presses during STEP_AMPLITUDE (both modes).
 *
 *         0–9  : accumulate digit.
 *         #    : confirm amplitude and advance to STEP_RUNNING.
 *         *    : start output using the previously saved amplitude.
 */
static void handleAmplitudeStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    switch (key) {

    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9: {
        uint8_t digit = (uint8_t)key;
        if (pushDigit(digit)) {
            displayOnDigitInput(ctx->inputBuffer, ctx->digitCount);
        }
        break;
    }

    case KEY_HASH: {
        /*
         * Confirm the entered amplitude.
         * Valid range: 0–33 tenths of a volt.
         * If the value is out of range, keep the previously saved amplitude.
         */
        uint16_t tenths = ctx->inputBuffer;

        if (tenths >= AMPLITUDE_MIN_TENTHS && tenths <= AMPLITUDE_MAX_TENTHS) {
            ctx->amplitude = tenthsToQ10(tenths);
        }

        clearInputBuffer();
        applyContextToWave();
        ctx->step = STEP_RUNNING;
        dacCounterEnable(24, DISABLE);

        if (activeMode == GEN_MODE_1) {
            displayOnRunningMode1();
        } else {
            /* Enable potentiometer peripherals */
            ADC_On();
            TIM_On(LPC_TIM0);
            displayOnRunningMode2(ctx->frequency);
        }
        break;
    }

    case KEY_ASTERISK:
        /*
         * User pressed * without confirming amplitude.
         * Discard the partial entry and start with the saved amplitude.
         */
        clearInputBuffer();
        applyContextToWave();
        dacCounterEnable(24, DISABLE);
        ctx->step = STEP_RUNNING;

        if (activeMode == GEN_MODE_1) {
            dacCounterEnable(24, DISABLE);
            displayOnRunningMode1();
        } else {
            ADC_On();
            TIM_On(LPC_TIM0);
            displayOnRunningMode2(ctx->frequency);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief  Handles key presses during STEP_RUNNING (output active).
 *
 *         Only * (stop) and D (mode switch) are processed.
 *         All other keys are ignored.
 */
static void handleRunningStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    switch (key) {

    case KEY_ASTERISK:
        /* Stop output — no signal should be present during configuration */
        dacCounterDisable();
        DAC_UpdateValue(0);

        /* If running in Mode 2, shut down the ADC and potentiometer timer */
        if (activeMode == GEN_MODE_2) {
            TIM_Off(LPC_TIM0);
            ADC_Off();
        }

        ctx->step = STEP_WAVEFORM;
        displayOnWaveformStep();
        break;

    default:
        /* A, B, C, digits, HASH: ignored while output is active */
        break;
    }
}

/* ---------------------------- Public functions ---------------------------- */

void fsmInit(void) {
    /* ----- Mode 1: keyboard-controlled frequency ----- */
    modeCtx[GEN_MODE_1].waveType = WAVE_SINE;
    modeCtx[GEN_MODE_1].frequency = DEFAULT_FREQUENCY;
    modeCtx[GEN_MODE_1].amplitude = DEFAULT_AMPLITUDE;
    modeCtx[GEN_MODE_1].step = STEP_WAVEFORM;
    modeCtx[GEN_MODE_1].inputBuffer = 0;
    modeCtx[GEN_MODE_1].digitCount = 0;

    /* ----- Mode 2: ADC-controlled frequency ----- */
    modeCtx[GEN_MODE_2].waveType = WAVE_SINE;
    modeCtx[GEN_MODE_2].frequency = 0; /* Not used in Mode 2; ADC defines it */
    modeCtx[GEN_MODE_2].amplitude = DEFAULT_AMPLITUDE;
    modeCtx[GEN_MODE_2].step = STEP_WAVEFORM;
    modeCtx[GEN_MODE_2].inputBuffer = 0;
    modeCtx[GEN_MODE_2].digitCount = 0;

    activeMode = GEN_MODE_1;

    /* Show initial state on the display */
    displayOnWaveformStep();
}

void fsmProcessKey(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    /* -----------------------------------------------------------------
     * Key D: global mode switch.
     * Processed before any other key, from any step.
     * ----------------------------------------------------------------- */
    if (key == KEY_D) {
        /* Stop output — no signal during configuration */
        dacCounterDisable();
        DAC_UpdateValue(0);

        /* If Mode 2 was running, shut down the ADC and potentiometer timer */
        if (activeMode == GEN_MODE_2 && ctx->step == STEP_RUNNING) {
            TIM_Off(LPC_TIM0);
            ADC_Off();
        }

        /*
         * Discard any partial entry in the mode being left.
         * Confirmed values (waveType, frequency, amplitude) are preserved.
         */
        clearInputBuffer();

        /* Switch to the other mode */
        activeMode = (activeMode == GEN_MODE_1) ? GEN_MODE_2 : GEN_MODE_1;

        /*
         * The new mode always starts in the stopped state.
         * If it was mid-configuration, reset it to STEP_WAVEFORM.
         */
        modeCtx[activeMode].step = STEP_WAVEFORM;

        /* Refresh the display for the new mode's current state */
        displayOnWaveformStep();
        return;
    }

    /* -----------------------------------------------------------------
     * Dispatch to the handler for the current step of the active mode.
     * ----------------------------------------------------------------- */
    switch (ctx->step) {

    case STEP_WAVEFORM:
        handleWaveformStep(key);
        break;

    case STEP_FREQUENCY:
        /* STEP_FREQUENCY exists only in Mode 1; Mode 2 never reaches this */
        handleFrequencyStep(key);
        break;

    case STEP_AMPLITUDE:
        handleAmplitudeStep(key);
        break;

    case STEP_RUNNING:
        handleRunningStep(key);
        break;

    default:
        break;
    }
}

/* ----------- Default (weak) implementations of display hooks -------------- */
/*
 * These functions do nothing until the display module overrides them.
 * Being __attribute__((weak)), the linker will use the display module's
 * version when available, with no changes required here.
 */

__attribute__((weak)) void displayOnWaveformStep(void) {}
__attribute__((weak)) void displayOnWaveformPreview(WaveType_t type) { (void)type; }
__attribute__((weak)) void displayOnFrequencyStep(void) {}
__attribute__((weak)) void displayOnAmplitudeStep(void) {}
__attribute__((weak)) void displayOnDigitInput(uint16_t v, uint8_t d) {
    (void)v;
    (void)d;
}
__attribute__((weak)) void displayOnRunningMode1(void) {}
__attribute__((weak)) void displayOnRunningMode2(uint16_t f) { (void)f; }

/* ------------------------------ End Of File ------------------------------- */
