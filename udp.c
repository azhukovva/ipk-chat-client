#include "udp.h"

bool AUTHENTIFIED = false;
char DISPLAY_NAME[MAX_DNAME];
char CURRENT_STATE[MAX_DNAME];

int socket_desc_tcp = -1;
int epollfd_tcp = -1;

struct epoll_event event_udp;

int create_auth_message_udp()
{
}

int create_join_message_udp()
{
}

int create_confirm_message_udp()
{
}

int create_msg_message_udp()
{
}

int create_err_message_udp()
{
}

int create_bye_message_udp()
{
}

void print_error()
{
}

void handle_input_command_udp()
{
}

void hadle_server_response_udp()
{
}

void handle_signal()
{
    clean(socket_desc_tcp, epollfd_tcp);
    exit(EXIT_SUCCESS);
}

int udp_connect(char *server_ip, int port, int timeout, int retransmissions)
{

    return EXIT_SUCCESS;
}