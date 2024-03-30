#include "tcp.h"

// epoll -  I/O event notification facility.
// Allows monitoring multiple file descriptors to see if any of them are ready for I/O operations

// uninitialized file descriptors
// initialisation when create a socket
int socket_desc_tcp = -1;
int epollfd_tcp = -1;

struct epoll_event event_tcp;

enum command_type_t
{
    AUTH,
    JOIN,
    RENAME,
    HELP,
    UNKNOWN
};

enum response_type_t
{
    REPLY,
    MSG,
    ERR,
    BYE
};

struct message_info_t
{
    char username[MAX_USERNAME];
    char secret[MAX_SECRET];
    char display_name[MAX_DNAME];
    char channel_id[MAX_ID];
    char additional_params[MAX_CONTENT]; // any characters up to 99 characters that are not a newline character
};

char *create_auth_message_tcp(char *username, char *display_name, char *secret)
{
    // "AUTH"     SP ID    as DNAME using SECRET
    static char message[MAX_CHAR];
    snprintf(message, sizeof(message), "AUTH %s AS %s USING %s\r\n", username, display_name, secret);
    return message;
}

char *create_join_message_tcp(char *channel_id, char *display_name)
{
    static char message[MAX_CHAR];
    snprintf(message, sizeof(message), "JOIN %s AS %s\r\n", channel_id, display_name);
    return message;
}

char *create_msg_message_tcp(char *display_name, char *message_content)
{
    static char message[MAX_CHAR];
    snprintf(message, sizeof(message), "MSG FROM %s IS %s\r\n", display_name, message_content);
    return message;
}

