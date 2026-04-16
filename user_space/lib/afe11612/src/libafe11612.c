/*! **************************************************************************
* @file    libafe11612.c
* @author  Omri Cohen
* @date    12 Apr 2026
* @brief   Library wrapper for Linux iio driver controlling the afe11612 adc.
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

#include "libafe11612.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>

typedef enum {
    AFE_VAL_INT,
    AFE_VAL_UINT32_HEX,
    AFE_VAL_DOUBLE,
} afe_val_type_t;

/*! ****************************************************************************
 * @fn read_file_value
 * @brief Read and parse a value from a sysfs file.
 * @param path Path to the file to read.
 * @param type Expected value type.
 * @param out Pointer to store the parsed value.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
static int read_file_value(const char *path, afe_val_type_t type, void *out)
{
    FILE *f;
    int ret;

    f = fopen(path, "r");
    if (!f)
        return -errno;

    switch (type) {
    case AFE_VAL_INT:
        ret = fscanf(f, "%d", (int *)out);
        break;

    case AFE_VAL_UINT32_HEX:
        ret = fscanf(f, "%x", (uint32_t *)out);
        break;

    case AFE_VAL_DOUBLE:
        ret = fscanf(f, "%lf", (double *)out);
        break;

    default:
        fclose(f);
        return -EINVAL;
    }

    fclose(f);

    if (ret != 1)
        return -EIO;

    return 0;
}

/*! ****************************************************************************
 * @fn afe11612_init
 * @brief Initialize an AFE11612 device instance by locating its IIO device.
 * @param dev Pointer to the AFE11612 device structure.
 * @param spi_dev SPI device identifier string.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
int afe11612_init(afe11612_t *dev, const char *spi_dev)
{
    DIR *dir;
    struct dirent *ent;
    int found = 0;

    if (!dev || !spi_dev)
        return -EINVAL;

    /* STEP 2.1: store SPI base path */
    snprintf(dev->spi_path, sizeof(dev->spi_path),
             "/sys/bus/spi/devices/%s", spi_dev);

    /* STEP 2.2: scan for iio:deviceX */
    dir = opendir(dev->spi_path);
    if (!dir)
        return -errno;

    while ((ent = readdir(dir)) != NULL) {
        /*
         * IIO devices are exposed as directories named:
         *   iio:device0, iio:device1, ...
         */
        if (strncmp(ent->d_name, "iio:device", 10) == 0) {

            if (found) {
                /* More than one IIO device = unexpected state */
                closedir(dir);
                return -EEXIST;
            }

            /* STEP 2.3: build full IIO device path */
            snprintf(dev->device_path, sizeof(dev->device_path),
                     "%s/%s", dev->spi_path, ent->d_name);

            found = 1;
        }
    }

    closedir(dir);

    if (!found)
        return -ENODEV;

    return 0;
}

/*! ****************************************************************************
 * @fn afe11612_read_device_id
 * @brief Read the device ID of the AFE11612.
 * @param dev Pointer to the AFE11612 device structure.
 * @param id Pointer to store the device ID value.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
int afe11612_read_device_id(afe11612_t *dev, uint32_t *id)
{
    char path[512];

    if (!dev || !id)
        return -EINVAL;

    snprintf(path, sizeof(path), "%s/device_id", dev->spi_path);

    return read_file_value(path, AFE_VAL_UINT32_HEX, id);
}

/*! ****************************************************************************
 * @fn afe11612_read_voltage_raw
 * @brief Read raw voltage data from a specified ADC channel.
 * @param dev Pointer to the AFE11612 device structure.
 * @param channel ADC channel index.
 * @param value Pointer to store the raw voltage value.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
int afe11612_read_voltage_raw(afe11612_t *dev, int channel, int *value)
{
    char path[512];

    if (channel < 0 || channel >= AFE_MAX_VOLTAGES)
        return -EINVAL;

    snprintf(path, sizeof(path),
             "%s/in_voltage%d_raw", dev->device_path, channel);
    return read_file_value(path, AFE_VAL_INT, value);
}

/*! ****************************************************************************
 * @fn afe11612_read_voltage_input
 * @brief Read scaled voltage input from a specified ADC channel.
 * @param dev Pointer to the AFE11612 device structure.
 * @param channel ADC channel index.
 * @param value Pointer to store the scaled voltage value.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
int afe11612_read_voltage_input(afe11612_t *dev, int channel, double *value)
{
    char path[512];

    if (channel < 0 || channel >= AFE_MAX_VOLTAGES)
        return -EINVAL;

    snprintf(path, sizeof(path),
             "%s/in_voltage%d_input", dev->device_path, channel);
    return read_file_value(path, AFE_VAL_DOUBLE, value);
}

/*! ****************************************************************************
 * @fn afe11612_read_temp_raw
 * @brief Read raw temperature data from a specified sensor channel.
 * @param dev Pointer to the AFE11612 device structure.
 * @param channel Temperature sensor channel index.
 * @param value Pointer to store the raw temperature value.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
int afe11612_read_temp_raw(afe11612_t *dev, int channel, int *value)
{
    char path[512];

    if (channel < 0 || channel >= AFE_MAX_TEMPS)
        return -EINVAL;

    snprintf(path, sizeof(path),
             "%s/in_temp%d_raw", dev->device_path, channel);
    return read_file_value(path, AFE_VAL_INT, value);
}

/*! ****************************************************************************
 * @fn afe11612_read_temp_input
 * @brief Read scaled temperature input from a specified sensor channel.
 * @param dev Pointer to the AFE11612 device structure.
 * @param channel Temperature sensor channel index.
 * @param value Pointer to store the scaled temperature value.
 * @return 0 on success, negative errno value on failure.
 *****************************************************************************/
int afe11612_read_temp_input(afe11612_t *dev, int channel, double *value)
{
    char path[512];

    if (channel < 0 || channel >= AFE_MAX_TEMPS)
        return -EINVAL;

    snprintf(path, sizeof(path),
             "%s/in_temp%d_input", dev->device_path, channel);
    return read_file_value(path, AFE_VAL_DOUBLE, value);
}