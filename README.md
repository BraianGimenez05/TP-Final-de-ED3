# Trabajo Final DIG III

**Electrónica Digital III** — Facultad de Ciencias Exactas, Físicas y Naturales — Universidad Nacional de Córdoba

## Requisitos

* Comunicación con PC bidireccional para controlar micro.

* Interfaz de usuario.

* Protocolo de comunicación con el microcontrolador, documentado en este README.

* Un sensor, de los vistos en clase.

* Un actuador, de los vistos en clase.

* Definir los criterios de aceptación completos según su TP individual.

# Sistema de Levitación Neumática en Lazo Cerrado

> **Grupo:** C2-G11
>
> **Integrantes:**
>
> * Franco Diaz
>
> * Braian Gimenez
>
> * Gustavo Witanowsky

> **Profesor:** Marcos Blasco

## 1. Descripción General del Proyecto

El sistema mantiene una esfera (pelota de telgopor) suspendida en el aire dentro de un conducto vertical transparente. Un ventilador ubicado en la base del tubo genera el flujo de aire, y un sensor de distancia infrarrojo ubicado en el tope de la estructura mide la altura real de la esfera. Una LPC1769 ejecuta un lazo de control PID que ajusta la potencia del ventilador para estabilizar la esfera en la altura de consigna (setpoint) indicada por el usuario.

Desde un dashboard en Python en la PC, comunicado por UART, el usuario fija el setpoint, ajusta las constantes del PID, observa la respuesta del sistema en tiempo real y graba los ensayos. Está pensado como planta didáctica para estudiar control realimentado: respuesta a escalón, rechazo de perturbaciones y efecto de cada constante del PID.

### Alcances del Proyecto

* El sistema es capaz de:

  * Medir la altura de la esfera con un sensor IR analógico, acondicionado con un filtro activo y adquirido por ADC + DMA con promediado.

  * Convertir la lectura del sensor a milímetros mediante una tabla de calibración medida sobre el equipo.

  * Mantener la esfera en una altura de consigna mediante un PID periódico (período de 40 ms, disparado por la interrupción del Timer1) que actúa sobre el ventilador por PWM.

  * Recibir por UART el setpoint, las constantes Kp, Ki, Kd, el duty de equilibrio, la velocidad de rampa y los parámetros de perturbación.

  * Enviar telemetría continua (tiempo, setpoint, altura, duty y lectura del ADC).

  * **Alarma Autónoma y Perturbación (DAC + DMA):** Cuando el error absoluto de posición supera los ±10 cm (±100 mm), el sistema dispara una alarma autónoma. El DMA copia una tabla desde la memoria hacia el DAC para generar una onda cuadrada sin intervención de la CPU, logrando que el buzzer suene de forma intermitente. A su vez, esta onda cuadrada se puede sumar matemáticamente al duty del ventilador. Esto simula "ráfagas" de aire artificiales a una frecuencia configurable (0,4 a 5 Hz), permitiendo evaluar dinámicamente qué tan rápido el PID compensa la fuerza externa (rechazo de perturbaciones).

* Fuera de alcance:

  * Conectividad inalámbrica (Wi-Fi / Bluetooth).

  * Almacenamiento de datos en el propio equipo (tarjeta SD o memoria flash).

  * Autoajuste (autotuning) de las constantes del PID.

### Criterios de Aceptación

| \# | Criterio | Cómo se verifica | 
| ----- | ----- | ----- | 
| 1 | La esfera se mantiene en el setpoint con un error de ±2 cm en régimen permanente. | Telemetría por UART durante 30 s. | 
| 2 | Ante un escalón de setpoint de 10 cm, el sobrepico es menor al 10 %. | Gráfico de la respuesta en el dashboard. | 
| 3 | Ante el mismo escalón, el tiempo de establecimiento es menor a 6 s (banda de ±2 cm). | Gráfico de la respuesta en el dashboard. | 
| 4 | El setpoint se puede fijar entre 10 y 30 cm desde la PC (rango útil del sensor). | Comandos por UART. | 
| 5 | La esfera vuelve al setpoint luego de una perturbación externa. | Ensayo de perturbación. | 
| 6 | La comunicación es bidireccional: la PC envía comandos y la LPC responde y envía telemetría. | Dashboard / terminal serie. | 

## 2. Protocolo de Comunicación

* **Interfaz:** UART0 de la LPC1769 (TX = P0.2, RX = P0.3) a través de un adaptador USB-UART CP2102.

* **Configuración:** 9600 baudios, 8 bits de datos, sin paridad, 1 bit de stop (8N1).

* **Formato:** comandos en texto ASCII. Cada comando termina con `\r` o `\n` (tecla Enter). Máximo 15 caracteres por comando.

* **Respuestas:** texto ASCII terminado en `\r\n`.

### Comandos

