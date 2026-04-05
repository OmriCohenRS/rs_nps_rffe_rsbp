/*! **************************************************************************
* @file    libapio16.c
* @author  Omri Cohen
* @date    23 Mar 2026
* @brief   Library wrapper for Linux GPIO driver controlling the APIO16 SPI expander.
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

/*! **************************************************************************
* This module provides an abstraction layer for accessing the APIO16 GPIO expander
* via libgpiod. It includes initialization, direction control, level control, and
* reset functionality.
******************************************************************************
*/

#include "libapio16.h"
#include <stdio.h>
#include <stdlib.h>

static struct gpiod_chip *chip = NULL;

/* ================= INIT ================= */

/*! ****************************************************************************
 * @fn     apio16_init
 * @brief  Initialize APIO16 by opening the associated GPIO chip.
 * @return \b val - <br>
 *         0 for success <br>
 *        -1 if gpiod_chip_open() failed
 *****************************************************************************/
int apio16_init(void)
{
    chip = gpiod_chip_open(APIO16_CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return -1;
    }
    return 0;
}

/*! ****************************************************************************
 * @fn     apio16_close
 * @brief  Close APIO16 chip and release resources.
 * @return None
 *****************************************************************************/
void apio16_close(void)
{
    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}

/* ================= INTERNAL ================= */

/*! ****************************************************************************
 * @fn     request_line
 * @brief  Internal helper for requesting a GPIO line with direction and default value.
 * @param  line - Line index (0–15).
 * @param  direction - GPIOD direction (input/output).
 * @param  value - Initial output value.
 * @return Pointer to gpiod_line_request or NULL if failed.
 *****************************************************************************/
static struct gpiod_line_request* request_line(
    int line,
    enum gpiod_line_direction direction,
    enum gpiod_line_value value)
{
    struct gpiod_line_settings *settings = NULL;
    struct gpiod_line_config *config = NULL;
    struct gpiod_request_config *req_cfg = NULL;
    struct gpiod_line_request *request = NULL;

    settings = gpiod_line_settings_new();
    config   = gpiod_line_config_new();
    req_cfg  = gpiod_request_config_new();

    if (!settings || !config || !req_cfg)
        goto error;

    gpiod_line_settings_set_direction(settings, direction);
    if (direction == GPIOD_LINE_DIRECTION_OUTPUT)
        gpiod_line_settings_set_output_value(settings, value);

    unsigned int offsets[] = { (unsigned int)line };

    if (gpiod_line_config_add_line_settings(config, offsets, 1, settings) < 0)
        goto error;

    gpiod_request_config_set_consumer(req_cfg, "apio16");

    request = gpiod_chip_request_lines(chip, req_cfg, config);
    if (!request)
        goto error;

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);
    gpiod_request_config_free(req_cfg);
    return request;

error:
    perror("request_line");
    if (settings) gpiod_line_settings_free(settings);
    if (config)   gpiod_line_config_free(config);
    if (req_cfg)  gpiod_request_config_free(req_cfg);
    return NULL;
}

/* ================= DIRECTION ================= */

/*! ****************************************************************************
 * @fn     apio16_set_direction
 * @brief  Configure line direction (input/output).
 * @param  line - Line index (0–15).
 * @param  dir  - APIO16_DIR_INPUT or APIO16_DIR_OUTPUT.
 * @return \b val - <br>
 *         0 for success <br>
 *        -1 if request_line() failed
 *****************************************************************************/
int apio16_set_direction(int line, apio16_direction_t dir)
{
    struct gpiod_line_request *req;

    req = request_line(
            line,
            dir == APIO16_DIR_INPUT ?
                GPIOD_LINE_DIRECTION_INPUT :
                GPIOD_LINE_DIRECTION_OUTPUT,
            GPIOD_LINE_VALUE_INACTIVE);

    if (!req)
        return -1;

    gpiod_line_request_release(req);
    return 0;
}

/*! ****************************************************************************
 * @fn     apio16_get_direction
 * @brief  Read current line direction.
 * @param  line - Line index (0–15).
 * @param  dir  - Output pointer storing direction.
 * @return \b val - <br>
 *         0 for success <br>
 *        -1 if gpiod_chip_get_line_info() failed
 *****************************************************************************/
int apio16_get_direction(int line, apio16_direction_t *dir)
{
    struct gpiod_line_info *info;

    info = gpiod_chip_get_line_info(chip, line);
    if (!info) {
        perror("get_line_info");
        return -1;
    }

    enum gpiod_line_direction d = gpiod_line_info_get_direction(info);

    *dir = (d == GPIOD_LINE_DIRECTION_INPUT) ?
            APIO16_DIR_INPUT : APIO16_DIR_OUTPUT;

    gpiod_line_info_free(info);
    return 0;
}

/* ================= LEVEL ================= */

/*! ****************************************************************************
 * @fn     apio16_set_level
 * @brief  Set a line output level (high/low).
 * @param  line  - Line index (0–15).
 * @param  value - APIO16_HIGH or APIO16_LOW.
 * @return \b val - <br>
 *         0 for success <br>
 *        -1 if request_line() failed
 *****************************************************************************/
int apio16_set_level(int line, apio16_level_t value)
{
    struct gpiod_line_request *req;

    req = request_line(
            line,
            GPIOD_LINE_DIRECTION_OUTPUT,
            value == APIO16_HIGH ?
                GPIOD_LINE_VALUE_ACTIVE :
                GPIOD_LINE_VALUE_INACTIVE);

    if (!req)
        return -1;

    gpiod_line_request_release(req);
    return 0;
}

/*! ****************************************************************************
 * @fn     apio16_get_level
 * @brief  Read a line level.
 * @param  line  - Line index (0–15).
 * @param  value - Output pointer storing level.
 * @return \b val - <br>
 *         0 for success <br>
 *        -1 if request_line() failed or read error
 *****************************************************************************/
int apio16_get_level(int line, apio16_level_t *value)
{
    struct gpiod_line_request *req;
    enum gpiod_line_value val;

    req = request_line(
            line,
            GPIOD_LINE_DIRECTION_INPUT,
            GPIOD_LINE_VALUE_INACTIVE);

    if (!req)
        return -1;

    val = gpiod_line_request_get_value(req, line);
    gpiod_line_request_release(req);

    if (val < 0) {
        perror("get_value");
        return -1;
    }

    *value = (val == GPIOD_LINE_VALUE_ACTIVE) ? APIO16_HIGH : APIO16_LOW;
    return 0;
}

/*! ****************************************************************************
 * @fn     apio16_reset
 * @brief  Reset APIO16 to default configuration.
 *         Lines 0–7 = inputs
 *         Lines 8–15 = outputs (low)
 * @return \b val - <br>
 *         0 for success <br>
 *        -1 on direction/level setup failure
 *****************************************************************************/
int apio16_reset(void)
{
    for (int i = 0; i < 16; i++) {
        if (i < 8) {
            if (apio16_set_direction(i, APIO16_DIR_INPUT) != 0)
                return -1;
        } else {
            if (apio16_set_level(i, APIO16_LOW) != 0)
                return -1;
        }
    }
    return 0;
}
