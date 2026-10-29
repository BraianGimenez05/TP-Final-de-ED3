# Sistema de Levitación Neumática en Lazo Cerrado
> **Asignatura:** Electrónica Digital III - Universidad Nacional de Córdoba
> **Grupo:** C2-G11
> **Integrantes:**
> * Braian Gimenez
> * Nombre Apellido
> **Profesor:** Marcos Blasco

---

## 🚀 1. Descripción General del Proyecto
El sistema mantiene una esfera (pelota de ping-pong) suspendida en el aire dentro de un conducto vertical transparente. Una turbina (blower) ubicada en la base genera el flujo de aire, y un sensor de distancia infrarrojo ubicado en el tope del tubo mide la altura real de la esfera. Una LPC1769 ejecuta un lazo de control PID que ajusta la potencia de la turbina para estabilizar la esfera exactamente en la altura de consigna (setpoint) indicada por el usuario.

Desde un dashboard en Python en la PC, comunicado por UART, el usuario fija el setpoint, ajusta las constantes del PID y observa la respuesta del sistema en tiempo real. Está pensado como planta didáctica para estudiar control realimentado: respuesta a escalón, rechazo de perturbaciones y efecto de cada constante del PID.

### 🎯 Alcances del Proyecto
* **El sistema SÍ es capaz de:**
  * Medir la altura de la esfera con un sensor IR analógico, acondicionado y adquirido por ADC + DMA con promediado.
  * Mantener la esfera en una altura de consigna mediante un PID periódico (disparado por Timer) que actúa sobre la turbina por PWM.
  * Recibir setpoint y constantes Kp, Ki, Kd por UART y enviar telemetría (altura, setpoint, error, salida PWM).
  * Generar con DAC + DMA una señal de perturbación (senoidal precalculada) para evaluar el rechazo del lazo.
* **El sistema NO incluye (fuera de alcance):**
  * Conectividad inalámbrica (Wi-Fi / Bluetooth).
  * Almacenamiento local de datos (SD / flash).
  * Autoajuste (autotuning) de las constantes del PID.

### ⏩ Posibles Etapas Siguientes
* Reemplazar el sensor Sharp por un sensor ToF (VL53L0X) más preciso y con menor tiempo de actualización.
* Autotuning del PID (método de relé / Ziegler-Nichols) y control por modelo.
* Pasar el circuito de protoboard a PCB.
* Guardar en flash interna las constantes del PID y la calibración del sensor.

---

## 📐 2. Arquitectura del Sistema

### 🔌 Hardware & Interconexión
* **Diagrama de Bloques:** *(pendiente)* `docs/diagrama_bloques.png`

```text
                 ┌───────────────┐   UART   ┌──────────────────┐
                 │   LPC1769     │◄────────►│ Dashboard Python │
 Sharp IR ──►Filtro/escalado──►ADC─DMA─►PID │                  │
 (5 V)       (MCP6002, 3,3 V)  │  │         └──────────────────┘
                 │  PWM ───────┼──┼──► MOSFET IRLZ44N ──► Blower 12 V
                 │  DAC◄─DMA   │                (diodo de protección)
                 └──┬──────────┘
                    └──► Perturbación / señal acústica
```

* **Esquemático del Circuito:** *(pendiente, KiCad)* `![Esquemático Completo](hardware/esquematico.png)`
* **Descripción del Circuito y Consideraciones de Diseño:**
  * **Sensor:** Sharp GP2Y0A21YK0F alimentado a 5 V, con capacitor de 10 µF o más junto al sensor (consume picos de corriente).
  * **Acondicionamiento:** filtro pasa-bajos activo y escalado de la salida del sensor (0,4–3,1 V) al rango del ADC (0–3,3 V). Operacional rail-to-rail alimentado a 3,3 V para no superar nunca la tensión máxima del pin del ADC.
  * **Actuador:** blower 12 V conmutado por MOSFET de nivel lógico (IRLZ44N), resistencia de gate de 220 Ω, pull-down de 10 kΩ y diodo en antiparalelo con la carga. PWM a ~20–25 kHz para evitar zumbido audible.
  * **Alimentación:** fuente de 12 V para el blower, masa común con la LPC1769.
  * Ver [docs/notas_diseno.md](docs/notas_diseno.md) para las decisiones de diseño y [hardware/BOM.md](hardware/BOM.md) para la lista de materiales.

