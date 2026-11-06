# Notas de diseño

Decisiones de diseño y problemas encontrados durante el armado, con la solución adoptada en cada caso.

## Ventilador

El primer ventilador probado, un CGF-5010S-12 (axial de 50 mm, 12 V / 0,11 A), no levanta la pelota ni conectado directamente a 12 V. Se estimó que hace falta una presión de ~21 Pa sobre la pelota. Se reemplazó por un ventilador de 12 V / 0,26 A, que con 100 % de duty saca la pelota del tubo y con 68,5 % la mantiene flotando.

Con un ventilador de dos cables, el PWM se aplica a la alimentación a través de un MOSFET. Si el ventilador tiene cuatro cables, puede usarse directamente su entrada de PWM.

## Etapa de potencia

- MOSFET de nivel lógico IRLZ44N: conduce completamente con 3,3 V en el gate. Un IRF540N o IRFZ44N necesita 10 V y con la LPC conduciría parcialmente.
- Se probó primero un TIP31C, que no satura con la corriente que entrega un pin de la LPC (~5 mA, harían falta ~26 mA de base para 0,26 A). El transistor quedó en corto y se descartó.
- Diodo en antiparalelo con el ventilador (1N4007; un 1N5819 es mejor a frecuencias altas). Un zener no sirve como diodo de protección del motor.
- Masa común entre la fuente de 12 V y la LPC1769, salvo que se use el optoacoplador con masas separadas.

## Optoacoplador y frecuencia del PWM

El PWM se diseñó originalmente a 20 kHz para que el ventilador no zumbe. Al agregar un optoacoplador entre la LPC y el MOSFET, el ventilador pasó a funcionar casi a pleno con cualquier duty distinto de cero: el opto tarda más en apagarse que el período de 50 µs. La frecuencia se bajó a 500 Hz (`PWM_FREQ_HZ` en `ventilador.c`), con la que el osciloscopio muestra una onda cuadrada correcta. El costo es un zumbido audible.

Alternativas para volver a 20 kHz: un optoacoplador rápido (6N137, TLP2361) o eliminar el opto.

En la medición a 500 Hz, la tensión de drain durante la conducción sube hasta ~2 V. El gate no recibe tensión suficiente; la solución es alimentar el colector del opto con 12 V y tomar la señal del emisor (circuito en [armado_del_tp.md](armado_del_tp.md#circuito-con-optoacoplador)).

## Sensibilidad de la planta

La diferencia entre que la pelota caiga (68,0 %) y que suba hasta el extremo (70,0 %) es de 2 % de duty. Se consideró perforar el tubo, para que el empuje dependa de la altura, o agregar una fuga regulable en la base; se decidió no modificar la estructura y resolverlo con el lazo de control. El PID debe limitar su salida a un rango alrededor de u0 (por ejemplo, 60 a 80 %) para que una saturación no lance la pelota fuera del tubo.

## Operacional

El LM358 no es rail-to-rail: su salida queda ~1,5 V por debajo de la alimentación.

- A 3,3 V la salida no pasa de ~1,8 V y se pierde la parte alta de la señal del sensor (pelota a menos de ~15 cm).
- A 5 V llega a ~3,5 V, por encima del máximo del pin del ADC.

La opción preferida es un operacional rail-to-rail (MCP6002, LMV358 o TLV2372) alimentado a 3,3 V. Si se usa el LM358, se alimenta a 5 V y se protege la entrada del ADC con 1 kΩ en serie y un zener 1N4728A de 3,3 V.

## Sensor Sharp GP2Y0A21YK0F

- Rango útil: 10 a 80 cm. Por debajo de ~10 cm la curva se invierte y una pelota muy cercana da la misma tensión que una lejana; si la pelota sube demasiado, el PID interpretaría que está abajo y soplaría más. Por eso el rango de control se limitó a 10–40 cm medidos desde la rejilla, en un tubo de 55 cm.
- Alimentación de 5 V con 10 µF y 100 nF junto al sensor, porque consume picos de corriente.
- Actualiza la medición cada ~40 ms, lo que fija el período del lazo en 40 ms (~25 Hz).
- La curva no es lineal: se calibra con una tabla de valores medidos a alturas conocidas.

## Drivers de la cátedra

Al usar los drivers LPC17xx de la cátedra se encontraron dos detalles de la UART:

- `UART_Init` deja el transmisor deshabilitado. Hay que llamar a `UART_TxEnable` después de inicializar; si no, no sale ningún dato.
- `UART_Init` también deja la FIFO deshabilitada, pero `UART_Send` escribe hasta 16 bytes seguidos suponiendo que está activa. Con la FIFO apagada se pierden caracteres. Se resuelve llamando a `UART_FIFOConfig` después de `UART_Init`.

En el ADC, `ADC_PinConfig` deshabilita la resistencia de pull-up del pin. Si se configura el pin escribiendo los registros a mano, hay que deshabilitarla también (registro PINMODE), porque eleva la lectura.

Con Redlib, `printf` envía el texto a la consola del depurador y no a la UART. Los mensajes se arman con `sprintf` y se envían con la función propia del módulo `comunicacionUart`. Redlib no imprime `float`, por lo que los valores se manejan como enteros (duty en por mil, tensión en mV).

## DAC

Si la perturbación senoidal del DAC se sumara solo en el software, el DAC no tendría función física. Opciones para que la perturbación sea real:

1. Sumar la salida del DAC a la señal del sensor con un operacional sumador (ruido de medición real).
2. DAC → transistor → un segundo ventilador chico que sople de costado por un orificio del tubo.
3. Señal acústica proporcional al error (DAC → buzzer piezoeléctrico).
