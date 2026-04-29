from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import ctypes
import threading

from afe11612 import lib, get_device

ADC_CHANNELS  = 16
TEMP_CHANNELS = 3
DAC_CHANNELS  = 12

app = FastAPI()
hw_lock = threading.Lock()

# -------- API --------

@app.get("/api/adc")
def read_adc():
    dev = get_device()
    values = []
    with hw_lock:
        for ch in range(ADC_CHANNELS):
            v = ctypes.c_double()
            lib.afe11612_read_voltage_input(
                ctypes.byref(dev), ch, ctypes.byref(v)
            )
            values.append(v.value)
    return values


@app.get("/api/temp")
def read_temp():
    dev = get_device()
    values = []
    with hw_lock:
        for ch in range(TEMP_CHANNELS):
            t = ctypes.c_double()
            lib.afe11612_read_temp_input(
                ctypes.byref(dev), ch, ctypes.byref(t)
            )
            values.append(t.value)
    return values



@app.post("/api/dac/{channel}/{mv}")
def write_dac(channel: int, mv: int):
    if channel < 0 or channel >= DAC_CHANNELS:
        raise HTTPException(status_code=400, detail="Invalid DAC channel")

    # Safety clamp
    mv = max(0, min(mv, 5000))

    dev = get_device()
    with hw_lock:
        ret = lib.afe11612_write_dac_mv(
            ctypes.byref(dev),
            channel,
            mv
        )

    if ret != 0:
        raise HTTPException(status_code=500, detail="DAC write failed")

    return {
        "channel": channel,
        "mv": mv
    }

@app.get("/api/device_id")
def device_id():
    dev = get_device()
    value = ctypes.c_uint32()

    with hw_lock:
        ret = lib.afe11612_read_device_id(
            ctypes.byref(dev), ctypes.byref(value)
        )

    if ret != 0:
        raise HTTPException(500, "Failed to read device ID")

    return {"device_id": hex(value.value)}

# -------- GUI --------

@app.get("/")
def root():
    return FileResponse("static/index.html")

app.mount("/static", StaticFiles(directory="static"), name="static")