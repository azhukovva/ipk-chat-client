#include "common.h"

bool AUTHENTIFIED = false;  
char DISPLAY_NAME[MAX_DNAME];  
char CURRENT_STATE[MAX_DNAME];   

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
}

bool is_valid_parameter(const char *str, bool allow_spaces)
{ // allow_spaces is true -> spaces are allowed in the input string
    // allow_spaces is true for display_name
    while (*str)
    {
        if (!isalnum((unsigned char)*str) && *str != '-' && (!allow_spaces || (!isprint((unsigned char)*str) && *str != ' ')))
        {
            return false;
        }
        str++;
    }
    return true;
}

void init_message(struct message_info_t *message) {
    memset(message->username, 0, sizeof(message->username));
    memset(message->secret, 0, sizeof(message->secret));
    memset(message->display_name, 0, sizeof(message->display_name));
    memset(message->channel_id, 0, sizeof(message->channel_id));
    memset(message->additional_params, 0, sizeof(message->additional_params));
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