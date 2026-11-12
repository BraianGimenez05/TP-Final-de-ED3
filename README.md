# Trabajo Final DIG III

**Electrónica Digital III** — Facultad de Ciencias Exactas, Físicas y Naturales — Universidad Nacional de Córdoba

## Requisitos

- Comunicación con PC bidireccional para controlar micro.
- Interfaz de usuario.
- Protocolo de comunicación con el microcontrolador, documentado en este README.
- Un sensor, de los vistos en clase.
- Un actuador, de los vistos en clase.
- Definir los criterios de aceptación completos según su TP individual

---

# Sistema de Levitación Neumática en Lazo Cerrado
> **Grupo:** C2-G11
> **Integrantes:**
> * Franco Diaz
> * Braian Gimenez
> * Gustavo Witanowsky

> **Profesor:** Marcos Blasco

---

## 1. Descripción General del Proyecto
El sistema mantiene una esfera (pelota de telgopor) suspendida en el aire dentro de un conducto vertical transparente. Un ventilador ubicado en la base del tubo genera el flujo de aire, y un sensor de distancia infrarrojo ubicado en el tope de la estructura mide la altura real de la esfera. Una LPC1769 ejecuta un lazo de control PID que ajusta la potencia del ventilador para estabilizar la esfera en la altura de consigna (setpoint) indicada por el usuario.

Desde un dashboard en Python en la PC, comunicado por UART, el usuario fija el setpoint, ajusta las constantes del PID, observa la respuesta del sistema en tiempo real y graba los ensayos. Está pensado como planta didáctica para estudiar control realimentado: respuesta a escalón, rechazo de perturbaciones y efecto de cada constante del PID.

### Alcances del Proyecto
* El sistema es capaz de:
  * Medir la altura de la esfera con un sensor IR analógico, acondicionado con un filtro activo y adquirido por ADC + DMA con promediado.
  * Convertir la lectura del sensor a milímetros mediante una tabla de calibración medida sobre el equipo.
  * Mantener la esfera en una altura de consigna mediante un PID periódico (período de 40 ms, disparado por la interrupción del Timer1) que actúa sobre el ventilador por PWM.
  * Recibir por UART el setpoint, las constantes Kp, Ki, Kd, el duty de equilibrio, la velocidad de rampa y los parámetros de la perturbación, y enviar telemetría (tiempo, setpoint, altura, duty y lectura del ADC).
  * Compensar perturbaciones externas: desplazamientos de la esfera, corrientes de aire y variaciones del punto de equilibrio del ventilador.
  * Generar con el DAC + DMA una perturbación senoidal de amplitud y frecuencia configurables (0,4 a 5 Hz), que se suma al duty del ventilador para estudiar el rechazo de perturbaciones. La misma señal sale por AOUT y maneja un buzzer como indicador.
* Fuera de alcance:
  * Conectividad inalámbrica (Wi-Fi / Bluetooth).
  * Almacenamiento de datos en el propio equipo (tarjeta SD o memoria flash). Los ensayos se registran en la PC a través de la UART.
  * Autoajuste (autotuning) de las constantes del PID.

### Criterios de Aceptación
| # | Criterio | Cómo se verifica |
|---|---|---|
| 1 | La esfera se mantiene en el setpoint con un error de ±2 cm en régimen permanente | Telemetría por UART durante 30 s |
| 2 | Ante un escalón de setpoint de 10 cm, el sobrepico es menor al 10 % | Gráfico de la respuesta en el dashboard |
| 3 | Ante el mismo escalón, el tiempo de establecimiento es menor a 6 s (banda de ±2 cm) | Gráfico de la respuesta en el dashboard |
| 4 | El setpoint se puede fijar entre 10 y 30 cm desde la PC (rango útil del sensor en este tubo) | Comandos por UART |
| 5 | La esfera vuelve al setpoint luego de una perturbación externa | Ensayo de perturbación |
| 6 | La comunicación es bidireccional: la PC envía comandos y la LPC responde y envía telemetría | Dashboard / terminal serie |

