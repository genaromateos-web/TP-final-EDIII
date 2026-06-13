/***************************************************************************
 * @file        keyboard_fsm.c
 * @brief       Implementación de la máquina de estados para el control del
 *              generador de señales mediante teclado matricial.
 *
 * @version     1.0
 * @date        13. Jun. 2026
 * @author      Signal Labs (Mateos & Flores)
 ***************************************************************************/

/* -------------------------------- Includes -------------------------------- */
#include "keyboard_fsm.h"
#include "LPC17xx.h"
#include "cfg_adc.h"
#include "cfg_timer.h"
#include "waveform_gen.h"
#include "cfg_dac.h"

/* ----------------------------- Macros privados ---------------------------- */

/** Frecuencia por defecto al arrancar el sistema por primera vez (Hz). */
#define DEFAULT_FREQUENCY 60U

/**
 * @brief Amplitud por defecto: 1024 unidades Q10 = 3,3 V (escala completa).
 *        Corresponde a 33 décimas de volt ingresadas por teclado.
 */
#define DEFAULT_AMPLITUDE 1024U

/** Amplitud mínima aceptable en décimas de volt (0,1 V). */
#define AMPLITUDE_MIN_TENTHS 0U

/** Amplitud máxima aceptable en décimas de volt (3,3 V). */
#define AMPLITUDE_MAX_TENTHS 33U

/** Máximo de dígitos que acepta cualquier campo numérico. */
#define MAX_DIGITS 4U

/* ----------------------------- Variables públicas ------------------------- */

GeneratorMode_t activeMode = GEN_MODE_1;

ModeContext_t modeCtx[2];

/* ----------------------------- Helpers privados --------------------------- */

/**
 * @brief  Convierte décimas de volt (0–33) a la escala Q10 interna (0–1024).
 *
 *         Fórmula: amplitude_q10 = (tenths * 1024) / 33
 *         Con tenths = 33 -> 1024 (escala completa, 3,3 V).
 *         Con tenths =  0 -> 0 (0V).
 *
 * @param  tenths  Valor en décimas de volt (1–33).
 * @return         Amplitud en escala Q10
 */
static uint16_t tenthsToQ10(uint16_t tenths) { return (uint16_t)(((uint32_t)tenths * 1024U) / 33U); }

/**
 * @brief  Aplica el contexto guardado del modo activo sobre currentWave.
 *
 *         Actualiza el puntero currentWave, la forma de onda, la frecuencia,
 *         la amplitud y recalcula el paso de fase.
 *         En Modo 2 no se actualiza la frecuencia (la maneja el ADC).
 */
static void applyContextToWave(void) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    /* Seleccionar la estructura WaveGen_t correspondiente a la forma de onda */
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

    /* Aplicar amplitud */
    currentWave->amplitude = ctx->amplitude;

    /* En Modo 1 la frecuencia es fija; en Modo 2 la actualiza el ADC */
    if (activeMode == GEN_MODE_1) {
        currentWave->frequency = ctx->frequency;
        currentWave->phaseStep = calculatePhaseStep(ctx->frequency);
    }
}

/**
 * @brief  Reinicia el buffer de ingreso de dígitos del modo activo.
 */
static void clearInputBuffer(void) {
    modeCtx[activeMode].inputBuffer = 0;
    modeCtx[activeMode].digitCount = 0;
}

/**
 * @brief  Acumula un dígito en el buffer de ingreso del modo activo.
 *
 *         Si ya se alcanzaron MAX_DIGITS el dígito se ignora (campo bloqueado).
 *
 * @param  digit  Dígito a acumular (0–9).
 * @return        1 si el dígito fue aceptado, 0 si el campo está bloqueado.
 */
static uint8_t pushDigit(uint8_t digit) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    if (ctx->digitCount >= MAX_DIGITS) {
        /* Campo bloqueado: se llegó al máximo de 4 dígitos */
        return 0;
    }

    /* Concatenar el dígito: valor = valor_actual * 10 + nuevo_dígito */
    ctx->inputBuffer = ctx->inputBuffer * 10U + digit;
    ctx->digitCount++;

    return 1;
}

/* ---------------------- Handlers de cada paso de la FSM ------------------- */

/**
 * @brief  Maneja las teclas durante STEP_WAVEFORM.
 *
 *         A / B / C  : previsualiza la onda en el display.
 *         #          : confirma la selección y avanza al siguiente paso.
 *         *          : va directo a STEP_RUNNING con los parámetros guardados.
 *         D          : se maneja en el dispatcher global.
 */
