#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// Colors for debugging
#define COLOR_CYAN "\033[0;36m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

// Debug
// #define DEBUG

void debug(const char* message, ...);

#endif