### 💻 Arquitectura de Software (Firmware)
* **Diagrama de Flujo o Máquina de Estados:** *(pendiente)* `![Máquina de Estados](docs/diagrama_software.png)`
* **Modelo de la planta y diseño del PID:** ver [docs/modelo_pid.md](docs/modelo_pid.md).
* **Armado y medición de parámetros:** ver [docs/armado_del_tp.md](docs/armado_del_tp.md).

---

## ⚡ 3. Especificaciones Eléctricas, Alimentación y Entorno

### 🔌 Parámetros de Alimentación y Consumo
* **Tensión de operación del sistema:** 3,3 V (LPC1769 y acondicionamiento), 5 V (sensor), 12 V (blower).
* **Método de alimentación:** LPC1769 por USB; fuente externa de 12 V para el blower.
* **Consumo estimado o medido:**
  * En modo activo (blower al máximo): `XX mA` *(a medir)*

### 📌 Electrónica Digital III (Cortex-M / ARM)
* **IDE y SDK:** MCUXpresso IDE [vX.XX] con CMSIS + drivers LPC17xx.
* **Microcontrolador Principal:** NXP LPC1769.
* **Bibliotecas de Terceros y Versiones:** *(completar)*
* **Periféricos Avanzados Utilizados:** ADC, GPDMA (ADC→memoria y memoria→DAC), DAC, PWM1, Timer, UART, SysTick, NVIC.
* **Estrategia de Concurrencia:** bare-metal dirigido por interrupciones. El Timer marca el período del lazo de control (~25 Hz, limitado por la actualización del sensor de ~40 ms); el DMA llena el buffer del ADC sin intervención de la CPU; la UART recibe comandos y envía telemetría.

---

## 🔄 4. Proceso de Integración y Desarrollo

| Semana | Objetivo |
|---|---|
| 1 | Armado mecánico del tubo. Lectura del sensor por ADC y control del blower por PWM, por separado. |
| 2 | Acondicionamiento de señal, ADC con DMA y promediado. Comunicación UART y dashboard en Python. |
| 3 | Implementación y ajuste del PID. DAC + DMA para la perturbación. |
| 4 | Integración, ensayos (respuesta a escalón, perturbaciones) y documentación. |

* **Etapa 1 (Validación inicial):** *(completar)*
* **Etapa 2 (Adquisición/Comunicación):** *(completar)*
* **Etapa 3 (Integración lógica):** *(completar)*
* **Etapa 4 (Sistema Completo):** *(completar)*

---

## 📊 5. Ensayos, Pruebas y Resultados
* **Pruebas Funcionales Realizadas:** *(completar)*
  * Curva de calibración del sensor (tensión vs. altura medida con regla).
  * Respuesta a escalón de setpoint: sobrepico, tiempo de establecimiento, error en estado estacionario.
  * Rechazo de perturbaciones generadas con el DAC.
* **Evidencia Fotográfica y Gráficos:**
  * *Capturas de instrumental:* *(osciloscopio: salida del filtro, PWM, DAC)*
  * *Foto del Prototipo Real:* *(pendiente)*

---

## 📂 6. Estructura del Repositorio

```text
├── firmware/          # Código fuente del proyecto (MCUXpresso)
│   ├── src/           # Archivos de código (.c)
│   └── inc/           # Archivos de cabecera (.h)
├── hardware/          # Esquemáticos (KiCad), PDF/imagen y BOM
├── docs/              # Datasheets, imágenes del README, notas de diseño
└── README.md
```