static void handleWaveformStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    switch (key) {

    case KEY_A:
        /* Previsualizar sinusoidal*/
        ctx->waveType = WAVE_SINE;
        displayOnWaveformPreview(WAVE_SINE);
        break;

    case KEY_B:
        /* Previsualizar cuadrada*/
        ctx->waveType = WAVE_SQUARE;
        displayOnWaveformPreview(WAVE_SQUARE);
        break;

    case KEY_C:
        /* Previsualizar triangular*/
        ctx->waveType = WAVE_TRIANGLE;
        displayOnWaveformPreview(WAVE_TRIANGLE);
        break;

    case KEY_HASH:
        /* Confirmar la onda seleccionada y avanzar al siguiente campo */
        clearInputBuffer();

        if (activeMode == GEN_MODE_1) {
            /* Modo 1: siguiente paso es ingresar frecuencia */
            ctx->step = STEP_FREQUENCY;
            displayOnFrequencyStep();
        } else {
            /* Modo 2: no hay ingreso de frecuencia, ir directo a amplitud */
            ctx->step = STEP_AMPLITUDE;
            displayOnAmplitudeStep();
        }
        break;

    case KEY_ASTERISK:
        /*
         * El usuario presiona * antes de seleccionar onda.
         * Se inicia la generación con los parámetros guardados anteriormente.
         */
        applyContextToWave();
        dacCounterEnable(24, DISABLE); /* Reanudar salida del DAC */
        ctx->step = STEP_RUNNING;

        if (activeMode == GEN_MODE_1) {
            displayOnRunningMode1();
        } else {
            /* Habilitar ADC y timer para que el potenciómetro tome control */
            ADC_On();
            TIM_On(LPC_TIM0);
            displayOnRunningMode2(ctx->frequency);
        }
        break;

    default:
        /* Dígitos y resto de teclas se ignoran en este paso */
        break;
    }
}

/**
 * @brief  Maneja las teclas durante STEP_FREQUENCY (solo Modo 1).
 *
 *         0–9  ? acumula dígito.
 *         #    ? confirma la frecuencia y avanza a STEP_AMPLITUDE.
 *         *    ? inicia la generación con la frecuencia guardada.
 */
static void handleFrequencyStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    /* En STEP_FREQUENCY solo llegan teclas del Modo 1 */
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
        uint8_t digit = (uint8_t)key; /* Key_t numérico == valor del dígito */
        if (pushDigit(digit)) {
            /* Informar al display el valor acumulado hasta ahora */
            displayOnDigitInput(ctx->inputBuffer, ctx->digitCount);
        }
        /* Si pushDigit devuelve 0, el campo está bloqueado y no se notifica */
        break;
    }

    case KEY_HASH: {
        /*
         * Confirmar frecuencia.
         * Si el valor ingresado es 0 (campo vacío o se ingresó "0"),
         * se conserva la frecuencia guardada anteriormente.
         */
        if (ctx->inputBuffer > 0) {
            ctx->frequency = ctx->inputBuffer;
        }
        /* inputBuffer == 0 -> se descarta, se conserva el valor anterior */

        clearInputBuffer();
        ctx->step = STEP_AMPLITUDE;
        displayOnAmplitudeStep();
        break;
    }

    case KEY_ASTERISK:
        /*
         * El usuario presiona * sin confirmar la frecuencia.
         * Se descarta el ingreso parcial y se inicia con la frecuencia guardada.
         */
        clearInputBuffer();
        applyContextToWave();
        dacCounterEnable(24, DISABLE); /* Reanudar salida del DAC */
        ctx->step = STEP_RUNNING;
        displayOnRunningMode1();
        break;

    default:
        /* A, B, C y D se ignoran aquí*/
        break;
    }
}