| Comando | Descripción | Ejemplo | Respuesta | 
| ----- | ----- | ----- | ----- | 
| `A` | Arranca el PID (modo automático). Si el motor está detenido, aplica empujón de arranque. | `A` | `PID en marcha` | 
| `Dxxx` | Modo manual: detiene el PID y fija el duty en `xxx` por mil (0–1000). | `D560` | `MANUAL duty=560` | 
| `Sxxx` | Setpoint de altura en mm (límite 100–300 mm). | `S200` | línea de parámetros | 
| `Pxxx` | Kp × 1000, en ‰ de duty por mm de error. | `P300` | línea de parámetros | 
| `Ixxx` | Ki × 1000, en ‰ de duty por (mm · s). | `I30` | línea de parámetros | 
| `Kxxx` | Kd × 1000, en ‰ de duty · s por mm. | `K250` | línea de parámetros | 
| `Uxxx` | Duty de equilibrio u0, en por mil. | `U535` | línea de parámetros | 
| `Rxx` | Velocidad de la rampa del setpoint, en mm/s (0 = cambio de golpe). | `R20` | línea de parámetros | 
| `Wxx` | Amplitud de la onda cuadrada del DAC, en ‰ de duty (0 = apagada). | `W50` | línea de parámetros | 
| `Fxx` | Frecuencia de la perturbación/alarma, en décimas de Hz (0,4 a 5 Hz). | `F10` | línea de parámetros | 
| `?` | Consulta los parámetros actuales. | `?` | Detalles de SP, u0, Kp, Ki, Kd, W, F... | 

### Telemetría

Cada 200 ms la LPC envía una línea con el estado actual del sistema:
`t=12340 sp=175 y=173 u=558 a=2153`

*(t = ms desde el arranque, sp = setpoint en mm, y = altura medida, u = duty aplicado, a = cuentas crudas del ADC).*

## 3. Arquitectura del Sistema

### Hardware & Interconexión

```
                 ┌──────────────────────────┐   UART   ┌──────────────────┐
                 │         LPC1769          │◄────────►│ Dashboard Python │
 Sharp IR ──► Filtro ──► ADC ──DMA──► PID   │  CP2102  │  (gráfico, CSV)  │
 (5 V)       (LM358)       Timer1           │          └──────────────────┘
                 │                PWM ──────┼──► Optoacoplador ──► MOSFET K3569 ──► Ventilador
                 │                          │      (aislación)          (protección)
                 │  tabla ──DMA──► DAC ────┼──► Seguidor (LM358) ──► Transistor ──► Buzzer
                 └──────────────────────────┘

```

* **Descripción del circuito y consideraciones de diseño:**

  * **Sensor:** Sharp GP2Y0A21YK0F alimentado a 5 V, con capacitores de desacople.

  * **Acondicionamiento:** filtro pasa-bajos activo Sallen-Key de \~16 Hz con LM358 como seguidor. La salida pasa por una resistencia de 1 kΩ para limitar la corriente hacia la entrada del ADC.

  * **Actuador:** Ventilador de 12 V / 0,26 A conmutado por un MOSFET K3569 (2SK3569), aislado galvánicamente por un optoacoplador PC817 (limitando el PWM a 500 Hz).

  * **Alarma DAC-DMA:** El sistema monitorea el error absoluto. Si `|Setpoint - Altura| > 100 mm`, la CPU habilita el canal del DMA. El DMA transfiere de forma autónoma una tabla de memoria al DAC (AOUT) dibujando una onda cuadrada. Esta señal se aísla con la otra mitad del LM358 (seguidor) y excita la base de un transistor NPN para hacer sonar el buzzer intermitentemente de forma física puro hardware.

### Arquitectura de Software (Firmware)

* **Módulos:**

  * `ventilador` — PWM1.1 a 500 Hz, resolución de 0,1 %.

  * `comunicacionUart` — UART0 por interrupciones (FIFO).

  * `sensor` — ADC en modo burst con GPDMA (buffer circular de 64 muestras) alojado en la RAM AHB.

  * `demora` — Timer0 contando en µs para telemetría.

  * `perturbacion` — GPDMA hacia el DAC operando la señal de alarma en onda cuadrada.

* **Lazo de Control (TIMER1, cada 40 ms):**

  * Lee el promedio del ADC del DMA.

  * Calcula el PID con Anti-Windup y Derivada sobre la medición (filtrada).

  * Verifica el error de posición para encender/apagar el DMA del buzzer.

  * Actualiza el PWM independientemente del `main`.

## 4. Pruebas y Resultados

* **Curva de calibración del sensor:**
  La medición es lineal (\~27 cuentas/cm) entre 0 y 35 cm. Por encima de 35 cm, el sensor entra en saturación, justificando el límite de Setpoint establecido por software entre 100 y 300 mm.

* **Respuesta a Escalones y Perturbaciones (Ensayos documentados en CSV):**

  * La sintonización definitiva lograda con (u0 = 53,5 %, Kp = 0,30, Ki = 0,030, Kd = 0,25) demostró una regulación estable a 200 mm.

  * La acción derivativa demostró amortiguar exitosamente los movimientos excesivos de la esfera.

  * El disparo de la alarma a los ±10 cm validó la interacción entre el cálculo matemático del PID y los periféricos de transferencia directa (DMA+DAC).

  * Limitación reconocida: La dinámica no lineal del tubo neumático afecta la estabilización perfecta en los extremos del conducto en comparación con el punto central de calibración.

## 5. Estructura del Repositorio

```
├── firmware/          # Código fuente del proyecto (C - MCUXpresso)
│   ├── src/           # Archivos de lógica y periféricos
│   └── inc/           # Headers de configuración
├── pc/                # Dashboard en Python para telemetría y control
├── hardware/          # Lista de materiales y conexionado
├── docs/              # Modelo y diseño del PID, notas técnicas
│   └── ensayos/       # Registros tabulados en CSV de las pruebas
├── scrum/             # Weekly, review y retrospectiva del equipo
└── README.md

```