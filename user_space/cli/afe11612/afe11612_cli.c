/*! **************************************************************************
* @file    afe11612_cli.c
* @author  Omri Cohen
* @date    12 Apr 2026
* @brief   Cli application to read device id, temp and voltage from afe11612
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "libafe11612.h"

static void usage(const char *prog)
{
    printf(
        "Usage: %s --dev <spiX.Y> [OPTIONS]\n\n"
        "Options:\n"
        "  --dev <spiX.Y>   SPI device (e.g. spi0.0, spi0.1)\n"
        "  --id             Read device ID\n"
        "  --voltage <ch>   Read voltage channel\n"
        "  --temp <ch>      Read temperature channel\n"
        "  --raw            Read raw value (default: scaled/input)\n"
        "  -h               Show this help\n\n"
        "Examples:\n"
        "  %s --dev spi0.0 --id\n"
        "  %s --dev spi0.1 --voltage 2\n"
        "  %s --dev spi0.0 --temp 1 --raw\n",
        prog, prog, prog, prog);
}

int main(int argc, char **argv)
{
    afe11612_t dev;
    const char *spi_dev = NULL;

    int read_id = 0;
    int read_voltage = 0;
    int read_temp = 0;
    int raw = 0;
    int channel = -1;

    int i, ret;

    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--dev") && i + 1 < argc) {
            spi_dev = argv[++i];
        } else if (!strcmp(argv[i], "--id")) {
            read_id = 1;
        } else if (!strcmp(argv[i], "--voltage") && i + 1 < argc) {
            read_voltage = 1;
            channel = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--temp") && i + 1 < argc) {
            read_temp = 1;
            channel = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--raw")) {
            raw = 1;
        } else if (!strcmp(argv[i], "-h")) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    /* Validate arguments */
    if (!spi_dev) {
        fprintf(stderr, "Error: --dev is required\n");
        return 1;
    }

    if (read_id + read_voltage + read_temp != 1) {
        fprintf(stderr, "Error: specify exactly one action (--id | --voltage | --temp)\n");
        return 1;
    }

    /* Initialize device */
    ret = afe11612_init(&dev, spi_dev);
    if (ret) {
        fprintf(stderr, "Failed to init %s: %s\n", spi_dev, strerror(-ret));
        return 1;
    }

    /* Execute command */
    if (read_id) {
        uint32_t id;
        ret = afe11612_read_device_id(&dev, &id);
        if (ret) {
            fprintf(stderr, "Failed to read device ID: %s\n", strerror(-ret));
            return 1;
        }
        printf("Device ID: 0x%04x\n", id);
    }

    if (read_voltage) {
        if (raw) {
            int value;
            ret = afe11612_read_voltage_raw(&dev, channel, &value);
            if (ret) {
                fprintf(stderr, "Voltage raw read failed: %s\n", strerror(-ret));
                return 1;
            }
            printf("Voltage[%d] raw: %d\n", channel, value);
        } else {
            double value;
            ret = afe11612_read_voltage_input(&dev, channel, &value);
            if (ret) {
                fprintf(stderr, "Voltage input read failed: %s\n", strerror(-ret));
                return 1;
            }
            printf("Voltage[%d]: %.6f\n", channel, value);
        }
    }

    if (read_temp) {
        if (raw) {
            int value;
            ret = afe11612_read_temp_raw(&dev, channel, &value);
            if (ret) {
                fprintf(stderr, "Temp raw read failed: %s\n", strerror(-ret));
                return 1;
            }
            printf("Temp[%d] raw: %d\n", channel, value);
        } else {
            double value;
            ret = afe11612_read_temp_input(&dev, channel, &value);
            if (ret) {
                fprintf(stderr, "Temp input read failed: %s\n", strerror(-ret));
                return 1;
            }
            printf("Temp[%d]: %.6f\n", channel, value);
        }
    }

    return 0;
}