### Posibles Etapas Siguientes
* Revisar el acondicionamiento de la señal del sensor (en particular el zener de protección) para recuperar la sensibilidad y el nivel de ruido medidos en la primera calibración.
* Mejorar la respuesta ante escalones de setpoint: probar Ki = 0,020 con una rampa de 10 mm/s, o agregar peso de referencia en el término proporcional.
* Medir la respuesta en frecuencia completa con la perturbación del DAC, con más frecuencias y la telemetría a mayor velocidad.
* Agregar un tope mecánico dentro del tubo para que la esfera no ingrese en la zona ciega del sensor.
* Reemplazar el sensor Sharp por un sensor más preciso y con menor tiempo de actualización.
* Pasar el circuito de protoboard a PCB.

---

## 2. Protocolo de Comunicación

* Interfaz: UART0 de la LPC1769 (TX = P0.2, RX = P0.3) a través de un adaptador USB-UART CP2102.
* Configuración: 9600 baudios, 8 bits de datos, sin paridad, 1 bit de stop (8N1).
* Formato: comandos en texto ASCII. Cada comando termina con `\r` o `\n` (tecla Enter). Máximo 15 caracteres por comando.
* Respuestas: texto ASCII terminado en `\r\n`.

### Comandos

| Comando | Descripción | Ejemplo | Respuesta |
|---|---|---|---|
| `A` | Arranca el PID (modo automático) usando el u0 configurado. Si el ventilador está detenido, aplica u0 + 150 ‰ durante 300 ms para que el motor arranque. Con rampa, el setpoint parte de la altura actual de la esfera. Cuando la esfera despega de la rejilla (supera 50 mm), la integral se pone en cero | `A` | `PID en marcha` y línea de parámetros |
| `Dxxx` | Modo manual: detiene el PID y fija el duty en `xxx` por mil (0–1000) | `D560` | `MANUAL duty=560` |
| `Sxxx` | Setpoint de altura en mm desde la rejilla. Se limita a 100–300 mm | `S200` | línea de parámetros |
| `Pxxx` | Kp × 1000, en ‰ de duty por mm de error | `P300` | línea de parámetros |
| `Ixxx` | Ki × 1000, en ‰ de duty por (mm · s) | `I30` | línea de parámetros |
| `Kxxx` | Kd × 1000, en ‰ de duty · s por mm | `K250` | línea de parámetros |
| `Uxxx` | Duty de equilibrio u0, en por mil | `U535` | línea de parámetros |
| `Rxx` | Velocidad de la rampa del setpoint, en mm/s (0 = el setpoint cambia de golpe). Máximo 200 | `R20` | línea de parámetros |
| `Wxx` | Amplitud de la perturbación senoidal del DAC, en ‰ de duty (0 = apagada). Máximo 200. Se suma al duty solo en modo automático | `W50` | línea de parámetros |
| `Fxx` | Frecuencia de la perturbación, en décimas de Hz (4–50, es decir 0,4 a 5 Hz) | `F10` | línea de parámetros |
| `?` | Consulta los parámetros actuales | `?` | `SP=200 u0=535 Kp=300 Ki=30 Kd=250 (x1000) W=0 F=5 R=20 modo=AUTO` |
| (otro) | Comando no reconocido | `X` | lista de comandos |

Los parámetros modificados por UART se pierden al reiniciar la placa; los valores por defecto están en el firmware.

### Telemetría

Cada 200 ms la LPC envía una línea con el tiempo desde el arranque, el setpoint, la altura medida, el duty aplicado y la lectura cruda del ADC:

```text
t=12340 sp=175 y=173 u=558 a=2153
```

| Campo | Unidad |
|---|---|
| `t` | ms desde el arranque |
| `sp` | mm (setpoint que sigue el PID; con rampa, avanza de a poco hasta el valor pedido) |
| `y` | mm (altura de la esfera desde la rejilla) |
| `u` | por mil |
| `a` | cuentas del ADC (0–4095), promedio del buffer del DMA; se usa para calibrar el sensor |

A 9600 baudios cada línea tarda ~37 ms en transmitirse; por eso la telemetría se envía cada 200 ms y no en cada paso del lazo (40 ms).

