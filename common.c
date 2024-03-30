#include "common.h"

bool AUTHENTIFIED = false;       // TODO
char DISPLAY_NAME[MAX_DNAME];    // TODO
char CURRENT_STATE[MAX_DNAME];   // TODO

void handle_signal() {
    // Your implementation here
}

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