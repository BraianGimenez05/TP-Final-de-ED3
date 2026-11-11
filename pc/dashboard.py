
"""
Dashboard del levitador neumático (TP final de Electrónica Digital III, grupo C2-G11).

Uso:  python3 pc/dashboard.py
Sin la placa, elegir el puerto SIMULADOR.
"""

import csv
import queue
import threading
import tkinter as tk
import re
from collections import deque
from datetime import datetime
from pathlib import Path
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from tkinter import messagebox, ttk

import serial
import serial.tools.list_ports

try:                                    # el simulador es opcional (no se sube al repo)
    from simulador import LPCSimulada
except ImportError:
    LPCSimulada = None

BAUDIOS = 9600
SIMULADOR = "SIMULADOR"
VENTANA_S = 30      # segundos visibles en el gráfico
RE_TELEM = re.compile(r"t=(\d+) sp=(\d+) y=(\d+) u=(\d+)")
PAUSA_ENTRE_CMD_MS = 150
RE_PARAM = re.compile(r"SP=(\d+) u0=(\d+) Kp=(-?\d+) Ki=(-?\d+) Kd=(-?\d+).*?"
                      r"(?:W=(\d+) F=(\d+) )?(?:R=(\d+) )?modo=(\w+)")
# Los ensayos se guardan en docs/ensayos/ del repo, sin importar desde dónde se ejecute
CARPETA_ENSAYOS = Path(__file__).resolve().parent.parent / "docs" / "ensayos"


class ConexionSerie:
    """Puerto serie real. Lee líneas en un hilo y las deja en una cola."""

    def __init__(self, puerto, cola_rx):
        self.puerto = puerto
        self.cola_rx = cola_rx
        self.ser = None
        self.activo = False

    def abrir(self):
        self.ser = serial.Serial(self.puerto, BAUDIOS, timeout=0.1)
        self.activo = True
        threading.Thread(target=self._leer, daemon=True).start()

    def _leer(self):
        # Corre en el hilo aparte: lee línea por línea mientras esté conectado
        while self.activo:
            try:
                linea = self.ser.readline()
            except serial.SerialException:
                self.cola_rx.put("!! Se perdió la conexión con el puerto")
                self.activo = False
                break
            if linea:
                self.cola_rx.put(linea.decode("ascii", errors="replace").strip())

    def enviar(self, texto):
        if self.ser and self.activo:
            self.ser.write((texto + "\r").encode("ascii"))   # \r = Enter

    def cerrar(self):
        self.activo = False
        if self.ser:
            self.ser.close()