---

## 3. Arquitectura del Sistema

### Hardware & Interconexión

```text
                 ┌──────────────────────────┐   UART   ┌──────────────────┐
                 │         LPC1769          │◄────────►│ Dashboard Python │
 Sharp IR ──► Filtro ──► ADC ──DMA──► PID   │  CP2102  │  (gráfico, CSV)  │
 (5 V)     (LM358 + zener)      Timer1      │          └──────────────────┘
                 │                PWM ──────┼──► Optoacoplador ──► MOSFET IRLZ44N ──► Ventilador 12 V
                 │                          │     (aislación)         (diodo de protección)
                 │   tabla ──DMA──► DAC ────┼──► Seguidor (LM358) ──► Transistor ──► Buzzer
                 └──────────────────────────┘
```

| Señal | Pin LPC1769 | Conector LPCXpresso |
|---|---|---|
| PWM del ventilador | P2.0 (PWM1.1) | J6-42 |
| Entrada del ADC (altura) | P0.23 (AD0.0) | J6-15 |
| Salida del DAC (perturbación) | P0.26 (AOUT) | J6-18 |
| UART TX | P0.2 (TXD0) | J6-21 |
| UART RX | P0.3 (RXD0) | J6-22 |
| Masa | GND | J6-1 / J6-54 |

* Descripción del circuito y consideraciones de diseño:
  * Sensor: Sharp GP2Y0A21YK0F alimentado a 5 V, con capacitores de 10 µF y 100 nF junto al sensor (consume picos de corriente cada vez que pulsa su LED).
  * Acondicionamiento: filtro pasa-bajos activo Sallen-Key de ~16 Hz (R = 10 kΩ, C = 1 µF) con un LM358 como seguidor. Como el LM358 se alimenta a 5 V, la salida pasa por una resistencia de 1 kΩ y un zener de 3,3 V que limitan la tensión en la entrada del ADC.
  * Actuador: ventilador de 12 V / 0,26 A conmutado por un MOSFET de nivel lógico (IRLZ44N), con pull-down de 10 kΩ en el gate y diodo 1N4007 en antiparalelo con la carga.
  * Aislación: un optoacoplador PC817 separa la LPC (3,3 V) del circuito de 12 V. Las masas de ambos lados no están unidas. La velocidad del optoacoplador limita el PWM a 500 Hz.
  * Indicador de la perturbación: la salida del DAC pasa por la otra mitad del LM358, conectada como seguidor para no cargar el pin AOUT, y a través de una resistencia de 1 kΩ maneja la base de un transistor NPN que enciende un buzzer alimentado a 5 V.
  * Alimentación: fuente de 12 V / 2 A para el ventilador; la LPC1769 por USB; el sensor, el operacional y el buzzer con los 5 V del adaptador USB-UART.
  * Ver [docs/notas_diseno.md](docs/notas_diseno.md) para las decisiones de diseño, [docs/armado_del_tp.md](docs/armado_del_tp.md) para el armado y [hardware/Materiales.md](hardware/Materiales.md) para la lista de materiales.

### Arquitectura de Software (Firmware)
* Módulos:
  * `ventilador` — PWM1.1 en P2.0 a 500 Hz, duty en por mil (resolución de 0,1 %).
  * `comunicacionUart` — UART0 a 9600 8N1 con FIFO, recepción de comandos por interrupción.
  * `sensor` — ADC en modo burst (AD0.0, P0.23, ~1500 muestras/s) con GPDMA en un buffer circular de 64 muestras (~43 ms, un ciclo del sensor). La lectura es el promedio del buffer, convertido a mm con la tabla de calibración. El buffer está en la RAM AHB (RAM2).
  * `demora` — Timer0 contando en µs, como base de tiempo de la telemetría y de las demoras.
  * `perturbacion` — DAC con GPDMA (canal 1, memoria → DAC): una tabla de 1024 muestras con un período de senoidal se repite sin intervención de la CPU, y el contador del DAC fija la frecuencia. El PID lee el valor actual del DAC y lo suma al duty, multiplicado por la amplitud.
  * Timer1 — interrupción por match cada 40 ms, que ejecuta el lazo de control.
