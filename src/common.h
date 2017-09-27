#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>              // printf

#define LOG(str, args...) \
do \
{ \
    printf(str "\n", ##args); \
} while(0)

#define COLOR_RED    "\x1b[1;91m"
#define COLOR_GREEN  "\x1b[1;92m"
#define COLOR_YELLOW "\x1b[1;93m"
#define COLOR_BLUE   "\x1b[1;94m"
#define COLOR_PURPLE "\x1b[1;95m"
#define COLOR_CYAN   "\x1b[1;96m"
#define COLOR_RESET  "\x1b[0m"

#endif
