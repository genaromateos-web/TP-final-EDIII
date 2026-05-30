UNIVERSIDAD NACIONAL DE CÓRDOBA

### FACULTAD DE CIENCIAS EXACTAS, FÍSICAS Y NATURALES

#### CÁTEDRA DE COMPUTACIÓN

#### Electrónica Digital 3

### Trabajo Práctico Final: Generador de Señales

<div style="text-align: center; margin: 20px;">
<img src="https://static.wikia.nocookie.net/logopedia/images/2/29/Uncescudo1896.jpg/revision/latest?cb=20251229193121&path-prefix=es" width="250" height="300">
</div>


**Grupo:** Signal Labs

**Integrantes:**

* Mateos Ferrero Genaro Agustín
* Flores Arnaldo Eliezer

**Profesor:** Erick

---

<h2 style="margin: 0px;">Índice</h2>
1. Portada(hacer). Lleva logo de la universidad, nombre equipo y de sus participantes, nombre de la materia, nombre del profesor y nombre del proyecto.
2. <a href="#resumenProyecto">Resumen del proyecto</a>
 * Explicación breve del objetivo, problema que resuelve y resultado esperado.
3. Descripción general del sistema
  * Funcionamiento global
   * Entradas y salidas
   * Flujo básico de operación
4. Arquitectura del sistema
   * Diagrama de modulos
5. Configuración de periféricos del microcontrolador
6. <a href="#funcSoftware">Funcionamiento del software</a>
8. 1 Estructura del programa
 8. 2 Inicialización del sistema
 8. 3 Lógica principal
 8. 4 Manejo de eventos/interrupciones
 8. 5 Algoritmos implementados
7. Conclusiones
9. 1 Objetivos alcanzados
9. 2 Limitaciones
9. 3 Mejoras futuras

------------------------------------------------------------------------------------------

<h2 id="resumenProyecto" style ="margin: 0px;">Resumen del proyecto </h2>
El presente proyecto consiste en el diseño e implementación de un generador digital de señales analógicas basado en el microcontrolador NXP LPC1769. El sistema tiene como objetivo producir señales periódicas senoidales, cuadradas y triangulares con frecuencia y amplitud configurables, haciendo uso de los periféricos integrados en el microcontrolador y de una interfaz de control local y remota.
El principio de funcionamiento del sistema se basa en la técnica de síntesis digital directa (DDS, Direct Digital Synthesis). Cada forma de onda es representada mediante tablas de muestras numéricas precalculadas y almacenadas en memoria RAM. Dichas tablas contienen los valores digitales correspondientes a cada punto de la señal, escalados al rango de operación del conversor digital-analógico (DAC) de 10 bits incorporado en el LPC1769.
La transferencia de muestras hacia el DAC es realizada mediante el controlador GPDMA del microcontrolador. El DMA recorre las tablas de manera cíclica y transfiere automáticamente cada muestra al registro del DAC, reduciendo significativamente la intervención de la CPU y permitiendo un funcionamiento eficiente en tiempo real.
La temporización de las transferencias es controlada por el temporizador interno del DAC, el cual genera solicitudes periódicas de actualización. La frecuencia de estas solicitudes determina la frecuencia final de la señal generada y depende de la relación entre la cantidad de muestras por período y la frecuencia deseada de salida. Adicionalmente, el sistema permite variar la frecuencia de salida de forma continua mediante un potenciómetro conectado al conversor analógico-digital (ADC) del microcontrolador. La tensión leída por el ADC es mapeada al rango de frecuencias configurado, actualizando el período del temporizador en tiempo real.
Para evitar inconsistencias durante la actualización dinámica de parámetros, el sistema utiliza una estrategia de doble buffer en memoria RAM, permitiendo recalcular nuevas tablas de muestras mientras el DMA continúa transmitiendo la señal activa.
La amplitud de la señal es ajustada mediante el reescalado digital de las muestras antes de su transferencia al DAC, preservando la forma de onda y evitando pérdidas significativas de resolución.
El sistema incorpora una interfaz de usuario local basada en un teclado matricial y visualización mediante displays de siete segmentos. El teclado permite seleccionar la forma de onda generada y modificar parámetros de operación como frecuencia y amplitud, complementando el ajuste analógico de frecuencia provisto por el potenciómetro, mientras que los displays muestran en tiempo real la frecuencia configurada.
Adicionalmente, el sistema implementa comunicación serial UART entre el microcontrolador y una computadora. Sobre la PC se desarrolla una interfaz gráfica interactiva que permite visualizar y modificar los parámetros principales del generador, incluyendo frecuencia, amplitud y tipo de señal.
La interfaz también representa gráficamente una simulación de la forma de onda generada, proporcionando una herramienta de monitoreo y control más intuitiva para el usuario.
En síntesis, el proyecto integra diversos módulos funcionales del LPC1769, entre ellos el DAC, el controlador DMA, temporizadores,  el ADC, memoria RAM, interrupciones y comunicación UART, junto con periféricos externos de entrada y visualización.
El resultado es un generador de señales configurable, eficiente y de bajo costo, capaz de demostrar la utilización coordinada de múltiples recursos hardware y software en una aplicación de procesamiento digital de señales en tiempo real.

