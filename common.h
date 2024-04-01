#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "libs.h"

// TODO
#define MAX_DNAME 20
#define MAX_ID 20
#define MAX_SECRET 128
#define MAX_CONTENT 1400
#define MAX_USERNAME 20
// Default Ethernet MTU(Maximum Transmission Unit) == 1500 bytes
#define MAX_CHAR 1500
#define MAX_EVENTS 1

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

struct message_info_t
{
    char username[MAX_USERNAME];
    char secret[MAX_SECRET];
    char display_name[MAX_DNAME];
    char channel_id[MAX_ID];
    char additional_params[MAX_CONTENT]; // any characters up to 99 characters that are not a newline character
};

enum command_type_t get_command_type(const char *command);

bool is_valid_parameter(const char *str, bool allow_spaces);

void init_message(struct message_info_t *message);

void print_help();

void clean(int socket_desc, int epollfd);

#endif