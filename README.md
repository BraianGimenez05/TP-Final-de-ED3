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
El sistema mantiene una esfera (pelota de telgopor) suspendida en el aire dentro de un conducto vertical transparente. Un ventilador ubicado en la base del tubo genera el flujo de aire, y un sensor de distancia infrarrojo ubicado en el tope de la estructura del tubo mide la altura real de la esfera. Una LPC1769 ejecuta un lazo de control PID que ajusta la potencia del ventilador para estabilizar la esfera en la altura de consigna (setpoint) indicada por el usuario.

Desde un dashboard en Python en la PC, comunicado por UART, el usuario fija el setpoint, ajusta las constantes del PID y observa la respuesta del sistema en tiempo real. Está pensado como planta didáctica para estudiar control realimentado: respuesta a escalón, rechazo de perturbaciones y efecto de cada constante del PID.

### Alcances del Proyecto
* El sistema es capaz de:
  * Medir la altura de la esfera con un sensor IR analógico, acondicionado y adquirido por ADC + DMA con promediado.
  * Mantener la esfera en una altura de consigna mediante un PID periódico (disparado por Timer) que actúa sobre el ventilador por PWM.
  * Recibir setpoint y constantes Kp, Ki, Kd por UART y enviar telemetría (altura, setpoint, error, salida PWM).
  * Generar con DAC + DMA una señal de perturbación (senoidal precalculada) para evaluar el rechazo del lazo.
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
| 4 | El setpoint se puede fijar entre 10 y 40 cm desde la PC | Comandos por UART |
| 5 | La esfera vuelve al setpoint luego de una perturbación (golpe de aire o señal del DAC) | Ensayo de perturbación |
| 6 | La comunicación es bidireccional: la PC envía comandos y la LPC responde y envía telemetría | Dashboard / terminal serie |

### Posibles Etapas Siguientes
* Reemplazar el sensor Sharp por un sensor más preciso y con menor tiempo de actualización.
* Autotuning del PID (método de relé / Ziegler-Nichols) y control por modelo.
* Pasar el circuito de protoboard a PCB.
* Guardar en flash interna las constantes del PID y la calibración del sensor.

---

## 2. Protocolo de Comunicación

* Interfaz: UART0 de la LPC1769 (TX = P0.2, RX = P0.3) a través de un adaptador USB-UART CP2102.
* Configuración: 9600 baudios, 8 bits de datos, sin paridad, 1 bit de stop (8N1).
* Formato: comandos en texto ASCII. Cada comando termina con `\r` o `\n` (tecla Enter). Máximo 15 caracteres por comando.
* Respuestas: texto ASCII terminado en `\r\n`.

### Comandos implementados

| Comando | Descripción | Ejemplo | Respuesta |
|---|---|---|---|
| `Dxxx` | Fija el duty del ventilador en `xxx` por mil (0–1000 = 0,0 %–100,0 %). Los valores mayores a 1000 se limitan a 1000 | `D685` | `OK duty=685` |
| `?` | Consulta el duty actual | `?` | `duty=685` |
| (otro) | Comando no reconocido | `X` | `Comando desconocido` |

### Comandos planificados

| Comando | Descripción |
|---|---|
| `Sxx` | Fija el setpoint de altura en cm |
| `Px` / `Ix` / `Kx` | Ajusta las constantes Kp, Ki y Kd del PID |
| `M` / `A` | Cambia entre modo manual (duty fijo) y automático (PID) |
| Telemetría | La LPC envía periódicamente altura, setpoint, error y duty para el dashboard |

---

## 3. Arquitectura del Sistema

### Hardware & Interconexión
* Diagrama de bloques: *(pendiente)* `docs/diagrama_bloques.png`

```text
                 ┌───────────────┐   UART   ┌──────────────────┐
                 │   LPC1769     │◄────────►│ Dashboard Python │
 Sharp IR ──►Filtro/escalado──►ADC─DMA─►PID │                  │
 (5 V)       (operacional)     │  │         └──────────────────┘
                 │  PWM ───────┼──┼──► Opto ──► MOSFET IRLZ44N ──► Ventilador 12 V
                 │  DAC◄─DMA   │                    (diodo de protección)
                 └──┬──────────┘
                    └──► Perturbación / señal acústica
```

* Esquemático del circuito: *(pendiente, KiCad)* `![Esquemático Completo](hardware/esquematico.png)`
* Descripción del circuito y consideraciones de diseño:
  * Sensor: Sharp GP2Y0A21YK0F alimentado a 5 V, con capacitor de 10 µF o más junto al sensor (consume picos de corriente).
  * Acondicionamiento: filtro pasa-bajos activo Sallen-Key (~16 Hz) con seguidor. El operacional no debe superar nunca los 3,3 V en la entrada del ADC (MCP6002 rail-to-rail a 3,3 V, o LM358 a 5 V con resistencia serie y zener de 3,3 V de protección).
  * Actuador: ventilador de 12 V / 0,26 A conmutado por MOSFET de nivel lógico (IRLZ44N) a través de un optoacoplador, con pull-down de 10 kΩ en el gate y diodo en antiparalelo con la carga. PWM a 500 Hz, limitado por la velocidad del optoacoplador.
  * Alimentación: fuente de 12 V / 2 A para el ventilador.
  * Ver [docs/notas_diseno.md](docs/notas_diseno.md) para las decisiones de diseño y [hardware/Materiales.md](hardware/Materiales.md) para la lista de materiales.