* Lazo de control (cada 40 ms):

```text
leer altura (mm) ──► rampa del setpoint ──► e = setpoint − altura
                                                │
        duty = u0 + Kp·e + I + D + W·seno  ◄────┘
        │   I: integral con anti-windup (no acumula si el duty está saturado)
        │   D: derivada sobre la medición, filtrada (Tf = 0,1 s)
        │   W·seno: perturbación del DAC (0 si está apagada)
        └──► limitar a 150–650 ‰ ──► PWM del ventilador
```

* Lazo de control: la interrupción del Timer1 (match 0, cada 40 ms) lee el promedio del ADC que dejó el DMA, lo convierte a mm y, en modo automático, calcula el PID y actualiza el PWM. Como el período lo marca el hardware, no se ve afectado por lo que haga el programa principal (por ejemplo, la espera mientras sale una línea por la UART).
* Programa principal: el `main` envía la telemetría cada 200 ms (con la base de tiempo del Timer0) y atiende los comandos que llegan por la UART.
* Modelo de la planta y diseño del PID: ver [docs/modelo_pid.md](docs/modelo_pid.md).

### Interfaz de Usuario (PC)
* [pc/dashboard.py](pc/dashboard.py): aplicación en Python (tkinter + matplotlib + pyserial).
  * Gráfico en tiempo real de la altura, el setpoint y el duty (últimos 30 s).
  * Panel para arrancar o detener el PID, fijar el duty manual y modificar los parámetros.
  * Consola para comandos manuales y respuestas de la LPC.
  * Grabación de los ensayos en CSV (`docs/ensayos/`).
* La lectura del puerto serie corre en un hilo aparte, de modo que la ventana no se bloquea. Los comandos se envían de a uno, cada 150 ms, porque la LPC descarta un comando nuevo mientras no procesó el anterior.

---

## 4. Especificaciones Eléctricas, Alimentación y Entorno

### Parámetros de Alimentación y Consumo
* Tensión de operación del sistema: 3,3 V (LPC1769), 5 V (sensor y operacional), 12 V (ventilador).
* Método de alimentación: LPC1769 por USB; fuente externa de 12 V para el ventilador.
* Consumo estimado:
  * Ventilador al máximo: 0,26 A a 12 V (dato del fabricante).
  * Sensor Sharp: ~30 mA a 5 V.

### Electrónica Digital III (Cortex-M / ARM)
* IDE y SDK: MCUXpresso IDE con CMSISv2p00_LPC17xx y los drivers LPC17xx de la cátedra.
* Microcontrolador principal: NXP LPC1769.
* Periféricos utilizados: ADC, DAC, GPDMA (canal 0: ADC → memoria; canal 1: memoria → DAC), PWM1, Timer0, Timer1, UART0, NVIC.
* Estrategia de concurrencia: bare-metal. El DMA llena el buffer del ADC y alimenta al DAC sin intervención de la CPU; el lazo de control corre en la interrupción del Timer1, con período fijo de 40 ms; la UART recibe los comandos por interrupción, con mayor prioridad que el Timer1; el programa principal envía la telemetría y procesa los comandos.

### Perturbaciones Exteriores y Condiciones del Entorno
La esfera de telgopor es muy liviana y el tubo está abierto en su extremo superior, por lo que el sistema es sensible al entorno. Durante los ensayos se identificaron las siguientes perturbaciones:

