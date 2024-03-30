#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

// TODO
#define MAX_DNAME 20

extern bool AUTHENTIFIED;       // TODO
extern char DISPLAY_NAME[MAX_DNAME];    // TODO
extern char CURRENT_STATE[MAX_DNAME];   // TODO

enum command_type_t
{
    AUTH,
    JOIN,
    RENAME,
    HELP,
    UNKNOWN
};

enum command_type_t get_command_type(const char *command);

/**
 * TODO
 */
void print_help();

/**
 * TODO
 */
void clean(int socket_desc, int epollfd);

#endif