### Arquitectura de Software (Firmware)
* Módulos:
  * `ventilador` — PWM1.1 en P2.0, duty en por mil.
  * `comunicacionUart` — UART0 a 9600 8N1, recepción de comandos por interrupción.
  * `sensor` — lectura del ADC (AD0.0, P0.23).
* Diagrama de flujo o máquina de estados: *(pendiente)* `![Máquina de Estados](docs/diagrama_software.png)`
* Modelo de la planta y diseño del PID: ver [docs/modelo_pid.md](docs/modelo_pid.md).
* Armado y medición de parámetros: ver [docs/armado_del_tp.md](docs/armado_del_tp.md).

---

## 4. Especificaciones Eléctricas, Alimentación y Entorno

### Parámetros de Alimentación y Consumo
* Tensión de operación del sistema: 3,3 V (LPC1769), 5 V (sensor), 12 V (ventilador).
* Método de alimentación: LPC1769 por USB; fuente externa de 12 V para el ventilador.
* Consumo estimado o medido:
  * Ventilador al máximo: 0,26 A a 12 V (dato del fabricante).
  * Sensor Sharp: ~30 mA a 5 V.

### Electrónica Digital III (Cortex-M / ARM)
* IDE y SDK: MCUXpresso IDE con CMSISv2p00_LPC17xx y los drivers LPC17xx de la cátedra.
* Microcontrolador principal: NXP LPC1769.
* Periféricos utilizados: ADC, GPDMA (ADC→memoria y memoria→DAC), DAC, PWM1, Timer, UART, SysTick, NVIC.
* Estrategia de concurrencia: bare-metal dirigido por interrupciones. El Timer marca el período del lazo de control (~25 Hz, limitado por la actualización del sensor de ~40 ms); el DMA llena el buffer del ADC sin intervención de la CPU; la UART recibe comandos por interrupción y el programa principal los ejecuta.

---

## 5. Proceso de Integración y Desarrollo

| Semana | Objetivo |
|---|---|
| 1 | Armado mecánico del tubo. Lectura del sensor por ADC y control del ventilador por PWM, por separado. |
| 2 | Acondicionamiento de señal, ADC con DMA y promediado. Comunicación UART y dashboard en Python. |
| 3 | Implementación y ajuste del PID. DAC + DMA para la perturbación. |
| 4 | Integración, ensayos (respuesta a escalón, perturbaciones) y documentación. |

* Etapa 1 (validación inicial): estructura de madera con tubo de 55 cm, etapa de potencia con IRLZ44N y PWM funcionando. El primer ventilador (0,11 A) no levantaba la esfera y se reemplazó por uno de 0,26 A. Duty de equilibrio medido: u0 ≈ 68,5 %.
* Etapa 2 (adquisición y comunicación): UART bidireccional a 9600 baudios con recepción por interrupción y comandos `Dxxx` / `?`. Se agregó un optoacoplador en la etapa de potencia, lo que obligó a bajar el PWM a 500 Hz. Lectura del sensor por ADC en prueba.
* Etapa 3 (integración lógica): *(completar)*
* Etapa 4 (sistema completo): *(completar)*

El seguimiento semanal del equipo está en la carpeta [scrum/](scrum/).

---

## 6. Ensayos, Pruebas y Resultados
* Pruebas funcionales realizadas:
  * Duty de equilibrio: con 68,0 % la esfera cae, con 68,5 % flota al 30–40 % del tubo y con 70,0 % sube hasta arriba.
  * *(pendiente)* Curva de calibración del sensor (tensión vs. altura medida con regla).
  * *(pendiente)* Respuesta a escalón de setpoint: sobrepico, tiempo de establecimiento, error en estado estacionario.
  * *(pendiente)* Rechazo de perturbaciones generadas con el DAC.
* Evidencia fotográfica y gráficos:
  * *Capturas de instrumental:* *(osciloscopio: salida del filtro, PWM, DAC)*
  * *Foto del Prototipo Real:* *(pendiente)*

---

## 7. Estructura del Repositorio

```text
├── firmware/          # Código fuente del proyecto (MCUXpresso)
│   ├── src/           # Archivos de código (.c)
│   └── inc/           # Archivos de cabecera (.h)
├── hardware/          # Esquemáticos (KiCad), PDF/imagen y BOM
├── docs/              # Modelo y diseño del PID, guía de armado, notas de diseño
├── scrum/             # Weekly, review y retrospectiva del equipo
└── README.md
```
