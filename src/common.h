/* Copyright (c) 2017-2022 Siguza
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
**/

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>              // printf, fprintf, stderr

#define LOG(str, args...) \
do \
{ \
    printf(str "\n", ##args); \
} while(0)

#define ERR(str, args...) \
do \
{ \
    fprintf(stderr, str "\n", ##args); \
} while(0)

#define COLOR_RED    "\x1b[1;91m"
#define COLOR_GREEN  "\x1b[1;92m"
#define COLOR_YELLOW "\x1b[1;93m"
#define COLOR_BLUE   "\x1b[1;94m"
#define COLOR_PURPLE "\x1b[1;95m"
#define COLOR_CYAN   "\x1b[1;96m"
#define COLOR_RESET  "\x1b[0m"

typedef struct
{
    bool true_json;
    bool bytes_raw;
    bool first;
    int lvl;
    FILE *stream;
} common_ctx_t;

void common_print_bytes(common_ctx_t *ctx, const uint8_t *buf, size_t size);
void common_print_char(common_ctx_t *ctx, char c);

#endif