class Dashboard(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Levitador neumático — Grupo C2-G11")
        self.geometry("1200x700")

        self.cola_rx = queue.Queue()
        self.conexion = None
        self.pendientes = deque()     # comandos que esperan para salir, de a uno
        self.archivo_csv = None       # archivo abierto mientras se graba
        self.escritor_csv = None

        # Últimos datos recibidos (tiempo, setpoint, medición, duty)
        self.t, self.sp, self.y, self.u = deque(), deque(), deque(), deque()

        self._armar_barra_superior()
        cuerpo = ttk.Frame(self)
        cuerpo.pack(fill=tk.BOTH, expand=True)
        self._armar_grafico(cuerpo)
        columna = ttk.Frame(cuerpo, padding=6)          # columna derecha
        columna.pack(side=tk.RIGHT, fill=tk.Y)
        self._armar_panel(columna)
        self._armar_consola(columna)
        self._refrescar_puertos()
        self.protocol("WM_DELETE_WINDOW", self._salir)
        self.after(100, self._procesar_rx)
        self.after(250, self._actualizar_grafico)
        self.after(PAUSA_ENTRE_CMD_MS, self._enviar_pendientes)

    # ---------------------------------------------------- interfaz
    def _armar_barra_superior(self):
        barra = ttk.Frame(self, padding=6)
        barra.pack(side=tk.TOP, fill=tk.X)

        ttk.Label(barra, text="Puerto:").pack(side=tk.LEFT)
        self.combo_puerto = ttk.Combobox(barra, width=18, state="readonly")
        self.combo_puerto.pack(side=tk.LEFT, padx=4)
        ttk.Button(barra, text="Actualizar", command=self._refrescar_puertos).pack(side=tk.LEFT)
        self.boton_conectar = ttk.Button(barra, text="Conectar", command=self._conectar)
        self.boton_conectar.pack(side=tk.LEFT, padx=8)
        self.lbl_estado = ttk.Label(barra, text="Desconectado", foreground="gray")
        self.lbl_estado.pack(side=tk.LEFT)

        self.boton_csv = ttk.Button(barra, text="Grabar CSV", command=self._alternar_csv)
        self.boton_csv.pack(side=tk.RIGHT)
        self.lbl_csv = ttk.Label(barra, text="", foreground="red")
        self.lbl_csv.pack(side=tk.RIGHT, padx=8)

    def _armar_grafico(self, padre):
        self.fig = Figure(figsize=(8, 6), dpi=100)
        self.ax_y = self.fig.add_subplot(211)                    # arriba: altura
        self.ax_u = self.fig.add_subplot(212, sharex=self.ax_y)  # abajo: duty

        self.linea_y, = self.ax_y.plot([], [], label="Medición (y)", color="tab:blue")
        self.linea_sp, = self.ax_y.plot([], [], label="Setpoint", color="tab:orange", linestyle="--")
        self.linea_u, = self.ax_u.plot([], [], label="Duty (u)", color="tab:green")

        self.ax_y.set_ylabel("Altura [mm]")
        # Escalas fijas: altura en todo el rango del sensor; duty en el rango que usa el PID
        self.ax_y.set_ylim(0, 350)
        self.ax_y.set_yticks(range(0, 351, 50))
        self.ax_u.set_ylim(0, 750)
        self.ax_u.set_yticks(range(0, 751, 150))
        self.ax_y.set_xlim(-VENTANA_S, 0)
        self.ax_u.set_ylabel("Duty [‰]")
        self.ax_u.set_xlabel("Tiempo [s]")
        self.ax_y.legend(loc="upper left")
        self.ax_y.grid(True, alpha=0.3)
        self.ax_u.grid(True, alpha=0.3)
        self.fig.tight_layout()

        self.canvas = FigureCanvasTkAgg(self.fig, master=padre)
        self.canvas.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    def _armar_panel(self, padre):
        # Valores actuales
        actual = ttk.LabelFrame(padre, text="Valores actuales", padding=6)
        actual.pack(fill=tk.X)
        self.lbl_valores = ttk.Label(actual, text="y = —   sp = —\nerror = —   u = —",
                                     font=("monospace", 11))
        self.lbl_valores.pack(anchor=tk.W)
        self.lbl_modo = ttk.Label(actual, text="Modo: —")
        self.lbl_modo.pack(anchor=tk.W)

        # Control
        control = ttk.LabelFrame(padre, text="Control", padding=6)
        control.pack(fill=tk.X, pady=6)
        ttk.Button(control, text="Arrancar PID", command=lambda: self._encolar("A")).pack(fill=tk.X)
        ttk.Button(control, text="Apagar ventilador", command=lambda: self._encolar("D0")).pack(fill=tk.X, pady=2)
        fila = ttk.Frame(control)
        fila.pack(fill=tk.X, pady=2)
        ttk.Label(fila, text="Duty manual [‰]").pack(side=tk.LEFT)
        self.ent_duty = ttk.Entry(fila, width=7)
        self.ent_duty.insert(0, "560")
        self.ent_duty.pack(side=tk.LEFT, padx=4)
        ttk.Button(fila, text="Aplicar", command=self._aplicar_duty).pack(side=tk.LEFT)

        # Parámetros del PID: letra del comando, texto y valor inicial
        params = ttk.LabelFrame(padre, text="Parámetros del PID", padding=6)
        params.pack(fill=tk.X)
        self.entradas = {}
        campos = [("S", "Setpoint [mm]", "175"),
                  ("P", "Kp × 1000", "300"),
                  ("I", "Ki × 1000", "30"),
                  ("K", "Kd × 1000", "360"),
                  ("U", "u0 [‰]", "560"),
                  ("R", "Rampa [mm/s]", "20"),
                  ("W", "Perturbación [‰]", "0"),
                  ("F", "Frec. pert. [0,1 Hz]", "5")]
        for fila_n, (letra, texto, defecto) in enumerate(campos):
            ttk.Label(params, text=texto).grid(row=fila_n, column=0, sticky=tk.W, pady=1)
            ent = ttk.Entry(params, width=8)
            ent.insert(0, defecto)
            ent.grid(row=fila_n, column=1, padx=4)
            self.entradas[letra] = ent
        ttk.Button(params, text="Aplicar parámetros", command=self._aplicar_parametros).grid(
            row=len(campos), column=0, columnspan=2, sticky=tk.EW, pady=(6, 0))
        ttk.Button(params, text="Leer de la LPC (?)", command=lambda: self._encolar("?")).grid(
            row=len(campos) + 1, column=0, columnspan=2, sticky=tk.EW, pady=2)

    def _armar_consola(self, padre):
        marco = ttk.LabelFrame(padre, text="Consola", padding=6)
        marco.pack(fill=tk.BOTH, expand=True, pady=(6, 0))

        self.txt_consola = tk.Text(marco, width=42, height=10, font=("monospace", 9), state=tk.DISABLED)
        self.txt_consola.pack(fill=tk.BOTH, expand=True)

        self.var_ver_telem = tk.BooleanVar(value=False)
        ttk.Checkbutton(marco, text="Mostrar telemetría", variable=self.var_ver_telem).pack(anchor=tk.W)

        fila = ttk.Frame(marco)
        fila.pack(fill=tk.X, pady=(6, 0))
        self.ent_cmd = ttk.Entry(fila)
        self.ent_cmd.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.ent_cmd.bind("<Return>", lambda _e: self._enviar_manual())   # Enter envía
        ttk.Button(fila, text="Enviar", command=self._enviar_manual).pack(side=tk.LEFT, padx=4)

    # ---------------------------------------------------- conexión
    def _refrescar_puertos(self):
        # Solo dispositivos USB (CP2102, etc.): los /dev/ttyS* internos de la PC no sirven
        puertos = [p.device for p in serial.tools.list_ports.comports() if p.vid is not None]
        opciones = puertos + ([SIMULADOR] if LPCSimulada else [])
        self.combo_puerto["values"] = opciones
        # Si está el CP2102 (ttyUSB), lo elige solo
        preferido = next((p for p in puertos if "ttyUSB" in p), opciones[0] if opciones else "")
        self.combo_puerto.set(preferido)

    def _conectar(self):
        if self.conexion:                       # el mismo botón sirve para desconectar
            self._desconectar()
            return

        puerto = self.combo_puerto.get()
        if not puerto:
            messagebox.showwarning("Sin puertos", "No hay ningún puerto serie. Conectá el CP2102 y apretá Actualizar.")
            return
        if puerto == SIMULADOR:
            self.conexion = LPCSimulada(self.cola_rx)
        else:
            self.conexion = ConexionSerie(puerto, self.cola_rx)

        try:
            self.conexion.abrir()
        except (serial.SerialException, OSError) as err:
            self.conexion = None
            messagebox.showerror("No se pudo abrir el puerto",
                                 f"{puerto}\n\n{err}\n\n¿Está conectado el CP2102? "
                                 "¿Hay un screen abierto usando el mismo puerto?")
            return

        self.lbl_estado.config(text=f"Conectado a {puerto}", foreground="green")
        self.boton_conectar.config(text="Desconectar")
        self._log(f"-- Conectado a {puerto} a {BAUDIOS} baudios")
        self._encolar("?")        # carga en los campos los valores que tiene la LPC

    def _desconectar(self):
        if self.archivo_csv:            # si estaba grabando, cierra el archivo
            self._alternar_csv()
        self.conexion.cerrar()
        self.conexion = None
        self.pendientes.clear()
        self.lbl_estado.config(text="Desconectado", foreground="gray")
        self.boton_conectar.config(text="Conectar")
        self._log("-- Desconectado")

    # ---------------------------------------------------- envío y recepción
    def _encolar(self, cmd):
        if not self.conexion:
            messagebox.showwarning("Sin conexión", "Primero conectate a un puerto.")
            return
        self.pendientes.append(cmd)

    def _enviar_pendientes(self):
        # Manda un comando de la fila cada PAUSA_ENTRE_CMD_MS
        if self.conexion and self.pendientes:
            cmd = self.pendientes.popleft()
            self.conexion.enviar(cmd)
            self._log(f"> {cmd}")
        self.after(PAUSA_ENTRE_CMD_MS, self._enviar_pendientes)

    def _enviar_manual(self):
        cmd = self.ent_cmd.get().strip()
        if cmd:
            self._encolar(cmd)
            self.ent_cmd.delete(0, tk.END)

    def _entero(self, entrada, nombre):
        try:
            return int(entrada.get())
        except ValueError:
            messagebox.showerror("Valor inválido", f"{nombre} tiene que ser un número entero.")
            return None

    def _aplicar_duty(self):
        valor = self._entero(self.ent_duty, "El duty")
        if valor is not None:
            self._encolar(f"D{valor}")

    def _aplicar_parametros(self):
        for letra, ent in self.entradas.items():
            valor = self._entero(ent, letra)
            if valor is None:
                return
            self._encolar(f"{letra}{valor}")       # S175, P300, I20, K360, U505

    def _procesar_rx(self):
        # Vacía la cola: procesa todas las líneas que llegaron desde la última vez
        while not self.cola_rx.empty():
            linea = self.cola_rx.get()
            m = RE_TELEM.fullmatch(linea)
            if m:                                           # es telemetría
                t, sp, y, u = (int(x) for x in m.groups())
                self._agregar_muestra(t, sp, y, u)
                if self.var_ver_telem.get():
                    self._log(linea)
            else:                                           # es otra respuesta (OK, parámetros...)
                p = RE_PARAM.search(linea)
                if p:
                    self._cargar_parametros(p.groups())
                elif linea.startswith("PID en marcha"):
                    self.lbl_modo.config(text="Modo: AUTO")
                elif linea.startswith("MANUAL"):
                    self.lbl_modo.config(text="Modo: MANUAL")
                self._log(linea)
        self.after(100, self._procesar_rx)      # se vuelve a llamar dentro de 100 ms

    def _cargar_parametros(self, grupos):
        sp, u0, kp, ki, kd, amp, frec, rampa, modo = grupos
        for letra, valor in zip("SUPIKWFR", (sp, u0, kp, ki, kd, amp, frec, rampa)):
            if valor is None:                   # firmware viejo, sin rampa
                continue
            self.entradas[letra].delete(0, tk.END)
            self.entradas[letra].insert(0, valor)
        self.lbl_modo.config(text=f"Modo: {modo}")

    def _agregar_muestra(self, t, sp, y, u):
        if self.t and t < self.t[-1]:       # el tiempo volvió atrás: la LPC se reinició
            for lista in (self.t, self.sp, self.y, self.u):
                lista.clear()
        self.t.append(t); self.sp.append(sp); self.y.append(y); self.u.append(u)
        # Descarta lo que tenga más de VENTANA_S segundos
        while self.t[-1] - self.t[0] > VENTANA_S * 1000:
            for lista in (self.t, self.sp, self.y, self.u):
                lista.popleft()
        self.lbl_valores.config(text=f"y = {y:<5d} sp = {sp}\nerror = {sp - y:<5d} u = {u}")
        if self.escritor_csv:
            self.escritor_csv.writerow([t, sp, y, u])

    def _actualizar_grafico(self):
        if self.t:
            ts = [(x - self.t[-1]) / 1000.0 for x in self.t]   # el dato más nuevo queda en t = 0
            self.linea_y.set_data(ts, self.y)
            self.linea_sp.set_data(ts, self.sp)
            self.linea_u.set_data(ts, self.u)
            self.ax_y.set_xlim(-VENTANA_S, 0)
            self.canvas.draw_idle()
        self.after(250, self._actualizar_grafico)

    def _log(self, texto):
        self.txt_consola.config(state=tk.NORMAL)
        self.txt_consola.insert(tk.END, texto + "\n")
        self.txt_consola.see(tk.END)            # baja hasta la última línea
        self.txt_consola.config(state=tk.DISABLED)

    # ---------------------------------------------------- grabación CSV
    def _alternar_csv(self):
        if self.archivo_csv:                                    # estaba grabando: detener
            self.archivo_csv.close()
            self._log(f"-- Grabación guardada en {self.archivo_csv.name}")
            self.archivo_csv = self.escritor_csv = None
            self.boton_csv.config(text="Grabar CSV")
            self.lbl_csv.config(text="")
            return

        if not self.conexion:
            messagebox.showwarning("Sin conexión", "Primero conectate a un puerto.")
            return
        CARPETA_ENSAYOS.mkdir(parents=True, exist_ok=True)
        ruta = CARPETA_ENSAYOS / datetime.now().strftime("ensayo_%Y-%m-%d_%H-%M-%S.csv")
        self.archivo_csv = open(ruta, "w", newline="")
        self.escritor_csv = csv.writer(self.archivo_csv)
        self.escritor_csv.writerow(["t_ms", "setpoint", "y", "u"])   # encabezado
        self.boton_csv.config(text="Detener grabación")
        self.lbl_csv.config(text=f"Grabando {ruta.name}")
        self._log(f"-- Grabando en {ruta}")

    def _salir(self):
        if self.conexion:
            self._desconectar()
        self.destroy()


if __name__ == "__main__":
    Dashboard().mainloop()