#!/usr/bin/env python3
import ctypes
import tkinter as tk
from tkinter import ttk

# ================= LIB LOAD =================

lib = ctypes.CDLL("../../../build/user_space/lib/apio16/libapio16.so")

lib.apio16_init.restype = ctypes.c_int
lib.apio16_close.restype = None

lib.apio16_get_level.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
lib.apio16_get_level.restype = ctypes.c_int

lib.apio16_set_level.argtypes = [ctypes.c_int, ctypes.c_int]
lib.apio16_set_level.restype = ctypes.c_int

lib.apio16_get_direction.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]
lib.apio16_get_direction.restype = ctypes.c_int

lib.apio16_set_direction.argtypes = [ctypes.c_int, ctypes.c_int]
lib.apio16_set_direction.restype = ctypes.c_int

lib.apio16_reset.restype = ctypes.c_int

# init library
if lib.apio16_init() != 0:
    print("Failed to init libapio16")
    exit(1)

# ================= HELPERS =================

def read_pin(pin):
    val = ctypes.c_int()
    if lib.apio16_get_level(pin, ctypes.byref(val)) == 0:
        return val.value
    return None

def write_pin(pin, val):
    return lib.apio16_set_level(pin, val) == 0

def get_dir(pin):
    d = ctypes.c_int()
    if lib.apio16_get_direction(pin, ctypes.byref(d)) == 0:
        return "output" if d.value == 1 else "input"
    return None

def set_dir(pin, is_output):
    return lib.apio16_set_direction(pin, 1 if is_output else 0) == 0


# ================= GUI =================

class GUI:
    def __init__(self, root):
        self.root = root
        root.title("APIO16 GUI")

        # headers
        headers = ["Pin", "Direction", "Value"]
        for col, text in enumerate(headers):
            tk.Label(root, text=text, font=("Arial", 12, "bold")).grid(row=0, column=col, padx=5, pady=5)

        self.rows = []

        # pins
        for pin in range(16):
            self.add_pin_row(pin)

        # reset button
        reset_btn = tk.Button(root,
                              text="Reset APIO16",
                              command=self.reset_apio16,
                              bg="orange")
        reset_btn.grid(row=17, column=0, columnspan=3, pady=10)


        update_btn = tk.Button(
            root,
            text="Update Status",
            command=self.update_status,
            bg="lightblue"
        )
        update_btn.grid(row=18, column=0, columnspan=3, pady=5)

        # handle window close
        root.protocol("WM_DELETE_WINDOW", self.on_close)

    # ================= ROW =================

    def update_status(self):
        for pin in range(16):
            dir_var, val_var = self.rows[pin]

            # Read direction
            d = get_dir(pin)
            if not d:
                continue

            dir_var.set(d)

            # Read value if input
            if d == "input":
                v = read_pin(pin)
                if v is not None:
                    val_var.set(str(v))
            else:
                # output → readback optional, but keep UI consistent
                val_var.set(val_var.get())

    def add_pin_row(self, pin):
        row = pin + 1

        tk.Label(self.root, text=str(pin)).grid(row=row, column=0)

        dir_var = tk.StringVar(value="input")
        val_var = tk.StringVar(value="?")

        dir_box = ttk.Combobox(self.root,
                               textvariable=dir_var,
                               values=["input", "output"],
                               width=8,
                               state="readonly")

        val_box = ttk.Combobox(self.root,
                               textvariable=val_var,
                               values=["0", "1"],
                               width=5,
                               state="readonly")

        dir_box.grid(row=row, column=1, padx=3, pady=2)
        val_box.grid(row=row, column=2, padx=3, pady=2)

        # events
        dir_box.bind("<<ComboboxSelected>>",
                     lambda e, p=pin, d=dir_var, v=val_var: self.change_dir(p, d, v))

        val_box.bind("<<ComboboxSelected>>",
                     lambda e, p=pin, d=dir_var, v=val_var: self.change_val(p, d, v))

        self.rows.append((dir_var, val_var))

    # ================= ACTIONS =================

    def change_dir(self, pin, dir_var, val_var):
        is_output = dir_var.get() == "output"

        if set_dir(pin, is_output):
            if not is_output:
                val = read_pin(pin)
                if val is not None:
                    val_var.set(str(val))
            else:
                val_var.set("0")  # default output low
        else:
            print(f"Failed to set direction for pin {pin}")

    def change_val(self, pin, dir_var, val_var):
        if dir_var.get() != "output":
            return

        val = int(val_var.get())
        if not write_pin(pin, val):
            print(f"Failed to write pin {pin}")

    def reset_apio16(self):
        if lib.apio16_reset() == 0:
            for pin in range(16):
                dir_var, val_var = self.rows[pin]

                if pin < 8:
                    dir_var.set("input")
                    val_var.set("?")
                else:
                    dir_var.set("output")
                    val_var.set("0")
        else:
            print("Reset FAILED")

    # ================= CLOSE =================

    def on_close(self):
        lib.apio16_close()
        self.root.destroy()


# ================= MAIN =================

if __name__ == "__main__":
    root = tk.Tk()
    app = GUI(root)
    root.mainloop()