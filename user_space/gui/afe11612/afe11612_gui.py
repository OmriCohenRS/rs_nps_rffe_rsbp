#!/usr/bin/env python3

import os
import ctypes
import tkinter as tk
from tkinter import ttk
import threading
import time

# Load library
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
LIB_PATH = os.path.join(
    BASE_DIR,
    "../../../build/user_space/lib/afe11612/libafe11612.so"
)
afe = ctypes.CDLL(LIB_PATH)

# Device struct
class AFE11612(ctypes.Structure):
    _fields_ = [
        ("device_path", ctypes.c_char * 256),
        ("spi_path", ctypes.c_char * 256),
    ]

# C function prototypes
afe.afe11612_init.argtypes = [
    ctypes.POINTER(AFE11612), ctypes.c_char_p
]

afe.afe11612_read_voltage_input.argtypes = [
    ctypes.POINTER(AFE11612),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_double),
]

afe.afe11612_read_temp_input.argtypes = [
    ctypes.POINTER(AFE11612),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_double),
]

afe.afe11612_read_device_id.argtypes = [
    ctypes.POINTER(AFE11612),
    ctypes.POINTER(ctypes.c_uint32),
]

afe.afe11612_write_dac_mv.argtypes = [
    ctypes.POINTER(AFE11612),
    ctypes.c_int,
    ctypes.c_int,
]


class AFEGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("AFE11612 Analog Front-End")

        self.dev = AFE11612()
        afe.afe11612_init(ctypes.byref(self.dev), b"spi0.0")

        self.running = True
        self.build_ui()
        self.start_worker()

        self.minsize(self.winfo_reqwidth(), self.winfo_reqheight())

    # ================= UI =================

    def build_ui(self):
        # ---------- Header ----------
        header = ttk.Frame(self, padding=10)
        header.pack(fill="x")

        self.device_var = tk.StringVar(value="Device ID: ---")
        ttk.Label(header, textvariable=self.device_var).pack(side="left")

        ttk.Button(
            header, text="Read Device ID",
            command=self.read_device_id
        ).pack(side="left", padx=10)

        self.live_var = tk.StringVar(value="● LIVE")
        ttk.Label(header, textvariable=self.live_var, foreground="green").pack(side="right")

        # ---------- Main body ----------
        body = ttk.Frame(self, padding=10)
        body.pack(fill="both", expand=True)
        body.columnconfigure(0, weight=1)
        body.columnconfigure(1, weight=1)

        # ---------- DACs (LEFT) ----------
        dac_frame = ttk.LabelFrame(body, text="DAC Outputs (mV)", padding=10)
        dac_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)

        self.dac_vars = []

        for i in range(12):
            box = ttk.Frame(dac_frame, padding=5)
            box.pack(fill="x", pady=2)

            ttk.Label(box, text=f"DAC{i}", width=6).pack(side="left")

            var = tk.StringVar(value="0")
            self.dac_vars.append(var)

            entry = ttk.Entry(box, textvariable=var, width=10)
            entry.pack(side="left", padx=5)

            ttk.Label(box, text="mV").pack(side="left")

            ttk.Button(
                box,
                text="Set",
                command=lambda ch=i: self.set_dac(ch)
            ).pack(side="right")

        # ---------- ADCs (RIGHT) ----------
        adc_frame = ttk.LabelFrame(body, text="ADC Voltages (V)", padding=10)
        adc_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)

        self.volt_vars = []

        for i in range(16):
            var = tk.StringVar(value="---")
            self.volt_vars.append(var)

            row, col = i // 2, i % 2
            box = ttk.Frame(adc_frame, padding=6)
            box.grid(row=row, column=col, sticky="nsew", padx=4, pady=4)

            ttk.Label(box, text=f"CH{i}").pack(anchor="w")
            ttk.Label(box, textvariable=var, font=("Segoe UI", 11, "bold")).pack(anchor="w")

        # ---------- Status ----------
        self.status = tk.StringVar(value="Updating…")
        ttk.Label(self, textvariable=self.status, relief="sunken", anchor="w").pack(fill="x")

        ttk.Button(self, text="Quit", command=self.close).pack(pady=8)

    # ================= DAC =================

    def set_dac(self, channel):
        try:
            mv = int(self.dac_vars[channel].get())
        except ValueError:
            self.status.set(f"Invalid DAC{channel} value")
            return

        ret = afe.afe11612_write_dac_mv(
            ctypes.byref(self.dev),
            channel,
            mv
        )

        if ret == 0:
            self.status.set(f"DAC{channel} set to {mv} mV")
        else:
            self.status.set(f"DAC{channel} ERROR ({ret})")

    # ================= ADC / TEMP =================

    def update_loop(self):
        while self.running:
            heartbeat = int(time.time()) % 2
            self.live_var.set("● LIVE" if heartbeat else "○ LIVE")

            for i in range(16):
                val = ctypes.c_double()
                if afe.afe11612_read_voltage_input(
                    ctypes.byref(self.dev), i, ctypes.byref(val)
                ) == 0:
                    self.volt_vars[i].set(f"{val.value:.6f} V")

            self.status.set(f"Last update: {time.strftime('%H:%M:%S')}")
            time.sleep(0.5)

    def start_worker(self):
        threading.Thread(target=self.update_loop, daemon=True).start()

    # ================= Misc =================

    def read_device_id(self):
        device_id = ctypes.c_uint32()
        if afe.afe11612_read_device_id(ctypes.byref(self.dev), ctypes.byref(device_id)) == 0:
            self.device_var.set(f"Device ID: 0x{device_id.value:04X}")
        else:
            self.device_var.set("Device ID: ERROR")

    def close(self):
        self.running = False
        self.destroy()


if __name__ == "__main__":
    AFEGUI().mainloop()