| Perturbación | Efecto observado | Cómo la compensa el sistema |
|---|---|---|
| Corrientes de aire en la habitación (puertas, ventanas, ventilación, personas cerca del tubo) | La esfera sube o baja de forma repentina | El término proporcional y el derivativo corrigen el desvío; en un ensayo con la esfera desplazada 5 cm, volvió al setpoint en 4,8 s |
| Temperatura y calentamiento del ventilador | El duty necesario para sostener la esfera aumentó a lo largo de un mismo día: de 46,5 % a ~56 % | El término integral compensa la diferencia; el duty de equilibrio (u0) se puede reajustar por UART sin recompilar |
| Variación de la tensión de alimentación | Cambia la velocidad del ventilador para un mismo duty | Igual que el caso anterior: integral y reajuste de u0 |
| Despegue desde la rejilla | Con la esfera apoyada, hace falta más aire para levantarla que para sostenerla. La integral juntaba ese esfuerzo extra y, al despegar, la esfera subía hasta el tope | Empujón inicial (u0 + 150 ‰ durante 300 ms), rampa del setpoint desde la altura real y reset de la integral cuando la esfera supera 50 mm |
| Zona ciega del sensor (esfera a menos de ~10 cm del sensor) | La curva del sensor se invierte y la esfera se lee más abajo de lo que está | El setpoint se limita a 10–30 cm para trabajar siempre en la zona útil |
| Ruido eléctrico del PWM y del sensor | Variaciones de la medición | Filtro activo, promedio de 64 muestras por DMA y filtro en el término derivativo |

Recomendaciones para los ensayos: realizarlos con puertas y ventanas cerradas y sin ventilación forzada, dejar el ventilador funcionando unos minutos antes de medir y reajustar u0 al duty promedio observado.

---

## 5. Proceso de Integración y Desarrollo

| Semana | Objetivo |
|---|---|
| 1 | Armado mecánico del tubo. Lectura del sensor por ADC y control del ventilador por PWM, por separado. |
| 2 | Acondicionamiento de señal, ADC con DMA y promediado. Comunicación UART y dashboard en Python. |
| 3 | Implementación y ajuste del PID. |
| 4 | Integración, ensayos (respuesta a escalón, perturbaciones) y documentación. |

* Etapa 1 (validación inicial): estructura de madera con tubo de 55 cm y etapa de potencia con MOSFET IRLZ44N. El primer ventilador (0,11 A) no levantaba la esfera y se reemplazó por uno de 0,26 A. Un transistor bipolar TIP31C no llegaba a saturar con la corriente de un pin de la LPC y se reemplazó por el MOSFET de nivel lógico.
* Etapa 2 (adquisición y comunicación): UART bidireccional con recepción por interrupción y protocolo de comandos. Se agregó un optoacoplador para aislar la LPC del circuito de 12 V, lo que obligó a bajar el PWM de 20 kHz a 500 Hz. Lectura del sensor con ADC + DMA en buffer circular, calibración a mm y dashboard en Python con registro de ensayos en CSV.
* Etapa 3 (integración lógica): PID en mm funcionando en el equipo, con derivada sobre la medición y filtrada, anti-windup, rampa de setpoint y empujón de arranque. Las ganancias se ajustaron experimentalmente a partir del modelo de la planta.
* Etapa 4 (sistema completo): sistema integrado y probado sobre el equipo, con más de 30 ensayos registrados (calibración, regulación, arranque, escalones de setpoint y perturbaciones, incluida la del DAC). En esta etapa el lazo de control pasó a la interrupción del Timer1, se repitió la calibración del sensor y se activó la perturbación con el DAC. Durante la integración se resolvieron, entre otros, los siguientes problemas:

| Problema | Causa | Solución |
|---|---|---|
| La UART perdía caracteres | El driver deja la FIFO deshabilitada | Habilitar la FIFO con `UART_FIFOConfig` |
| El DMA del ADC copiaba sin esperar conversiones nuevas | El pedido de DMA del canal 0 se apaga al leer ADDR0, no ADGDR | El DMA lee el registro ADDR0 |
| Fallaba la grabación de la flash | El DMA escribía en la RAM donde MCUXpresso carga el programa de grabación | Buffer del DMA en la RAM2; recuperación de las placas con el modo ISP |
| La esfera quedaba pegada arriba | Zona ciega del sensor Sharp | Límite del setpoint a 10–30 cm |
| Medición incorrecta del sensor | Operacional dañado por una conexión errónea | Reemplazo del LM358 y verificación de cada etapa con el tester |
| La esfera quedaba más abajo de lo que indicaba el gráfico | Después de los cambios en el acondicionamiento, la tabla de calibración quedó desplazada | Nueva calibración, con la lectura cruda del ADC agregada a la telemetría |
| La lectura del ADC quedaba fija en un valor | El ADC o el DMA quedaban detenidos después de grabar con el debugger | Reinicio completo de la placa (desconectar el USB) después de grabar |
| El período del PID variaba | El envío de la telemetría por UART bloquea al programa principal ~25 ms | El PID pasó a la interrupción del Timer1 |
| El buzzer sonaba antes de que corriera el programa | P0.26 arranca con el pull-up interno activado y el seguidor copia 3,3 V a la base del transistor | Al inicializar el DAC el pin queda en 0 V |

