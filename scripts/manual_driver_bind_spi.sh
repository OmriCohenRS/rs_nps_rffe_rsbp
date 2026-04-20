#!/bin/sh

make clean
make 

echo "" | sudo tee /sys/bus/spi/devices/spi0.0/driver_override
sudo rmmod afe11612
sudo insmod afe11612.ko
echo afe11612 | sudo tee /sys/bus/spi/devices/spi0.0/driver_override
echo spi0.0 | sudo tee /sys/bus/spi/drivers/spidev/unbind
echo spi0.0 | sudo tee /sys/bus/spi/drivers/afe11612/bind