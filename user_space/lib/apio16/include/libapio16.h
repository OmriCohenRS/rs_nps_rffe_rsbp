/*! **************************************************************************
* @file     libapio16.h
* @author   Omri Cohen
* @date     23 Mar 2026
* @brief    Header for library wrap Linux kernel driver for apio16 gpio expander over spi
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

#ifndef LIBAPIO16_H
#define LIBAPIO16_H

#include <gpiod.h>

#ifdef __cplusplus
extern "C" {
#endif

#define APIO16_CHIP "/dev/gpiochip2"

/* Direction */
typedef enum {
    APIO16_DIR_INPUT = 0,
    APIO16_DIR_OUTPUT = 1
} apio16_direction_t;

/* Level */
typedef enum {
    APIO16_LOW = 0,
    APIO16_HIGH = 1
} apio16_level_t;

/* Init / cleanup */
int apio16_init(void);
void apio16_close(void);

/* Direction */
int apio16_set_direction(int line, apio16_direction_t dir);
int apio16_get_direction(int line, apio16_direction_t *dir);

/* Level */
int apio16_set_level(int line, apio16_level_t value);
int apio16_get_level(int line, apio16_level_t *value);

int apio16_reset(void);

#ifdef __cplusplus
}
#endif

#endif