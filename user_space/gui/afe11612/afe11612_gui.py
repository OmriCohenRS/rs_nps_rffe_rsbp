#!/usr/bin/env python3
import ctypes
import tkinter as tk
from tkinter import ttk
import threading
import time

afe = ctypes.CDLL("../../../build/user_space/lib/afe11612/libafe11612.so")

class AFE11612(ctypes.Structure):
    _fields_ = [
        ("device_path", ctypes.c_char * 256),
        ("spi_path", ctypes.c_char * 256),
    ]

afe.afe11612_init.argtypes = [
    ctypes.POINTER(AFE11612), ctypes.c_char_p
]
afe.afe11612_read_voltage_input.argtypes = [
    ctypes.POINTER(AFE11612), ctypes.c_int, ctypes.POINTER(ctypes.c_double)
]
afe.afe11612_read_temp_input.argtypes = [
    ctypes.POINTER(AFE11612), ctypes.c_int, ctypes.POINTER(ctypes.c_double)
]
afe.afe11612_read_device_id.argtypes = [
    ctypes.POINTER(AFE11612), ctypes.POINTER(ctypes.c_uint32)
]

class AFEGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("AFE11612 Analog Front-End Monitor")

        self.dev = AFE11612()
        afe.afe11612_init(ctypes.byref(self.dev), b"spi0.0")

        self.running = True
        self.build_ui()
        self.update_idletasks()
        self.minsize(self.winfo_reqwidth(), self.winfo_reqheight())
        self.start_worker()

    def build_ui(self):
        header = ttk.Frame(self, padding=10)
        header.pack(fill="x")

        self.device_var = tk.StringVar(value="Device ID: ---")
        ttk.Label(
            header, textvariable=self.device_var,
            font=("Segoe UI", 10)
        ).pack(side="left")

        ttk.Button(
            header, text="Read Device ID",
            command=self.read_device_id
        ).pack(side="left", padx=10)

        self.live_var = tk.StringVar(value="● LIVE")
        ttk.Label(
            header, textvariable=self.live_var,
            foreground="green"
        ).pack(side="right")

        body = ttk.Frame(self, padding=10)
        body.pack(fill="both", expand=True)
        body.columnconfigure(0, weight=1)
        body.columnconfigure(1, weight=1)

        # Voltages
        v_frame = ttk.LabelFrame(body, text="Voltages (V)", padding=10)
        v_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        self.volt_vars = []

        for i in range(16):
            var = tk.StringVar(value="---")
            self.volt_vars.append(var)
            row, col = i // 2, i % 2

            box = ttk.Frame(v_frame, padding=6)
            box.grid(row=row, column=col, sticky="nsew", padx=4, pady=4)
            ttk.Label(box, text=f"CH{i}").pack(anchor="w")
            ttk.Label(box, textvariable=var, font=("Segoe UI", 12, "bold")).pack(anchor="w")

        # Temperatures
        t_frame = ttk.LabelFrame(body, text="Temperatures (°C)", padding=10)
        t_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)
        self.temp_vars = []

        for i in range(3):
            var = tk.StringVar(value="---")
            self.temp_vars.append(var)
            box = ttk.Frame(t_frame, padding=10)
            box.pack(fill="x", pady=4)
            ttk.Label(box, text=f"Sensor {i}").pack(anchor="w")
            ttk.Label(box, textvariable=var, font=("Segoe UI", 12, "bold")).pack(anchor="w")

        self.status = tk.StringVar(value="Updating…")
        ttk.Label(
            self, textvariable=self.status,
            relief="sunken", anchor="w"
        ).pack(fill="x")

        ttk.Button(self, text="Quit", command=self.close).pack(pady=8)

    def read_device_id(self):
        device_id = ctypes.c_uint32()
        ret = afe.afe11612_read_device_id(ctypes.byref(self.dev), ctypes.byref(device_id))
        if ret == 0:
            self.device_var.set(f"Device ID: 0x{device_id.value:04X}")
        else:
            self.device_var.set("Device ID: ERROR")

    def _apply_update(self, live_text, voltages, temperatures, status_text):
        self.live_var.set(live_text)
        for i, text in enumerate(voltages):
            if text is not None:
                self.volt_vars[i].set(text)
        for i, text in enumerate(temperatures):
            if text is not None:
                self.temp_vars[i].set(text)
        self.status.set(status_text)

    def start_worker(self):
        threading.Thread(target=self.update_loop, daemon=True).start()
        
    def update_loop(self):
        while self.running:
            heartbeat = int(time.time()) % 2
            live_text = "● LIVE" if heartbeat else "○ LIVE"
            voltages = [None] * 16
            for i in range(16):
                val = ctypes.c_double()
                if afe.afe11612_read_voltage_input(
                    ctypes.byref(self.dev), i, ctypes.byref(val)
                ) == 0:
                    voltages[i] = f"{val.value:.6f} V"
            temperatures = [None] * 3
            for i in range(3):
                val = ctypes.c_double()
                if afe.afe11612_read_temp_input(
                    ctypes.byref(self.dev), i, ctypes.byref(val)
                ) == 0:
                    temperatures[i] = f"{val.value:.2f} °C"
            status_text = f"Last update: {time.strftime('%H:%M:%S')}"
            try:
                self.after(0, self._apply_update, live_text, voltages, temperatures, status_text)
            except tk.TclError:
                break
            time.sleep(0.5)

    def close(self):
        self.running = False
        self.destroy()

if __name__ == "__main__":
    AFEGUI().mainloop()