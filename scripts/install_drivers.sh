#!/bin/sh

set -e

REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT"

cp build/kernel/drivers/apio16/apio16.ko /lib/modules/$(uname -r)/kernel/drivers/spi/
cp build/kernel/drivers/afe11612/afe11612.ko /lib/modules/$(uname -r)/kernel/drivers/spi/
sudo depmod -a

cp build/device_tree/overlays/rffe-overlay.dtbo /boot/overlays/

#reboot rsbp now