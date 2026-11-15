# Lista de materiales

| Componente | Cant. | Detalle | Estado |
|---|---|---|---|
| LPC1769 (LPCXpresso) + cable USB | 1 | Microcontrolador | disponible |
| Sensor Sharp GP2Y0A21YK0F | 1 | Rango 10–80 cm, alimentación 5 V. Cable JST de 3 pines | disponible |
| Ventilador 12 V / 0,26 A | 1 | Con el montaje final la pelota flota con ~46–56 % de duty, según la temperatura (u0 = 53,5 % en el firmware). El CGF-5010S-12 (0,11 A) se descartó por falta de caudal | disponible |
| MOSFET 2SK3569 (K3569) | 1 | Canal N. Es el MOSFET del montaje final; el gate recibe ~11 V a través del optoacoplador | disponible |
| MOSFET IRLZ44N | 2 | De nivel lógico. Se usó en las primeras etapas (gate excitado directo desde la LPC); en el montaje final se reemplazó por el 2SK3569. El TIP31C se descartó | disponible |
| Optoacoplador (PC817 o similar) | 1 | Entre P2.0 y el gate. Limita el PWM a 500 Hz; un 6N137 permitiría 20 kHz | disponible |
| Diodo 1N4007 | 1 | En antiparalelo con el ventilador. Un 1N5819 es preferible | disponible |
| Zener 1N4728A (3,3 V) | 1 | Protección de la entrada del ADC en una primera versión. No se usa en el montaje final | no usado |
| Operacional LM358 | 1 | Alimentado a 5 V. Mitad A: filtro Sallen-Key del sensor; mitad B: seguidor de la salida del DAC. Alternativa preferida: MCP6002 o LMV358 a 3,3 V | disponible |
| Fuente 12 V / 2 A | 1 | Alimentación del ventilador | disponible |
| Adaptador USB-UART CP2102 | 1 | UART0 y 5 V para el sensor y el operacional | disponible |
| Capacitores 10 µF / 25 V | 2 | Desacople del sensor y de la fuente de 12 V | |
| Capacitores 100 nF (104) | 4 | Desacople | |
| Capacitores 1 µF (105) | 2 | Filtro Sallen-Key | |
| Resistencias 220 Ω | 1 | Gate del MOSFET en la conexión directa (primeras pruebas). No se usa en el montaje final | no usado |
| Resistencia 470 Ω | 1 | Entrada del optoacoplador | |
| Resistencia 100 Ω | 1 | Gate del MOSFET (con optoacoplador) | |
| Resistencias 10 kΩ | 3 | Pull-down del gate y filtro (R1, R2) | |
| Resistencia 1 kΩ | 1 | Entre el filtro y P0.23 | |
| Buzzer + transistor NPN | 1 | Indicador en la salida del DAC en versiones anteriores. Se retiró cuando el DAC pasó a sacar la altura en tensión; la alarma usa el LED rojo de la placa (P0.22) | no usado |
| Tubo transparente | 1 | 55 cm de largo | disponible |
| Pelota de telgopor | varias | Reemplazó a la pelota de ping-pong (40 mm, 2,7 g). Se probó agregarle peso y se descartó | disponible |
| Estructura | — | Dos placas de madera y cuatro varillas roscadas con tuercas | disponible |
| Rejilla | 1 | Entre el ventilador y el tubo | |
| Protoboard y cables Dupont | — | | disponible |

"no usado" marca los componentes que se usaron en pruebas o versiones anteriores y no están en el montaje final. Los casilleros vacíos de la columna Estado corresponden a componentes pasivos de uso general.
