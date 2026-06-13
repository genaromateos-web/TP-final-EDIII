/***************************************************************************
 * @file        keyboard_fsm.h
 * @brief       Máquina de estados para el control del generador de señales
 *              mediante teclado matricial.
 *
 *              Implementa los dos modos de operación descritos en la
 *              referencia del teclado:
 *
 *              Modo 1 — frecuencia ingresada por teclado (1–9999 Hz)
 *              Modo 2 — frecuencia controlada por potenciómetro (ADC)
 *
 *              Cada modo tiene su propio contexto (forma de onda, frecuencia,
 *              amplitud) que se conserva de forma independiente al cambiar
 *              de modo con la tecla D.
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

/* ------------------------------ Tipos públicos ---------------------------- */

/**
 * @brief Modos de operación del generador.
 */
typedef enum {
    GEN_MODE_1, /**< Frecuencia ingresada por teclado.          */
    GEN_MODE_2  /**< Frecuencia controlada por potenciómetro.   */
} GeneratorMode_t;

/**
 * @brief Pasos del flujo de configuración dentro de cada modo.
 *        Modo 1: WAVEFORM ? FREQUENCY ? AMPLITUDE ? RUNNING ? (vuelve a WAVEFORM)
 *        Modo 2: WAVEFORM ? AMPLITUDE ? RUNNING   ? (vuelve a WAVEFORM)
 */
typedef enum {
    STEP_WAVEFORM,  /**< Esperando selección de forma de onda (A/B/C + #).  */
    STEP_FREQUENCY, /**< Esperando ingreso de frecuencia en Hz (solo M1).   */
    STEP_AMPLITUDE, /**< Esperando ingreso de amplitud en décimas de V.     */
    STEP_RUNNING    /**< Generación activa. Solo responden * y D.           */
} ConfigStep_t;

/**
 * @brief Contexto de configuración de un modo de operación.
 */
typedef struct {
    WaveType_t waveType;  /**< Forma de onda guardada.                         */
    uint16_t frequency;   /**< Frecuencia guardada en Hz.                      */
    uint16_t amplitude;   /**< Amplitud guardada en unidades Q10 (0–1024).     */
    ConfigStep_t step;    /**< Paso actual del flujo de configuración.         */
    uint16_t inputBuffer; /**< Dígitos acumulados durante el ingreso actual.   */
    uint8_t digitCount;   /**< Cantidad de dígitos ingresados (máx. 4).        */
} ModeContext_t;

/* ----------------------------- Variables públicas ------------------------- */

/**
 * @brief Modo activo actualmente (Modo 1 o Modo 2).
 */
extern GeneratorMode_t activeMode;

/**
 * @brief Contextos de ambos modos. Índice 0 = Modo 1, índice 1 = Modo 2.
 */
extern ModeContext_t modeCtx[2];

/* ----------------------------- Funciones públicas ------------------------- */

/**
 * @brief  Inicializa los contextos de ambos modos con los valores por defecto.
 *
 *         Debe llamarse una vez durante la inicialización del sistema,
 *         después de inicializar las tablas de onda.
 *
 *         Valores por defecto:
 *           - Forma de onda : Sinusoidal
 *           - Frecuencia    : 60 Hz  (Modo 1 únicamente)
 *           - Amplitud      : 1024   (equivale a 3,3 V = 33 décimas)
 */
void fsmInit(void);

/**
 * @brief  Procesa una tecla capturada y avanza la máquina de estados.
 *
 *         Debe llamarse desde el main una vez que keyboardScan() devolvió
 *         una tecla válida (distinta de KEY_NONE).
 *
 *         Internamente actualiza el contexto del modo activo, aplica los
 *         cambios sobre currentWave, y llama a los hooks de display.
 *
 * @param  key  Tecla capturada por keyboardScan().
 */
void fsmProcessKey(Key_t key);

/* ----------------------- Hooks de display (weak) -------------------------- */
/*
 * Estas funciones son llamadas por la FSM cada vez que el estado cambia.
 * Se declaran __attribute__((weak)) para que por ahora no hagan nada,
 * y puedan ser sobreescritas sin modificar este módulo cuando se implemente
 * el módulo de display.
 *
 * Parámetros comunes disponibles para todos los hooks:
 *   - activeMode          : modo activo (GEN_MODE_1 / GEN_MODE_2)
 *   - modeCtx[activeMode] : contexto completo del modo activo
 */

/**
 * @brief  Llamada cuando el sistema entra al paso STEP_WAVEFORM (Stop).
 *         El display debe mostrar el dibujo de la onda actualmente guardada.
 */
void displayOnWaveformStep(void);

/**
 * @brief  Llamada al presionar A, B o C durante STEP_WAVEFORM.
 *         El display debe actualizar el dibujo según la onda seleccionada.
 *
 * @param  type  Forma de onda que se acaba de seleccionar (aún sin confirmar).
 */
void displayOnWaveformPreview(WaveType_t type);

/**
 * @brief  Llamada cuando el sistema entra al paso STEP_FREQUENCY (solo Modo 1).
 *         El display debe mostrar 0 en D1 (campo vacío, listo para editar).
 */
void displayOnFrequencyStep(void);

/**
 * @brief  Llamada cuando el sistema entra al paso STEP_AMPLITUDE.
 *         El display debe mostrar 0 en D1 (campo vacío, listo para editar).
 */
void displayOnAmplitudeStep(void);

/**
 * @brief  Llamada cada vez que se ingresa un dígito en STEP_FREQUENCY o STEP_AMPLITUDE.
 *         El display debe mostrar el valor acumulado hasta el momento, alineado a la izquierda.
 *
 * @param  currentValue  Valor numérico acumulado hasta este dígito.
 * @param  digitCount    Cantidad de dígitos ingresados.
 */
void displayOnDigitInput(uint16_t currentValue, uint8_t digitCount);

/**
 * @brief  Llamada cuando el sistema entra al paso STEP_RUNNING en Modo 1.
 *         El display debe mostrar "- - - -".
 */
void displayOnRunningMode1(void);

/**
 * @brief  Llamada periódicamente (desde el flag del ADC) mientras el sistema
 *         está en STEP_RUNNING en Modo 2.
 *         El display debe mostrar la frecuencia actual leída del potenciómetro.
 *
 * @param  freqHz  Frecuencia actual en Hz (1–1000).
 */
void displayOnRunningMode2(uint16_t freqHz);

#endif /* KEYBOARD_FSM_H_ */
/* ------------------------------ End Of File ------------------------------- */
