/*! **************************************************************************
* @file     libafe11612.h
* @author   Omri Cohen
* @date     12 Apr 2026
* @brief    Header for library wrap Linux kernel driver for afe11612 adc
******************************************************************************
* @copyright (c) 2026 Ramon.Space. All Rights Reserved.
*
* This file is part of the Ramon.Space Software Development Kit and is
* provided under a commercial license agreement.
*
* Unauthorized copying, redistribution, or use of this file, via any
* medium, is strictly prohibited without the express permission of
* Ramon.Space.
*
* For licensing information, contact: <eyall@ramon.space>
******************************************************************************
*/

#ifndef AFE11612_H
#define AFE11612_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AFE_MAX_VOLTAGES    16
#define AFE_MAX_TEMPS       3
#define AFE_MAX_DACS        12

typedef struct {
    char device_path[256];   // /sys/.../spiX.Y/iio:deviceN
    char spi_path[256];      // /sys/bus/spi/devices/spiX.Y
} afe11612_t;


int afe11612_init(afe11612_t *dev, const char *spi_dev);

int afe11612_read_device_id(afe11612_t *dev, uint32_t *id);

int afe11612_read_voltage_raw(afe11612_t *dev, int channel, int *value);
int afe11612_read_voltage_input(afe11612_t *dev, int channel, double *value);

int afe11612_read_temp_raw(afe11612_t *dev, int channel, int *value);
int afe11612_read_temp_input(afe11612_t *dev, int channel, double *value);

int afe11612_write_dac_mv(afe11612_t *dev, int channel, int mv);

#ifdef __cplusplus
}
#endif
#endif