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
#include "cjson/cJSON.h"
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

#define AFE_DAC_CONFIG_PATH "/etc/afe11612/afe_dac_config.json"

int afe11612_write_dac_mv(afe11612_t *dev, int channel, int mv);

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
 * @fn write_file_int
 * @brief Write an integer value to a sysfs file.
 * @param path   Path to the sysfs file.
 * @param value  Integer value to write.
 * @return 0 on success, negative errno value on failure.
 * @note This helper is intended for sysfs attributes that accept
 *       single integer values (e.g. DAC voltage in mV).
 *****************************************************************************/
static int write_file_int(const char *path, int value)
{
    FILE *f;

    f = fopen(path, "w");
    if (!f)
        return -errno;

    fprintf(f, "%d", value);

    fclose(f);
    return 0;
}

/**
 * @fn afe11612_load_dac_config
 * @brief Load DAC configuration from JSON file (array-based format)
 * Expected format:
 * {
 *   "dac0": [500, 1000, 1500, ...]
 * }
 * @param dev pointer to device
 * @param path path to JSON config file
 * @return 0 on success, negative on error
 */
int afe11612_load_dac_config(afe11612_t *dev, const char *path)
{
    FILE *fp;
    long len;
    char *buf;
    cJSON *json, *arr;

    if (!dev || !path)
        return -1;

    fp = fopen(path, "rb");
    if (!fp)
        return -2;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);

    if (len <= 0) {
        fclose(fp);
        return -3;
    }

    buf = malloc(len + 1);
    if (!buf) {
        fclose(fp);
        return -4;
    }

    if (fread(buf, 1, len, fp) != (size_t)len) {
        fclose(fp);
        free(buf);
        return -5;
    }

    buf[len] = '\0';
    fclose(fp);

    json = cJSON_Parse(buf);
    free(buf);

    if (!json)
        return -6;

    arr = cJSON_GetObjectItem(json, "dac0");
    if (!cJSON_IsArray(arr)) {
        cJSON_Delete(json);
        return -7;
    }

    for (int i = 0; i < cJSON_GetArraySize(arr); i++) {
        cJSON *v = cJSON_GetArrayItem(arr, i);
        if (cJSON_IsNumber(v))
            afe11612_write_dac_mv(dev, i, v->valueint);
    }

    cJSON_Delete(json);
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
    int found = 0, ret = 0;

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

    ret = afe11612_load_dac_config(dev, AFE_DAC_CONFIG_PATH);
    if (ret != 0)
        printf("failed to afe11612_load_dac_config");


    return 0;
}

/**
 * @brief Set DAC output voltage (in millivolts)
 * @param dev     AFE11612 device handle
 * @param channel DAC channel index (0–11)
 * @param mv      Output voltage in millivolts
 * @return 0 on success, negative errno value on failure
 * @note The driver is responsible for converting mV to raw DAC code.
 */
int afe11612_write_dac_mv(afe11612_t *dev, int channel, int mv)
{
    char path[512];

    if (!dev || channel < 0 || channel >= AFE_MAX_DACS)
        return -EINVAL;

    snprintf(path, sizeof(path),
             "%s/out_voltage%d_raw",
             dev->device_path, channel);

    /* mv is written directly; driver converts mv → DAC code */
    return write_file_int(path, mv);
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

    if (!dev || channel < 0 || channel >= AFE_MAX_VOLTAGES)
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

    if (!dev || channel < 0 || channel >= AFE_MAX_VOLTAGES)
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

    if (!dev || channel < 0 || channel >= AFE_MAX_TEMPS)
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

    if (!dev || channel < 0 || channel >= AFE_MAX_TEMPS)
        return -EINVAL;

    snprintf(path, sizeof(path),
             "%s/in_temp%d_input", dev->device_path, channel);
    return read_file_value(path, AFE_VAL_DOUBLE, value);
}