# Cálculos y fundamentos DDS

## Frecuencia y período del temporizador DMA


> f<sub>TDAC</sub> = 25 MHz
> T<sub>TDAC</sub> = 40 ns


## Frecuencia de actualización del DAC


> f<sub>DAC</sub> = 1 MHz
> T<sub>DAC</sub> = 1 us

## Valor de recarga para el temporizador DMA


> DACCNTVAL = T<sub>act_DAC</sub> / T<sub>TDAC</sub> - 1 = 24


---

## Cantidad de muestras

Se utilizan 256 muestras para obtener una definición geométrica fiel a la señal.

> total_muestras = 256


---

## Acumulador de fase

Un ciclo de una onda senoidal equivale a dar una vuelta exacta a un círculo, es decir recorrer:


> 2π radianes


Se discretiza la circunferencia completa usando un entero de 32 bits:

* Inicio del ciclo (0 rad) → 0
* Final del ciclo (2π rad) → 2³² − 1 = 4,294,967,295

---

## Cálculo del paso de fase

### Período de la onda deseada


> T<sub>onda</sub> = 1 / f<sub>onda</sub>


### Muestras por ciclo


> T<sub>onda</sub> / T<sub>DAC</sub> = f<sub>DAC</sub> / f<sub>onda</sub>


### Paso de fase


> Paso = (f<sub>onda</sub> × 2^32) / f<sub>DAC</sub>


---

### Indexación de tabla

La tabla en memoria que almacena las muestras de la señal tiene 256 posiciones.

Para indexar esa tabla se necesitan 8 bits.

Por lo tanto:

> 
indice = acumulador >> 24


De esta manera se obtienen únicamente los 8 bits superiores del acumulador de fase.

---

### Resolución del generador

La mínima frecuencia modificable ocurre cuando:

> Paso de fase = 1


> Paso de fase = 1 * f<sub>s</sub> / 2<sup>32</sup>

De esta forma se justifica la utilización de un entero de 32 bits para discretizar la circunferencia, obteniendo una gran precisión para modificar la frecuencia de la onda.


<h2 id="funcSoftware" style ="margin: 0px;">Funcionamiento del software</h2>
<h3 style="margin: 0px;">DAC</h3>
El DAC será el encargado de convertir las muestras digitales generadas por el sistema en una señal analógica de salida. Para ello, el periférico recibirá periódicamente dichas muestras y generará en el pin de salida una aproximación analógica de la señal deseada.

El DAC posee una tasa de actualización de hasta 1 MHz, con un consumo aproximado de 700 µA. Esta característica se aprovecha para transferir datos mediante DMA a dicha velocidad, logrando una transferencia rápida y eficiente.

Por otro lado, la frecuencia de la onda observada en la salida del DAC no se modifica variando la velocidad de transferencia, sino mediante otro mecanismo de control implementado en el sistema.


   