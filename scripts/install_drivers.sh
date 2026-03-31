#!/bin/sh

set -e

cp build/kernel/drivers/apio16/apio16.ko /lib/modules/$(uname -r)/kernel/drivers/spi/
sudo depmod -a
cp build/device_tree/overlays/apio16-overlay.dtbo /boot/overlays/

#reboot rsbp now