"""\
/*! **************************************************************************
* @file    afe11612.py
* @author  Omri Cohen
* @date    12 Apr 2026
* @brief   Python wrapper for Linux IIO driver control of the AFE11612 ADC.
******************************************************************************
* @copyright (c) 2026 Ramon.Space. All Rights Reserved.
*************************************************************************** */
"""

import os
import ctypes
import threading

THIS_DIR = os.path.dirname(os.path.abspath(__file__))

LIB_PATH = os.path.normpath(
    os.path.join(
        THIS_DIR,
        "..",  # front_end
        "..",  # myrepo
        "build",
        "user_space",
        "lib",
        "afe11612",
        "libafe11612.so"
    )
)

if not os.path.isfile(LIB_PATH):
    raise FileNotFoundError(f"AFE11612 shared library not found: {LIB_PATH}")

lib = ctypes.CDLL(LIB_PATH)

class afe11612_t(ctypes.Structure):
    """ctypes representation of the native AFE11612 device context."""

    _fields_ = [
        ("device_path", ctypes.c_char * 256),
        ("spi_path", ctypes.c_char * 256),
    ]

lib.afe11612_read_device_id.argtypes = [
    ctypes.POINTER(afe11612_t),
    ctypes.POINTER(ctypes.c_uint32),
]
lib.afe11612_read_device_id.restype = ctypes.c_int

# Function prototypes (unchanged API)
lib.afe11612_init.argtypes = [
    ctypes.POINTER(afe11612_t),
    ctypes.c_char_p,
]
lib.afe11612_init.restype = ctypes.c_int

lib.afe11612_read_voltage_input.argtypes = [
    ctypes.POINTER(afe11612_t),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_double),
]
lib.afe11612_read_voltage_input.restype = ctypes.c_int

lib.afe11612_read_temp_input.argtypes = [
    ctypes.POINTER(afe11612_t),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_double),
]
lib.afe11612_read_temp_input.restype = ctypes.c_int

lib.afe11612_write_dac_mv.argtypes = [
    ctypes.POINTER(afe11612_t),
    ctypes.c_int,
    ctypes.c_int,
]
lib.afe11612_write_dac_mv.restype = ctypes.c_int

_dev = None
_lock = threading.Lock()

def get_device():
    """Return a thread-safe singleton device instance initialized on first use."""

    global _dev
    with _lock:
        if _dev is None:
            _dev = afe11612_t()
            ret = lib.afe11612_init(
                ctypes.byref(_dev),
                b"spi0.0"   # SPI bus/chip-select identifier
            )
            if ret != 0:
                raise RuntimeError("AFE11612 init failed")
        return _dev