/**
 * @brief  Maneja las teclas durante STEP_AMPLITUDE (ambos modos).
 *
 *         0–9  : acumula dígito.
 *         #    : confirma la amplitud y avanza a STEP_RUNNING.
 *         *    : inicia la generación con la amplitud guardada.
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
         * Confirmar amplitud.
         * Rango válido en décimas de volt: 0–33.
         * Si tenths > 33 -> fuera de rango, se conserva el valor anterior.
         */
        uint16_t tenths = ctx->inputBuffer;

        if (tenths >= AMPLITUDE_MIN_TENTHS && tenths <= AMPLITUDE_MAX_TENTHS) {
            /* Convertir décimas de volt a escala Q10 y guardar */
            ctx->amplitude = tenthsToQ10(tenths);
        }

        clearInputBuffer();
        applyContextToWave();
        ctx->step = STEP_RUNNING;

        if (activeMode == GEN_MODE_1) {
            displayOnRunningMode1();
        } else {
            /* Habilitar periféricos del potenciómetro */
            ADC_On();
            TIM_On(LPC_TIM0);
            displayOnRunningMode2(ctx->frequency);
        }
        break;
    }

    case KEY_ASTERISK:
        /*
         * Iniciar generación descartando el ingreso parcial de amplitud.
         * Se usa la amplitud guardada anteriormente.
         */
        clearInputBuffer();
        applyContextToWave();
        dacCounterEnable(24, DISABLE); /* Reanudar salida del DAC */
        ctx->step = STEP_RUNNING;

        if (activeMode == GEN_MODE_1) {
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
 * @brief  Maneja las teclas durante STEP_RUNNING (generación activa).
 *
 *         Solo responden * (Stop) y D (cambio de modo).
 *         Todas las demás teclas son ignoradas.
 */
static void handleRunningStep(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    switch (key) {

    case KEY_ASTERISK:
        /*
         * Stop: detener la generación y volver al paso de selección de onda.
         * Si estábamos en Modo 2, apagar el ADC y el timer del potenciómetro.
         */
        if (activeMode == GEN_MODE_2) {
            TIM_Off(LPC_TIM0);
            ADC_Off();
        }
        /* Detener la salida del DAC: no debe haber señal durante la configuración */
        dacCounterDisable();
        DAC_UpdateValue(0);

        ctx->step = STEP_WAVEFORM;
        displayOnWaveformStep();
        break;

    default:
        /*
         * A, B, C, dígitos, HASH: ignorados durante la generación.
         */
        break;
    }
}

/* ----------------------------- Funciones públicas ------------------------- */

void fsmInit(void) {
    /* ----- Modo 1: frecuencia por teclado ----- */
    modeCtx[GEN_MODE_1].waveType = WAVE_SINE;
    modeCtx[GEN_MODE_1].frequency = DEFAULT_FREQUENCY;
    modeCtx[GEN_MODE_1].amplitude = DEFAULT_AMPLITUDE;
    modeCtx[GEN_MODE_1].step = STEP_WAVEFORM;
    modeCtx[GEN_MODE_1].inputBuffer = 0;
    modeCtx[GEN_MODE_1].digitCount = 0;

    /* ----- Modo 2: frecuencia por potenciómetro ----- */
    modeCtx[GEN_MODE_2].waveType = WAVE_SINE;
    modeCtx[GEN_MODE_2].frequency = 0; /* No tiene sentido en Modo 2; el ADC lo define */
    modeCtx[GEN_MODE_2].amplitude = DEFAULT_AMPLITUDE;
    modeCtx[GEN_MODE_2].step = STEP_WAVEFORM;
    modeCtx[GEN_MODE_2].inputBuffer = 0;
    modeCtx[GEN_MODE_2].digitCount = 0;

    activeMode = GEN_MODE_1;

    /* Mostrar el estado inicial en el display */
    displayOnWaveformStep();
}

void fsmProcessKey(Key_t key) {
    ModeContext_t *ctx = &modeCtx[activeMode];

    /* -----------------------------------------------------------------
     * Tecla D: cambio de modo global.
     * Se procesa antes que cualquier otra tecla, desde cualquier paso.
     * ----------------------------------------------------------------- */
    if (key == KEY_D) {
        /* Si estábamos en Modo 2 corriendo, apagar el ADC y el timer */
        if (activeMode == GEN_MODE_2 && ctx->step == STEP_RUNNING) {
            TIM_Off(LPC_TIM0);
            ADC_Off();
        }

        /*
         * Descartar cualquier ingreso parcial del modo que se abandona.
         * Los valores ya confirmados (waveType, frequency, amplitude) se conservan.
         */
        clearInputBuffer();

        /* Cambiar al otro modo */
        activeMode = (activeMode == GEN_MODE_1) ? GEN_MODE_2 : GEN_MODE_1;

        /*
         * El nuevo modo siempre arranca en Stop.
         * Si estaba en medio de una configuración previa, vuelve a STEP_WAVEFORM.
         */
        modeCtx[activeMode].step = STEP_WAVEFORM;

        /* Actualizar display al estado actual del nuevo modo */
        displayOnWaveformStep();
        return;
    }

    /* -----------------------------------------------------------------
     * Despachar al handler correspondiente al paso actual del modo activo.
     * ----------------------------------------------------------------- */
    switch (ctx->step) {

    case STEP_WAVEFORM:
        handleWaveformStep(key);
        break;

    case STEP_FREQUENCY:
        /* STEP_FREQUENCY solo existe en Modo 1; en Modo 2 nunca se llega aquí */
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

/* --------- Implementaciones por defecto (weak) de los hooks de display ---- */
/*
 * Estas funciones no hacen nada hasta que el módulo de display las sobreescriba.
 * Al ser __attribute__((weak)), el linker usará la versión del módulo de display
 * si existe, sin necesidad de modificar keyboard_fsm.c.
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
