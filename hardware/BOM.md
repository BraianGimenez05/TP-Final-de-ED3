# Lista de materiales (BOM)

| Componente | Cant. | Detalle |
|---|---|---|
| Sensor Sharp GP2Y0A21YK0F | 1 | Con su cable JST (suele venderse aparte). Rango 10–80 cm, alimentación 5 V. |
| Blower 12 V | 1 | Tipo 5015 o 7530. Revisar la corriente (0,3–1 A). Si tiene 4 cables, usar la entrada PWM. |
| MOSFET IRLZ44N | 1 | De nivel lógico (**IRL**, no IRF): conduce bien con 3,3 V en el gate. |
| Diodo 1N4007 o 1N5819 | 1 | En antiparalelo con el blower. |
| Operacional MCP6002 (o TLV2372) | 1 | Rail-to-rail, alimentado a 3,3 V. Reemplaza al LM358 (ver notas de diseño). |
| Capacitores 10 µF | 2 | Desacople del sensor y de la fuente. |
| Capacitores 100 nF | 2–4 | Desacople y filtro. |
| Resistencia 220 Ω | 1 | Gate del MOSFET. |
| Resistencia 10 kΩ | 1 | Pull-down del gate. |
| Resistencias / capacitores del filtro | — | Valores según el diseño del filtro. |
| Fuente 12 V, 1–2 A | 1 | Masa común con la LPC1769. |
| Tubo transparente | 1 | 45–50 mm de diámetro interno, ≤ 80 cm de largo útil. |
| Pelotas de ping-pong | varias | Se abollan. |
| Adaptador USB–UART 3,3 V | 1 | Si la placa no tiene uno integrado. |