El seguimiento semanal del equipo está en la carpeta [scrum/](scrum/).

---

## 6. Ensayos, Pruebas y Resultados
* Pruebas funcionales realizadas:
  * Duty de equilibrio (PWM de 20 kHz, sin optoacoplador): con 68,0 % la esfera cae, con 68,5 % flota al 30–40 % del tubo y con 70,0 % sube hasta arriba. El rango útil del duty es muy estrecho, lo que justifica el control en lazo cerrado.
  * PWM con optoacoplador, verificado con osciloscopio sobre el drain del MOSFET: onda cuadrada de 499,99 Hz con el duty correcto y sobretensión al apagado limitada por el diodo.
  * Curva de calibración del sensor (esfera a alturas conocidas, ventilador apagado):

    | Altura [cm] | 0 | 5 | 10 | 15 | 20 | 25 | 30 | 35 | 40 |
    |---|---|---|---|---|---|---|---|---|---|
    | ADC | 1820 | 1972 | 2038 | 2157 | 2307 | 2442 | 2593 | 2731 | 2761 |

    Entre 0 y 35 cm la curva es aproximadamente lineal (~27 cuentas/cm) y el ruido de la medición es de ±1,3 cm. Entre 35 y 40 cm la lectura casi no cambia (límite del sensor), por lo que el setpoint se limita a 10–30 cm. La calibración se repitió el 12/11 porque, después de los cambios en el acondicionamiento, la tabla anterior indicaba la pelota varios centímetros más arriba de su posición real; para registrar los valores del ADC se agregó el campo `a=` a la telemetría.
* Ensayos del 11/11, con la calibración anterior (u0 = 50,5 %, Kp = 0,30, Ki = 0,020, Kd = 0,36, en ‰ por mm; PID en el programa principal):
  * Regulación (setpoint 175 mm): altura media 175,5 mm y desvío estándar 6,8 mm. La esfera permanece el 87 % del tiempo dentro de ±1 cm y el 99 % dentro de ±2 cm.
  * Perturbación externa (la esfera se desplazó 5 cm hacia abajo): vuelve al setpoint en 4,8 s, con un sobrepico de 1,9 cm.
  * Escalón de setpoint de 7,5 cm (175 → 250 → 175 mm): la esfera llega al nuevo valor en ~4,5 s, con un sobrepico de ~4 cm.

