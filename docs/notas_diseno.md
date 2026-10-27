# Notas de diseño

## Operacional: no usar el LM358 a 3,3 V
El LM358 no es rail-to-rail: su salida no llega a menos de ~1,5 V de la alimentación.
- A 3,3 V la salida no pasa de ~1,8 V → se pierde casi la mitad del rango del ADC.
- A 5 V llega a ~3,5 V → supera los 3,3 V máximos del pin del ADC y puede dañar la placa.

**Decisión:** MCP6002 (o TLV2372) alimentado a 3,3 V. Mismo encapsulado de 8 pines.
Si hay que usar el LM358, alimentarlo a 5 V y proteger la entrada del ADC con un zener de 3,3 V.

## Sensor Sharp GP2Y0A21YK0F
- Por debajo de ~10 cm la curva se invierte: una pelota muy cerca da la misma tensión que una lejana.
  Si la pelota sube demasiado, el PID cree que está abajo, sopla más y la pelota queda pegada arriba.
  → Montar el sensor 10–12 cm por encima del borde del tubo, o limitar el setpoint máximo por software.
- Rango útil: 10–80 cm.
- Alimentación 5 V, capacitor de 10 µF o más junto al sensor (consume picos de corriente).
- Actualiza una medición cada ~40 ms → el lazo de control corre a ~25 Hz.
- Curva no lineal → calibrar midiendo la tensión a alturas conocidas y armar una tabla o ajustar una curva.

## DAC: perturbación física
Si la senoidal del DAC se suma solo dentro del software, el DAC no hace falta. Opciones para que la perturbación sea física:
1. Sumar la salida del DAC a la señal del sensor con el operacional como sumador (ruido de medición real, casi sin costo extra).
2. DAC → transistor → segundo ventilador chico que sopla de costado por un agujero del tubo (perturbación real sobre la pelota).
3. Alternativa más simple: señal acústica proporcional al error (DAC → buzzer pasivo o parlante chico).

## Blower
- Con 4 cables: usar la entrada PWM directamente.
- Con 2 cables: PWM al MOSFET a ~20–25 kHz para que no zumbe.
- Diodo en antiparalelo con la carga, masa común con la LPC1769.
