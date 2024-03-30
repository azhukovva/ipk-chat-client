#include "debug.h"

/**
 * TODO
 */
void debug(const char* message, ...) {
#ifdef DEBUG
    va_list args;
    va_start(args, message);
    printf(COLOR_CYAN "[DEBUG] " COLOR_RESET);
    vprintf(message, args);
    printf("\n");
    va_end(args);
#else
    message = message;
#endif
}