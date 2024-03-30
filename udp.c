#include "udp.h"

bool AUTHENTIFIED = false;
char DISPLAY_NAME[MAX_DNAME];
char CURRENT_STATE[MAX_DNAME];

int socket_desc_udp = -1;
int epollfd_udp = -1;

struct epoll_event event_udp;
struct timeval time_value;
struct sockaddr_in server_addr;
struct epoll_event events[MAX_EVENTS];


bool is_confirmed(){
    
}

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
    char message[MAX_CHAR]; // Messages that will be sent to the server
    if (CURRENT_STATE == "") // No command has been issued before
    {
        cleanup(socket_desc_udp, epollfd_udp);
        exit(EXIT_SUCCESS);
    }
    // bye message to message
    // wait for confirmation from the server

    clean(socket_desc_udp, epollfd_udp);
    exit(EXIT_SUCCESS);
}

int udp_connect(char *server_ip, int port, int timeout, int retransmissions)
{
    signal(SIGINT, handle_signal); // interrupting signal
    // Setting the timeout with microsecond precision
    timeout *= 1000;
    time_value.tv_sec = 0;
    time_value.tv_usec = timeout;
    
    // Creating a socket + setting the receive timeout
    socket_desc_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_desc_udp < 0)
    {
        fprintf(stderr, "Error: Error while creating a socket\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }
    int set_timeout = setsockopt(socket_desc_udp, SOL_SOCKET, SO_RCVTIMEO, &time_value, sizeof(time_value));
    if (set_timeout < 0)
    {
        fprintf(stderr, "Error: Error while setting the timeout\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    // Server adress setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // IP address string to binary format
    int ip_adress = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if (ip_adress <= 0) // error in converting the IP address string
    {
        fprintf(stderr, "Error: Invalid address\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    // Epoll initialization
    int epollfd_udp = epoll_create1(0);
    if (epollfd_udp < 0)
    {
        fprintf(stderr, "Error: Error while creating epoll\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    // Epoll event setup
    event_udp.events = EPOLLIN;
    event_udp.data.fd = socket_desc_udp;

    int socket_desc_check = epoll_ctl(epollfd_udp, EPOLL_CTL_ADD, socket_desc_udp, &event_udp);
    if (socket_desc_check < 0)
    {
        fprintf(stderr, "Error: Error while adding the socket to epoll\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    event_udp.data.fd = STDIN_FILENO; // fd for standard input
    int stdin_check = epoll_ctl(epollfd_udp, EPOLL_CTL_ADD, STDIN_FILENO, &event_udp);
    if (stdin_check < 0)
    {
        fprintf(stderr, "Error: Error while adding the standard input to epoll\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    // Waiting for events
    for (;;){
        // Waiting for events
        int nfds = epoll_wait(epollfd_udp, events, MAX_EVENTS, -1); // The number of events
        if (nfds < 0)
        {
            fprintf(stderr, "Error: Error while waiting for events\n");
            clean(socket_desc_udp, epollfd_udp);
            return EXIT_FAILURE;
        }

        char server_response[MAX_CHAR];

        // Handling the events
        for (int i = 0; i < nfds; ++i){
            // Checks if the event is for the UDP socket
            // SOCKET
            if (events[i].data.fd == socket_desc_udp){
                // There's incoming data to be read from the socket
                struct sockaddr_in sender_addr;
                socklen_t sender_addr_len = sizeof(sender_addr);

                //REVIEW - ssize_t?
                int recvfrom_check = recvfrom(socket_desc_udp, server_response, MAX_CHAR, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
                if (recvfrom_check < 0)
                {
                    fprintf(stderr, "Error: Error while receiving data from the server\n");
                    clean(socket_desc_udp, epollfd_udp);
                    return EXIT_FAILURE;
                }
                // Everything is OK!
                hadle_server_response_udp();
            }
            // STDIN
            else if (events[i].data.fd == STDIN_FILENO){
                // There's incoming data to be read from the standard input
                char input[MAX_CHAR];
                if (fgets(input, MAX_CHAR, stdin) == NULL)
                {
                    clean(socket_desc_udp, epollfd_udp);
                    return EXIT_SUCCESS;
                }
                if (strlen(input) == 1)
                {
                    // only newline(\n) character or EOF
                    // check input again
                    continue;
                }
                // There is a command in the input
                if (input[0] == '/') // input is a command
                {
                    handle_input_command_udp();
                }
                else // input is a message
                {
                    //REVIEW - надо?
                    // if (!AUTHENTIFIED)
                    // {
                    //     // should enter the /auth command -> check input again
                    //     fprintf(stderr, "ERR: You must be authentified first\n");
                    //     continue;
                    // }
                    // if (!is_valid_parameter(input, true))
                    // {
                    //     fprintf(stderr, "ERR: Invalid message content\n");
                    //     continue;
                    // }


                    // create a message

                    // send the message

                }
            }

        }
    }

    return EXIT_SUCCESS;
}