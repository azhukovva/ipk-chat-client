#include "common.h"

bool AUTHENTIFIED = false;       // TODO
char DISPLAY_NAME[MAX_DNAME];    // TODO
char CURRENT_STATE[MAX_DNAME];   // TODO

void handle_signal() {
    // Your implementation here
}

enum command_type_t get_command_type(const char *command)
{
    if (strncmp(command, "/auth", 5) == 0)
    {
        return AUTH;
    }
    else if (strncmp(command, "/join", 5) == 0)
    {
        return JOIN;
    }
    else if (strncmp(command, "/rename", 7) == 0)
    {
        return RENAME;
    }
    else if (strncmp(command, "/help", 5) == 0)
    {
        return HELP;
    }
    else
    {
        fprintf(stderr, "ERR: Unknown command: %s\n", command);
        return UNKNOWN;
    }
};

void print_help() {
    printf("Command \t Parameters \t\t\t Description\n");
    printf("/auth \t\t Username Secret DisplayName \t Authenticates with the server\n");
    printf("/join \t\t ChannelID \t\t\t Joins a channel\n");
    printf("/rename \t DisplayName \t\t\t Changes your display name\n");
    printf("/help \t\t \t\t\t\t Prints this help\n");
}

void clean(int socket_desc, int epollfd)
{
    if (socket_desc != -1)
    {
        close(socket_desc);
    }
    if (epollfd != -1)
    {
        close(epollfd);
    }
}