* Ensayos del 12/11, con la calibración nueva y la versión final del firmware (u0 = 53,5 %, Kp = 0,30, Ki = 0,030, Kd = 0,25; PID en la interrupción del Timer1; rampa de 20 mm/s):
  * Ajuste de Kd (setpoint 200 mm): con Kd = 0,36 el desvío fue de 19,3 mm (75 % del tiempo dentro de ±2 cm); con Kd = 0,25, 15,1 mm (84 %); con Kd = 0,18, 18,4 mm (75 %). Se adoptó Kd = 0,25: el duty varía menos entre muestras y la derivada todavía amortigua el movimiento de la esfera.
  * Arranque con la esfera apoyada en la rejilla (setpoint 200 mm): antes de los cambios en el arranque, la esfera subía hasta el tope (350 mm) y tardaba ~30 s en estabilizarse. Con la rampa desde la altura real y el reset de la integral al despegar, el máximo fue de 263 mm y se estabilizó en ~15 s.
  * Regulación (setpoint 200 mm): altura media 195 mm y desvío estándar 20,2 mm. La esfera permanece el 36 % del tiempo dentro de ±1 cm y el 67 % dentro de ±2 cm.
  * Escalón de 10 cm con rampa (150 → 250 → 150 mm): en la subida la esfera sigue la rampa con una diferencia de ~2 cm, queda dentro de ±2 cm a los 5,7 s y llega a un máximo de 274 mm (+24 mm). En la bajada desciende hasta 105 mm (−45 mm) y no se mantiene dentro de ±2 cm, porque a 150 mm la regulación es peor (desvío de 34 mm con el setpoint fijo).
  * Escalón de 10 cm con rampa (200 → 300 → 200 mm): la esfera se atrasa respecto de la rampa, la integral acumula ese atraso y luego se pasa: llega a 350 mm al subir y a 103 mm al bajar. Cerca de 300 mm el sensor está próximo a su límite de medición.
  * Perturbación senoidal con el DAC (W = 50 ‰, setpoint 200 mm). La amplitud de la oscilación de la esfera se obtuvo de la componente de la altura a la frecuencia de la perturbación:

    | Frecuencia | Oscilación del duty | Oscilación de la esfera |
    |---|---|---|
    | Sin perturbación | — | 2–9 mm (ruido de la medición) |
    | 0,5 Hz | ±45 ‰ | ±6–13 mm |
    | 1 Hz | ±55 ‰ | ±6 mm |
    | 2 Hz | ±35–43 ‰ | ±4–5 mm |

    A medida que aumenta la frecuencia, la esfera se mueve menos: el ventilador y la esfera no alcanzan a seguir cambios rápidos del duty. Aun a 0,5 Hz, el desplazamiento causado por la perturbación es menor que la variación propia de la regulación. Durante el ensayo la lectura del ADC no se vio afectada por el DAC.
  * Limitación principal: después de los cambios en el acondicionamiento, la sensibilidad del sensor bajó de ~48 a ~27 cuentas/cm y el ruido de la medición entre muestras pasó de ~3,5 mm a ~10 mm. Por eso la regulación del 12/11 es peor que la del 11/11, aunque el control sea el mismo.

* Registros de todos los ensayos en [docs/ensayos/](docs/ensayos/), en formato CSV (tiempo, setpoint, altura, duty y, desde el 12/11, la lectura del ADC).

* Cumplimiento de los criterios de aceptación:

| # | Criterio | 11/11 (calibración anterior) | 12/11 (versión final) |
|---|---|---|---|
| 1 | Error de ±2 cm en régimen permanente | Cumple: 99 % del tiempo dentro de ±2 cm | No cumple: 67 % del tiempo dentro de ±2 cm, por el ruido de la medición |
| 2 | Sobrepico menor al 10 % ante un escalón | No cumple: ~50 % | No cumple: 24 % en la subida 150 → 250 mm (dentro de la variación propia de la regulación) y 45 % en la bajada |
| 3 | Establecimiento menor a 6 s | Llega al nuevo valor en ~4,5 s; el sobrepico prolonga el establecimiento | Cumple en la subida (5,7 s); no se establece en la bajada a 150 mm |
| 4 | Setpoint entre 10 y 30 cm desde la PC | Cumple: comando `S`, limitado a 100–300 mm | Cumple; la regulación es peor en los extremos del rango |
| 5 | Vuelve al setpoint tras una perturbación | Cumple: 4,8 s ante un desplazamiento de 5 cm | Cumple ante la perturbación del DAC: a 0,5 Hz la esfera se mueve ±6–13 mm alrededor del setpoint |
| 6 | Comunicación bidireccional | Cumple: comandos, respuestas y telemetría por UART | Cumple |

---

## 7. Estructura del Repositorio

```text
├── firmware/          # Código fuente del proyecto (MCUXpresso)
│   ├── src/           # Archivos de código (.c)
│   └── inc/           # Archivos de cabecera (.h)
├── pc/                # Dashboard en Python
├── hardware/          # Lista de materiales
├── docs/              # Modelo y diseño del PID, guía de armado, notas de diseño
│   └── ensayos/       # Registros de los ensayos (CSV)
├── scrum/             # Weekly, review y retrospectiva del equipo
└── README.md
```
