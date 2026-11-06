# Modelo de la planta y diseño del PID

Cálculo del controlador para el levitador neumático: modelo físico, linealización, diseño del PID y verificación por simulación.

Los parámetros marcados como supuestos (τ y el coeficiente de arrastre dentro del tubo) son estimaciones y deben medirse con el equipo armado (ver [sección 7](#7-identificación-de-parámetros-con-el-equipo-armado)). El PWM de equilibrio ya se midió: u0 = 68,5 % (el diseño original suponía 50 %).

---

## Resultado

| Ganancia | Valor | Unidades |
|---|---|---|
| Kp | **0,10** | % PWM / cm |
| Ki | **0,02** | % PWM / (cm·s) |
| Kd | **0,12** | % PWM·s / cm |
| Tf (filtro de la derivada) | **0,05** | s |
| b (peso de referencia) | **0,7** | — |
| Ts (período del lazo) | **40** | ms |

Con el error en cm y la salida en % de PWM, los números coinciden con las ganancias calculadas en SI (error en m, salida de 0 a 1).

Desempeño con el modelo nominal original (u0 = 50 %): margen de fase 51°, margen de ganancia 10,6 dB, sobrepico 3,6 % y establecimiento (2 %) de ~4,4 s para un escalón de 30 a 40 cm. Con el u0 medido la ganancia de la planta baja a 0,73 veces la nominal; el efecto se analiza en la [sección 4](#4-valores-numéricos).

---

## 1. Modelo físico

Sobre la pelota actúan el empuje del aire hacia arriba y el peso hacia abajo. El empuje depende de la velocidad relativa entre el aire y la pelota:

$$
m\,\ddot{y} = k\,(v_a - \dot{y})^2 - m\,g \qquad\text{con}\qquad k = \tfrac{1}{2}\,\rho\,C_d\,A
$$

El ventilador no cambia de velocidad instantáneamente; se modela como un sistema de primer orden:

$$
\tau\,\dot{v}_a = -v_a + K_f\,u
$$

| Símbolo | Significado |
|---|---|
| $y$ | altura de la pelota [m] |
| $v_a$ | velocidad del aire en el tubo [m/s] |
| $u$ | ciclo de trabajo del PWM, de 0 a 1 |
| $K_f$ | ganancia del ventilador [(m/s) por unidad de PWM] |
| $\tau$ | constante de tiempo del ventilador [s] |

## 2. Punto de equilibrio

Con la pelota quieta ($\dot{y} = 0$, $\ddot{y} = 0$), el empuje iguala al peso:

$$
k\,v_{a0}^2 = m\,g \quad\Rightarrow\quad v_{a0} = \sqrt{\frac{m\,g}{k}}, \qquad u_0 = \frac{v_{a0}}{K_f}
$$

## 3. Linealización

Derivando el empuje alrededor del equilibrio ($v_a = v_{a0} + \delta v_a$, $\dot y = \delta \dot y$):

$$
m\,\delta\ddot{y} = 2\,k\,v_{a0}\,(\delta v_a - \delta\dot{y})
\quad\Rightarrow\quad
\delta\ddot{y} = \alpha\,(\delta v_a - \delta\dot{y}), \qquad \alpha = \frac{2\,g}{v_{a0}}
$$

Pasando a Laplace y combinando con el ventilador, la función de transferencia del PWM a la altura es:

$$
G(s) = \frac{Y(s)}{U(s)} = \frac{K_f\,\alpha}{s\,(s + \alpha)\,(\tau s + 1)}\;e^{-\theta s}
$$

| Término | Significado |
|---|---|
| $1/s$ | integrador: la pelota acumula velocidad. Es inestable a lazo abierto. |
| $1/(s+\alpha)$ | la pelota tarda en alcanzar la velocidad del aire |
| $1/(\tau s+1)$ | el ventilador tarda en acelerar |
| $e^{-\theta s}$ | retardo: una muestra del sensor + medio período del retenedor. $\theta = T_s + T_s/2 = 60$ ms |

La ganancia no depende de la altura: en un tubo, la velocidad del aire es aproximadamente la misma a cualquier altura, así que un solo PID sirve para todo el rango.

## 4. Valores numéricos

| Parámetro | Valor | Origen |
|---|---|---|
| $m$ | 2,7 g | pelota de ping-pong |
| $d$ | 40 mm | → $A = \pi d^2/4 = 1{,}257\cdot10^{-3}$ m² |
| $\rho$ | 1,2 kg/m³ | aire |
| $C_d$ | 0,47 | esfera libre (supuesto; dentro del tubo es mayor) |
| $k$ | $3{,}54\cdot10^{-4}$ kg/m | $\tfrac12 \rho C_d A$ |
| $v_{a0}$ | 8,65 m/s | $\sqrt{mg/k}$ |
| $u_0$ | 0,685 | medido: PWM con el que la pelota flota (antes se suponía 0,50) |
| $K_f$ | 12,6 (m/s)/u | $v_{a0}/u_0$ (con u0 = 0,50 daba 17,3) |
| $\alpha$ | 2,27 s⁻¹ | $2g/v_{a0}$ |
| $\tau$ | 0,40 s | supuesto, valor típico para un ventilador de este tamaño |
| $T_s$ | 40 ms | período de actualización del Sharp |

Con el u0 medido:

$$
G(s) = \frac{28{,}7}{s\,(s + 2{,}27)\,(0{,}4\,s + 1)}\;e^{-0{,}06\,s}
$$

El diseño de las secciones 5 y 6 se hizo con la estimación original, $K_f\,\alpha = 39{,}2$. El valor actual equivale a $K_f \times 0{,}73$, que queda entre el caso nominal y el caso $K_f \times 0{,}5$ de la tabla de robustez: el lazo sigue siendo estable, con más margen de fase (entre 51° y 83°) y una respuesta algo más lenta. Las ganancias se recalculan cuando se midan $K_f$, $\alpha$ y $\tau$ con el ensayo al escalón.

## 5. Diseño del controlador

### Por qué un PID con peso de referencia

- El término I es necesario. El PWM de equilibrio $u_0$ no se conoce con exactitud; sin integral, cualquier error en $u_0$ deja la pelota a otra altura.
- Hay dos integradores en el lazo (el de la planta y el del PID). Eso hace que el escalón de referencia tenga mucho sobrepico: con 51° de margen de fase, un PID común daba 27 %.
- La solución es un PID de 2 grados de libertad: la parte proporcional ve solo una fracción $b$ de la referencia. El rechazo de perturbaciones no cambia, pero el sobrepico baja.

### Ley de control

$$
u = u_0 + K_p\,(b\,r - y) + K_i \int (r - y)\,dt - K_d\,\frac{dy_f}{dt}
$$

- Derivada sobre la medición ($y$), no sobre el error: al cambiar la referencia no hay un salto brusco en el PWM.
- Derivada filtrada con constante $T_f$, para no amplificar el ruido del Sharp.
- Anti-windup condicional: el integrador no acumula mientras el PWM está saturado (0% o 100%).
- Arranque sin salto: el integrador se inicializa en $K_p\,(1-b)\,r_0$. Si no, la pelota cae ~9 cm al activar el control.

### Versión discreta (para el firmware, cada $T_s$)

```text
e      = r - y
D      = (Tf·D - Kd·(y - y_ant)) / (Tf + Ts)      // derivada filtrada sobre y
u      = u0 + Kp·(b·r - y) + I + D
u_sat  = limitar(u, 0, 100)
si u == u_sat:  I = I + Ki·Ts·e                   // anti-windup
y_ant  = y
```

### Método de búsqueda

Se recorrieron combinaciones de $K_p$, $K_i$, $K_d$ y $b$ con estas condiciones:

- margen de fase ≥ 45°
- margen de ganancia ≥ 6 dB
- sobrepico ≤ 10% en un escalón de 30 a 40 cm (simulación no lineal)

Entre las que cumplían, se eligió la que menos se desvía ante una pérdida permanente de caudal del 2%.

## 6. Verificación por simulación

La simulación usa el modelo no lineal completo, con muestreo de 40 ms, retardo de una muestra del sensor, saturación del PWM y anti-windup.

### Respuesta al escalón 30 → 40 cm (modelo original, u0 = 50 %)

| t [s] | y [cm] | PWM [%] |
|---|---|---|
| 0,0 | 30,0 | 50,70 |
| 0,5 | 30,7 | 50,41 |
| 1,0 | 33,1 | 49,97 |
| 1,4 | 35,0 | 50,05 |
| 1,9 | 36,0 | 50,15 |
| 2,4 | 36,9 | 50,09 |
| 2,9 | 37,9 | 50,04 |
| 3,4 | 38,6 | 50,04 |
| 3,8 | 39,1 | 50,04 |
| 4,3 | 39,5 | 50,03 |
| 4,8 | 39,9 | 50,02 |

### Resumen

| Prueba | Resultado |
|---|---|
| Escalón 30 → 40 cm | sobrepico 3,6% · establecimiento ~4,4 s |
| Caudal −2% permanente | desvío máximo 7,7 cm, vuelve a la referencia |
| Frecuencia de cruce | 2,6 rad/s |

### Robustez (mismo PID, planta distinta)

| Variación | Margen de fase | Margen de ganancia |
|---|---|---|
| Nominal | 50,9° | 10,6 dB |
| α a la mitad | 50,6° | 13,3 dB |
| α al doble | 53,2° | 9,0 dB |
| Kf × 0,5 | 83,0° | 16,6 dB |
| Kf × 1,5 | 32,3° | 7,1 dB |
| τ × 0,5 | 55,0° | 9,4 dB |
| τ × 1,5 | 49,9° | 11,8 dB |
| Peor caso (α/2, Kf×1,5, τ×1,5) | 32,2° | 10,1 dB |

El lazo se mantiene estable en todos los casos. Si $K_f$ resulta mayor que lo estimado, va a oscilar más: en ese caso hay que bajar $K_p$ y $K_d$ en la misma proporción.

### Observaciones

- El límite de velocidad lo pone el ventilador ($\tau$). Con un cruce cerca de 2,6 rad/s, el lazo no puede ser mucho más rápido sin perder estabilidad.
- La planta es muy sensible: el PWM solo se mueve ±1 % alrededor del equilibrio. Se confirmó en el equipo: con 68,0 % la pelota cae y con 70,0 % sube hasta el extremo. El firmware maneja el duty en pasos de 0,1 % (por mil), y con el PWM a 500 Hz el contador tiene 50 000 cuentas por período, de modo que la resolución alcanza.
- Ruido de la derivada: la ganancia en alta frecuencia es $K_d/T_f = 2{,}4$ %/cm. Un ruido de 1 mm en la medición produce ~0,24% de variación en el PWM; por eso conviene promediar las muestras del ADC con DMA.

## 7. Identificación de parámetros con el equipo armado

> La guía completa, con el orden de trabajo y tablas para anotar los resultados, está en [armado_del_tp.md](armado_del_tp.md).

1. **$u_0$ (PWM de equilibrio).** En modo manual, subir el PWM de a poco hasta que la pelota flote quieta. Medido: 68,5 %.
2. **$K_f$ y $\alpha$.** Desde $u_0$, aplicar un escalón chico de PWM ($\Delta u$ = +2%) y registrar la altura por UART. La pelota termina subiendo a velocidad constante $v_\infty$:
   - $K_f = v_\infty / \Delta u$
   - el tiempo que tarda la velocidad en llegar al 63% de $v_\infty$ es aproximadamente $1/\alpha$
3. **$\tau$.** Si el ventilador tiene cable de tacómetro, medir cuánto tarda en llegar al 63% del cambio de RPM ante un escalón de PWM. Si no, estimarlo con el retraso entre el escalón de PWM y el momento en que la pelota empieza a acelerar.
4. Recalcular $G(s)$ con los valores medidos y repetir el diseño de la sección 5.

### Ajuste fino en el equipo

Si hace falta retocar a mano:

1. Empezar con $K_i = 0$ y $b = 1$.
2. Subir $K_d$ hasta que la pelota deje de oscilar.
3. Subir $K_p$ hasta que responda rápido sin oscilar.
4. Agregar $K_i$ de a poco hasta eliminar el error permanente.
5. Bajar $b$ (0,5–0,8) si el escalón tiene mucho sobrepico.
