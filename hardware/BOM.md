# Lista de materiales (BOM)

| Componente | Cant. | Detalle |
|---|---|---|
| Sensor Sharp GP2Y0A21YK0F | 1 | Con su cable JST de 3 pines (suele venderse aparte). Rango 10–80 cm, alimentación 5 V. |
| Ventilador / blower 12 V | 1 | El actual es de 12 V / 0,26 A. El CGF-5010S-12 (0,11 A) no levanta la pelota. Si no alcanza, usar un centrífugo más potente (9733 o 7530). Si tiene 4 cables, usar la entrada PWM. |
| MOSFET IRLZ44N | 2 | De nivel lógico (**IRL**, no IRF): conduce bien con 3,3 V en el gate. Uno de repuesto. Alternativa: TIP122 con 470 Ω en la base. **El TIP31C no sirve** (no satura con la corriente del pin). |
| Diodo 1N4007 o 1N5819 | 1 | En antiparalelo con el blower (raya hacia +12 V). No usar un zener. |
| Zener 1N4728A (3,3 V) | 1 | Opcional, para proteger la entrada del ADC. **No** sirve como diodo de flyback. |
| Operacional MCP6002-I/P DIP-8 (o LMV358) | 1 | Rail-to-rail, alimentado a 3,3 V. Reemplaza al LM358 (ver notas de diseño). |
| Capacitores 10 µF / 25 V | 2 | Desacople del sensor y de la fuente de 12 V. |
| Capacitores 100 nF (104) | 4 | Desacople. |
| Capacitores 1 µF (105) | 2 | Filtro Sallen-Key del sensor. |
| Resistencia 220 Ω | 1 | Gate del MOSFET. |
| Resistencia 10 kΩ | 3 | Pull-down del gate y filtro (R1, R2). |
| Resistencia 1 kΩ | 1 | Entre el filtro y P0.23. |
| Fuente 12 V / 2 A | 1 | Masa común con la LPC1769. |
| Tubo transparente | 1 | 45–50 mm de diámetro interno, ≤ 80 cm de largo útil. |
| Pelotas de ping-pong | varias | Se abollan. |
| Adaptador USB–UART (CP2102) | 1 | Su pin de 5 V alimenta al sensor. |
| Buzzer piezo o LED + 330 Ω | 1 | Salida del DAC. |
| Mecánica | — | Rejilla de cooler, tanza de 0,2–0,3 mm, soporte del sensor, cinta o silicona. |
| Opcionales | — | Jack DC 5,5 × 2,1 mm. |

## Pendiente de compra

IRLZ44N (×2), resistencias de 220 Ω, Sharp GP2Y0A21YK0F + cable JST, MCP6002-I/P, 2× 10 µF/25 V, 4× 100 nF, 2× 1 µF y la mecánica (rejilla, tanza, soporte, cinta/silicona).
