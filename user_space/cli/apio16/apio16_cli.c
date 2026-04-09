/*! **************************************************************************
* @author   Omri Cohen
* @date     23 Mar 2026
* @file     apio16_cli.c
* @brief    cli application for linux to controll apio16 device
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
#include <string.h>
#include <stdlib.h>
#include "libapio16.h"

void print_help() {
    printf("Commands:\n");
    printf("  r <pin>              - read level\n");
    printf("  w <pin> <0|1>        - write level\n");
    printf("  dir <pin>            - get direction\n");
    printf("  dir <pin> <in|out>   - set direction\n");
    printf("  reset                - reset (0-7 input, 8-15 output low)\n");
    printf("  q                    - quit\n\n");
}

int main() {
    char cmd[128];

    if (apio16_init() != 0) {
        printf("Failed to init APIO16\n");
        return -1;
    }

    print_help();

    while (1) {
        printf("apio16> ");
        fflush(stdout);

        if (!fgets(cmd, sizeof(cmd), stdin))
            break;

        if (cmd[0] == 'q')
            break;

        char op[16], arg1[16];
        int pin, val;

        int n = sscanf(cmd, "%s %d %s", op, &pin, arg1);

        if (!strcmp(op, "r") && n >= 2) {
            apio16_level_t v;
            if (apio16_get_level(pin, &v) == 0)
                printf("Pin %d = %d\n", pin, v);
            else
                printf("Read failed\n");

        } else if (!strcmp(op, "w") && n == 3) {
            val = atoi(arg1);
            if (apio16_set_level(pin, val ? APIO16_HIGH : APIO16_LOW) == 0)
                printf("Pin %d set to %d\n", pin, val);
            else
                printf("Write failed\n");

        } else if (!strcmp(op, "dir")) {
            if (n == 2) {
                apio16_direction_t d;
                if (apio16_get_direction(pin, &d) == 0)
                    printf("Pin %d direction = %s\n", pin,
                        d == APIO16_DIR_INPUT ? "input" : "output");
                else
                    printf("Get direction failed\n");

            } else if (n == 3) {
                if (!strcmp(arg1, "in")) {
                    apio16_set_direction(pin, APIO16_DIR_INPUT);
                } else if (!strcmp(arg1, "out")) {
                    apio16_set_direction(pin, APIO16_DIR_OUTPUT);
                } else {
                    printf("Use 'in' or 'out'\n");
                    continue;
                }
                printf("Pin %d direction updated\n", pin);
            }

        } else if (!strcmp(op, "reset")) {
            if (apio16_reset() == 0)
                printf("APIO16 reset complete\n");
            else
                printf("APIO16 reset FAILED\n");

        } else {
            print_help();
        }
    }

    apio16_close();
    return 0;
}