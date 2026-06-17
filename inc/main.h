/***************************************************************************
* @file        	[nombre_del_archivo.h]
 * @brief       [Descripción breve de lo que contiene o hace este archivo].
 * @version     1.0
 * @date        [Día Mes Año de creación]
 * @author      [Nombre del Autor]
 ***************************************************************************/

#ifndef MAIN_H_
#define MAIN_H_

#include "stdint.h"

/* ------------------------------ Public Macros -------------------------- */
#define bitMask(bit) (1U << (bit))

/* ------------------------------ Public Variables -------------------------- */
extern volatile uint8_t waveControl;

/* ------------------------------ Public Types ------------------------------ */

/**
 * @brief [Descripción breve de lo que agrupa o define este enum/struct].
 */

/* ---------------------------- Public Functions ---------------------------- */

/**
 * @brief  [Descripción breve de lo que hace la función].
 *
 * [Descripción detallada opcional si el comportamiento es complejo].
 *
 * @param  [nombre_param] [Descripción del parámetro, rango o valores válidos].
 * @return [Tipo de retorno] [Qué significa el valor devuelto].
 * @note   [Notas importantes, precondiciones o advertencias de uso].
 */
int main(void);

#endif /* MAIN_H_ */
/* ------------------------------ End Of File ------------------------------- */