char *create_err_message_tcp(char *display_name, char *message_content)
{
    static char message[MAX_CHAR];
    snprintf(message, sizeof(message), "ERR FROM %s IS %s\r\n", display_name, message_content);
    return message;
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

bool is_valid_parameter(const char *str, bool allow_spaces)
{ // allow_spaces is true -> spaces are allowed in the input string
    // allow_spaces is true for display_name
    while (*str)
    {
        if (!isalnum((unsigned char)*str) && *str != '-' && (!allow_spaces || !isprint((unsigned char)*str) && *str != ' '))
        {
            return false;
        }
        str++;
    }
    return true;
}

void print_error(char *message)
{
    fprintf(stderr, "ERR: %s\n", message);
    char *error_message = create_err_message_tcp(DISPLAY_NAME, message);
    send(socket_desc_tcp, error_message, strlen(error_message), 0);
    char bye_message[] = "BYE\r\n";
    send(socket_desc_tcp, bye_message, strlen(bye_message), 0);
    clean(socket_desc_tcp, epollfd_tcp);
    exit(EXIT_FAILURE);
}

void handle_input_command_tcp(char *command, int socket_desc_tcp, char *display_name)
{
    // "/auth", "/join", "/rename", "/help" are not expected to be longer than 7 characters
    // TODO (?)if strlen(command) > 7 exit
    // command type -> appropriate message
    strncpy(CURRENT_STATE, command, 7); // "/auth", "/join", "/rename", "/help" are not expected to be longer than 7 characters
    enum command_type_t cmd_type = get_command_type(command);
    struct message_info_t message; // empty stucture

    switch (cmd_type)
    {
    case AUTH:
        if (AUTHENTIFIED)
        {
            fprintf(stderr, "ERR: You are already authentified\n");
            return;
        }
        // reads input from the 'command' string
        // returns the number of items successfully matched and assigned
        sscanf(command, "/auth %s %s %s %99[^\n]", message.username, message.secret, message.display_name, message.additional_params);

        if (strlen(message.username) == 0 || strlen(message.secret) == 0 || strlen(message.display_name) == 0 || strlen(message.additional_params) > 0 ||
            !is_valid_parameter(message.username, false) || !is_valid_parameter(message.secret, false) || !is_valid_parameter(message.display_name, true))
        {
            fprintf(stderr, "ERR: Invalid parameters for /auth\n");
            return;
        }
        // we need to save it for the following requests
        strncpy(DISPLAY_NAME, message.display_name, MAX_DNAME);
        char *auth_message = create_auth_message_tcp(message.username, message.display_name, message.secret);

        // 1 - socket descriptor, 2 - pointer to the data, 3 - size, 4 - optional flags, default 0
        send(socket_desc_tcp, auth_message, strlen(auth_message), 0);
        epoll_ctl(epollfd_tcp, EPOLL_CTL_DEL, STDIN_FILENO, &event_tcp);
        break;

    case JOIN:
        if (!AUTHENTIFIED)
        {
            fprintf(stderr, "ERR: You must be authentified first\n");
            return;
        }

        sscanf(command, "/join %s %99[^\n]", message.channel_id, message.additional_params);
        if (strlen(message.channel_id) == 0 || strlen(message.additional_params) > 0 || !is_valid_parameter(message.channel_id, false))
        {
            fprintf(stderr, "ERR: Invalid parameters for /join\n");
            return;
        }

        char *join_message = create_join_message_tcp(message.channel_id, DISPLAY_NAME);
        send(socket_desc_tcp, join_message, strlen(join_message), 0);
        epoll_ctl(epollfd_tcp, EPOLL_CTL_DEL, STDIN_FILENO, &event_tcp);
        break;

    case RENAME:
        if (!AUTHENTIFIED)
        {
            fprintf(stderr, "ERR: You must be authentified first\n");
            return;
        }

        sscanf(command, "/rename %s %99[^\n]", DISPLAY_NAME, message.additional_params);
        if (strlen(DISPLAY_NAME) == 0 || strlen(message.additional_params) > 0 || !is_valid_parameter(DISPLAY_NAME, true))
        {
            fprintf(stderr, "ERR: Invalid parameters for /rename\n");
            return;
        }
        // Locally changes the display name of the user to be sent with new messages/selected commands
        strncpy(DISPLAY_NAME, message.display_name, MAX_DNAME);
        break;

    case HELP:
        sscanf(command, "/help %99[^\n]", message.additional_params);
        if (strlen(message.additional_params) > 0)
        {
            fprintf(stderr, "ERR: Invalid parameters for /help\n");
            return;
        }
        print_help();
        break;
    default:
        fprintf(stderr, "ERR: Unknown command: %s\n", command);
    }
}

// By breaking the response into tokens, the function can easily access and process each part of the response individually.
void hadle_server_response_tcp(char *response)
{
    // String 'response' -> smaller strings (tokens) with delimiter " "
    char *first_elem = strtok(response, " ");
    enum response_type_t response_type;

    // Determine the type of response
    // strcasecmp for case-insensitive comparison
    if (first_elem != NULL)
    {
        if (strcasecmp(first_elem, "REPLY") == 0)
        {
            response_type = REPLY;
        }
        else if (strcasecmp(first_elem, "MSG") == 0)
        {
            response_type = MSG;
        }
        else if (strcasecmp(first_elem, "ERR") == 0)
        {
            response_type = ERR;
        }
        else if (strncmp(first_elem, "BYE", 3) == 0)
        {
            response_type = BYE;
        }
        else
        {
            response_type = UNKNOWN;
        }
    }

    switch (response_type)
    {
    case REPLY:
    {
        // REPLY {"OK"|"NOK"} IS {MessageContent}\r\n
        char *status = strtok(NULL, " ");

        if (!(strcasecmp(status, "OK") == 0 && strcasecmp(status, "NOK") == 0))
        {
            print_error("Invalid/Unknown status in REPLY message");
        }
        strtok(NULL, " "); // IS
        char *message_content = strtok(NULL, "\r\n");
        if (message_content == NULL)
        {
            print_error("Invalid REPLY message");
        }

        if ((strncmp(CURRENT_STATE, "/auth", 5) == 0) || (strncmp(CURRENT_STATE, "/join", 5) == 0))
        {
            event_tcp.events = EPOLLIN;
            event_tcp.data.fd = STDIN_FILENO;

            if (epoll_ctl(epollfd_tcp, EPOLL_CTL_ADD, STDIN_FILENO, &event_tcp) == -1)
            {
                fprintf(stderr, "ERR: Error while adding descriptor\n");
                clean(socket_desc_tcp, epollfd_tcp);
                exit(EXIT_FAILURE);
            }
        }
        if (strcasecmp(status, "OK") == 0)
        {
            fprintf(stderr, "Success: %s\n", message_content);
            if (strncmp(CURRENT_STATE, "/auth", 5) == 0)
            {
                AUTHENTIFIED = true;
            }
        }
        // NOK
        else
        {
            fprintf(stderr, "Failure: %s\n", message_content);
        }
    }

    case MSG:
    {
        // MSG FROM {DisplayName} IS {MessageContent}\r\n
        if (!AUTHENTIFIED)
        {
            print_error("You must be authentified first");
        }
        strtok(NULL, " "); // FROM
        char *sender = strtok(NULL, " ");
        strtok(NULL, " "); // IS
        char *message_content = strtok(NULL, "\r\n");
        if (message_content == NULL || sender == NULL)
        {
            print_error("Invalid MSG message");
        }
        // Everything is OK!
        printf("%s: %s\n", sender, message_content);
    }
    case ERR:
    {
        // ERR FROM {DisplayName} IS {MessageContent}\r\n
        strtok(NULL, " "); // FROM
        char *sender = strtok(NULL, " ");
        strtok(NULL, " "); // IS
        char *message_content = strtok(NULL, "\r\n");

        if (message_content == NULL || sender == NULL)
        {
            print_error("Invalid ERR message");
        }

        char bye_message[] = "BYE\r\n";
        fprintf(stderr, "ERR FROM %s: %s\n", sender, message_content);
        send(socket_desc_tcp, bye_message, strlen(bye_message), 0);

        clean(socket_desc_tcp, epollfd_tcp);
        exit(EXIT_FAILURE);
    }

    case BYE:
    {
        // BYE\r\n
        if (strtok(NULL, "\r\n") != NULL) // Pointer to the next token
        {
            print_error("Invalid BYE message (extra strings after 'BYE' message)"); // Extra strings
        }
        clean(socket_desc_tcp, epollfd_tcp);
        exit(EXIT_SUCCESS);
    }
    default:
        print_error("Unknown message from server");
    }
}

static void handle_signal() 
{
    clean(socket_desc_tcp, epollfd_tcp);
    exit(EXIT_SUCCESS);
}

// IP adress + port number
int tcp_connect(char *server_ip, int port)
{
    signal(SIGINT, handle_signal); // ctrl+c

    struct sockaddr_in server;             // Stores the server's address information
    struct epoll_event events[MAX_EVENTS]; // Enent data
    char display_name[MAX_DNAME];          // Display name of the user

    // IPv4, TCP default
    socket_desc_tcp = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc_tcp == -1)
    {
        clean(socket_desc_tcp, epollfd_tcp);
        fprintf(stderr, "ERR: Error while creating a socket\n");
        return EXIT_FAILURE;
    }

    // IP, IPv4, port
    server.sin_addr.s_addr = inet_addr(server_ip); // IP to the binary form
    server.sin_family = AF_INET;
    server.sin_port = htons(port); // Host byte order -> network byte order

    printf("Ya socket %s %d\n", server_ip, port);

    // TODO add timeout

    if (connect(socket_desc_tcp, (struct sockaddr *)&server, sizeof(server)) != 0)
    {
        fprintf(stderr, "ERR: Connection error with tcp\n");
        clean(socket_desc_tcp, epollfd_tcp);
        return EXIT_FAILURE;
    }

    // Create epoll
    epollfd_tcp = epoll_create1(0); // 0 - no flags
    if (epollfd_tcp == -1)          // if fails
    {
        clean(socket_desc_tcp, epollfd_tcp);
        fprintf(stderr, "ERR: Error while creating epoll\n");
        return EXIT_FAILURE;
    }

    // Add the socket to the epoll instance
    event_tcp.events = EPOLLIN;
    event_tcp.data.fd = socket_desc_tcp;
    epoll_ctl(epollfd_tcp, EPOLL_CTL_ADD, socket_desc_tcp, &event_tcp);

    // Add STDIN_FILENO to the epoll instance
    event_tcp.events = EPOLLIN;
    event_tcp.data.fd = STDIN_FILENO;
    epoll_ctl(epollfd_tcp, EPOLL_CTL_ADD, STDIN_FILENO, &event_tcp);

    // Waiting for events
    for (;;)
    {
        int nfds = epoll_wait(epollfd_tcp, events, MAX_EVENTS, -1); // waits indefinitely

        if (nfds == -1)
        {
            clean(socket_desc_tcp, epollfd_tcp);
            fprintf(stderr, "ERR: Error_tcp in epoll_wait\n");
            return EXIT_FAILURE;
        }

        // loops over each event that occurred
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == socket_desc_tcp)
            {
                char response[MAX_CHAR];
                int bytes_received = recv(socket_desc_tcp, response, sizeof(response), 0);

                if (bytes_received <= 0) // if connection is closed
                {
                    fprintf(stderr, "ERR: Error while receiving bytes\n");
                    clean(socket_desc_tcp, epollfd_tcp);
                    return EXIT_FAILURE;
                }

                hadle_server_response_tcp(response); // process the response
            }

            else if (events[i].data.fd == STDIN_FILENO) // user input
            {
                char input[MAX_CHAR];

                // READ FROM STDIN
                if (fgets(input, sizeof(input), stdin) == NULL)
                {
                    clean(socket_desc_tcp, epollfd_tcp);
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
                    handle_input_command_tcp(input, socket_desc_tcp, display_name);
                }
                // message
                else
                {
                    if (!AUTHENTIFIED)
                    {
                        // should enter the /auth command -> check input again
                        fprintf(stderr, "ERR: You must be authentified first\n");
                        continue;
                    }
                    if (!is_valid_parameter(input, true))
                    {
                        fprintf(stderr, "ERR: Invalid message content\n");
                        continue;
                    }

                    char *msg_message = create_msg_message_tcp(DISPLAY_NAME, input);
                    send(socket_desc_tcp, msg_message, strlen(msg_message), 0);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}