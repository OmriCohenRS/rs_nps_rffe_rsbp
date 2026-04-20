# rs_nps_rffe_rsbp
Ramon space RFFE on raspberry pi sw for NPS project

## Table of content
- Repository hierarchy
- Device requierments
- build all
- Install drivers in kenrel
- Run applications

## Repository hierarchy
```text
├── README.md
├── CMakeLists.txt

├── docs/
│   ├── apio16/
│   ├── lmx2694/
│   └── afe11612/

├── kernel/
│   └── drivers/
│       ├── apio16/
│       │   ├── apio16.c
│       │   ├── Makefile
│       │   └── CMakeLists.txt
│       │
│       ├── lmx2694/
│       │   ├── lmx2694.c
│       │   ├── Makefile
│       │   └── CMakeLists.txt
│       │
│       ├── afe11612/
│       │   ├── afe11612.c
│       │   ├── Makefile
│       │   └── CMakeLists.txt
│       │
│       └── CMakeLists.txt

├── device_tree/
│   └── overlays/
│       ├── rffe-overlay.dts
│       └── CMakeLists.txt

├── user_space/
│   ├── lib/
│   │   ├── apio16/
│   │   │   ├── include/
│   │   │   ├── src/
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── lmx2694/
│   │   │   ├── include/
│   │   │   ├── src/
│   │   │   └── CMakeLists.txt
│   │   │
│   │   ├── afe11612/
│   │   │   ├── include/
│   │   │   ├── src/
│   │   │	└── CMakeLists.txt
│   │   │
│   │   └── CMakeLists.txt
│   │
│   ├── cli/
│   │   ├── apio16/
│   │   │   ├── apio16_cli.c
│   │   │	└── CMakeLists.txt
│   │   │
│   │   ├── lmx2694/
│   │   │   ├── lmx2694_cli.c
│   │   │	└── CMakeLists.txt
│   │   │
│   │   └── afe11612/
│   │   │   ├── afe11612_cli.c
│   │   │	└── CMakeLists.txt
│   │   │
│   │   └── CMakeLists.txt
│   │
│   └── gui/
│       ├── apio16/
│       │	└── apio16_gui.py
│       ├── lmx2694/
│       │	└── lmx2694_gui.py
│       └── afe11612/
│        	└── afe11612_gui.py
│
└── scripts/
```

## Device requierments
- Raspberry pi model 3/3B (4 might work as well)
- Raspberry pi MicroSD card 32GB+ with linux OS 64bit (can be built with pi image app)
- Raspberry pi SSH enabled
  ```bash
  sudo raspi-config nonint do_ssh 0
  ```
- Raspberry pi SPI enabled
  ```bash
  sudo raspi-config nonint do_spi 0
  ```
- Raspberry pi python3 and cmake installed
  ```bash
  sudo apt update && sudo apt install -y python3 python3-pip cmake
  ```

## Devices configuration
**Edit /boot/firmware/config.txt**
```text
It will probably contain 'dtparam=spi=on'. It only enable spi0 with cs0 and cs1.
Remove 'dtparam=spi=on' and Add 'dtoverlay=rffe-overlay'. This will enable and config spi according to
rffe-overlay.dtbo device tree.
Or dtoverlay='another_dt.dtbo' to load another device tree nodes.
```

**Device tree overlay**
```text
Device tree overlay (like rffe-overlay.dtbo) contain nodes that linux load to bind driver to spi bus and spi configration
like: driver, spi bus, cs, freq, mode etc..
```

**Device tree overlay example**
```text
In this node afe11612 driver bind to spi0 bus, freq is 1MHZ, SPI mode 1, and cs pin is configed to be
one of the gpios of apio16 (another device connected to rsbp):
    fragment@1 {
        target = <&spi0>;
        __overlay__ {
            status = "okay";

            #address-cells = <1>;
            #size-cells = <0>;
            cs-gpios = <&apio16 8 0>;

            afe11612@0 {
                compatible = "ti,afe11612";
                reg = <0>;                  /* logical SPI device */
                spi-max-frequency = <1000000>;
                spi-cpha;                   /* MODE 1 */
            };
        };
    };

Device tree written as my_dt.dts file and compiled to my_dt.dtbo. Then must move it to /boot/overalys/ to be load automaticly on boot
dtc -@ -I dts -O dtb -o my_dt.dtbo my_dt.dts
```

## Build all
Script build all kernel drivers, dts, userspace libs and cli.
```bash
./scripts/build_all.sh
```

## Install drivers in kenrel
Must be call only after build success.
Script move kernel drivers and devic tree (dt.dtbo and driver.ko) from build folder to kernel driver path 
```bash
./scripts/install_drivers.sh
```
After install driver, reboot rasberry pi.

## Run applications
**cli apps**
```bash
./build/user_space/cli/<device>/<cli app>
```

**gui apps**
```bash
cd user_space/gui/<device>
python3 <gui app>.py
```
