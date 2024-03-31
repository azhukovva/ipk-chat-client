#include "udp.h"
#include "common.h"

int socket_desc_udp = -1;
int epollfd_udp = -1;

struct epoll_event event_udp;
struct timeval time_value;
struct sockaddr_in server_addr;
struct epoll_event events[MAX_EVENTS];

enum response_type_t
{
    CONFIRM,
    REPLY,
    MSG,
    ERR,
    BYE
};

// Function to wait for a confirmation message from the server after sending a message
bool is_confirmed(int socket_desc_udp, char *message, int message_length, struct sockaddr *server_addr, socklen_t server_addr_len, uint16_t expected_message_id, int timeout, int retransmissions)
{
    // REVIEW - нужно ещё раз объявлять таймаут?
    struct timeval time_value;
    time_value.tv_sec = 0;
    time_value.tv_usec = timeout * 1000; // Setting the timeout with microsecond precision

    for (int i; i < retransmissions; i++)
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_desc_udp, &read_fds);

        // Function to monitor the file descriptors for readability within the specified timeout period
        int action = select(socket_desc_udp + 1, &read_fds, NULL, NULL, &time_value); // 0 if the timeout expires

        if (action > 0 && FD_ISSET(socket_desc_udp, &read_fds)) // At least one socket is ready for reading
        {
            char message[MAX_CHAR];
            struct sockaddr_in server_addr;
            socklen_t server_addr_len = sizeof(server_addr);

            int received_data = recvfrom(socket_desc_udp, message, sizeof(message) - 1, 0, (struct sockaddr *)&server_addr, &server_addr_len);
            if (received_data < 0)
            {
                fprintf(stderr, "Error: Error while receiving data from the server\n");
                clean(socket_desc_udp, epollfd_udp);
                return false;
            }

            // Check if the received message is the expected one
            //                                                      MSB                         LSB
            // to uint8 because of the << operation to prevent sign extension issues
            if (received_data >= 3 && message[0] == 0x00 && ((uint8_t)message[1] << 8 | (uint8_t)message[2]) == expected_message_id)
            {
                return true;
            }
            else
            {
                fprintf(stderr, "Error: Unexpected message received\n");
                clean(socket_desc_udp, epollfd_udp);
                return false;
            }
        }
        else if (action == 0)
        {
            // Timeout expired -> resend the message
            int sendto_check = sendto(socket_desc_udp, message, message_length, 0, server_addr, server_addr_len);
            if (sendto_check < 0)
            {
                fprintf(stderr, "Error: Error while sending data to the server\n");
                clean(socket_desc_udp, epollfd_udp);
                return false;
            }
        }
        else
        {
            fprintf(stderr, "Error: Error while waiting for the server's response\n");
            clean(socket_desc_udp, epollfd_udp);
            return false;
        }
    }
    return false;
}

void uint16_to_char_array(uint16_t value, char *char_array)
{
    char_array[0] = value >> 8; // MSB
    char_array[1] = value;      // LSB
}

void add(int index, char *message, char *to_add)
{
    int length = strlen(to_add);
    memcpy(message + index, to_add, length);
    index += length;
    message[index] = '\0';
}

int create_auth_message_udp(uint16_t message_id, char *message, char *username, char *display_name, char *secret)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = AUTH; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(index, message, username);
    add(index, message, DISPLAY_NAME);
    add(index, message, secret);

    return index;
}

int create_join_message_udp(uint16_t message_id, char *message, char *channel_id, char *display_name)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    // index++ -> where the next data should be written
    message[index++] = JOIN; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(index, message, channel_id);
    add(index, message, DISPLAY_NAME);

    return index;
}

int create_confirm_message_udp(uint16_t confirmed_message_id, char *message)
{
    char confirmed_message_id_char[2];
    uint16_to_char_array(confirmed_message_id, confirmed_message_id_char);

    int index = 0;
    message[index++] = CONFIRM; // Add identifier
    memcpy(message + index, confirmed_message_id_char, 2);
    index += 2;

    return index;
}

int create_msg_message_udp(uint16_t message_id, char *message, char *display_name, char *message_content)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = MSG; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(index, message, DISPLAY_NAME);
    add(index, message, message_content);

    return index;
}

int create_err_message_udp(uint16_t message_id, char *message, char *display_name, char *message_content)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = ERR; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(index, message, DISPLAY_NAME);
    add(index, message, message_content);

    return index;
}

int create_bye_message_udp(uint16_t message_id, char *message)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = BYE; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    return index;
}

void handle_input_command_udp(int socket_desc_tcp, char *command, uint16_t message_id, char *display_name, int timeout, int retransmissions)
{
    strncpy(CURRENT_STATE, command, 7);
    enum command_type_t cmd_type = get_command_type(command);
    struct message_info_t message; // empty stucture
    init_message(&message);

    char data[MAX_CHAR];

    switch (cmd_type)
    {
    case AUTH: {
        if (AUTHENTIFIED)
        {
            fprintf(stderr, "ERR: You are already authentified\n");
            return;
        }

        sscanf(command, "/auth %s %s %s %99[^\n]", message.username, message.secret, message.display_name, message.additional_params);

        if (strlen(message.username) == 0 || strlen(message.secret) == 0 || strlen(message.display_name) == 0 || strlen(message.additional_params) > 0 ||
            !is_valid_parameter(message.username, false) || !is_valid_parameter(message.secret, false) || !is_valid_parameter(message.display_name, true))
        {
            fprintf(stderr, "ERR: Invalid parameters for /auth\n");
            return;
        }

        strncpy(DISPLAY_NAME, message.display_name, MAX_DNAME);
        // create an auth message
        int messsage_size = create_auth_message_udp(message_id++, data, message.username, message.display_name, message.secret);
        // send the message
        sendto(socket_desc_udp, command, messsage_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        epoll_ctl(epollfd_udp, EPOLL_CTL_DEL, STDIN_FILENO, &event_udp);
        // wait for confirmation of the previously sent message
        // because of index++ in the create_auth_message_udp function
        if (!is_confirmed(socket_desc_udp, data, messsage_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id - 1, timeout, retransmissions))
        {
            fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
            clean(socket_desc_udp, epollfd_udp);
            exit(EXIT_FAILURE);
        }
        break;
    }
    case JOIN: {
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
        // create a join message
        // message_id++ ensures that each time a join message is created, it receives a unique ID
        int messsage_size = create_join_message_udp(message_id++, data, message.channel_id, message.display_name);
        // send the message
        sendto(socket_desc_udp, command, messsage_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        epoll_ctl(epollfd_udp, EPOLL_CTL_DEL, STDIN_FILENO, &event_udp);
        // wait for confirmation
        if (!is_confirmed(socket_desc_udp, data, messsage_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id - 1, timeout, retransmissions))
        {
            fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
            clean(socket_desc_udp, epollfd_udp);
            exit(EXIT_FAILURE);
        }
        break;
    }

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
        break;

    case HELP:
        sscanf(command, "/help %99[^\n]", message.additional_params);
        if (strlen(message.additional_params) > 0)
        {
            fprintf(stderr, "ERR: Invalid parameters for /help\n");
            break;
        }
        print_help();
        break;
    default:
        fprintf(stderr, "ERR: Unknown command: %s\n", command);
    }
}

void hadle_server_response_udp()
{
    // TODO
}

static void handle_signal()
{
    char message[MAX_CHAR];  // Messages that will be sent to the server
    if (strlen(CURRENT_STATE) == 0) // No command has been issued before
    {
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_SUCCESS);
    }
    // TODO
    //  bye message to message
    //  wait for confirmation from the server

    clean(socket_desc_udp, epollfd_udp);
    exit(EXIT_SUCCESS);
}

int udp_connect(char *server_ip, int port, int timeout, int retransmissions)
{
    signal(SIGINT, handle_signal); // interrupting signal

    // Creating a socket + setting the receive timeout
    socket_desc_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_desc_udp < 0)
    {
        fprintf(stderr, "Error: Error while creating a socket\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    time_value.tv_sec = 0;
    time_value.tv_usec = timeout;
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
    for (;;)
    {
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
        for (int i = 0; i < nfds; ++i)
        {
            // Checks if the event is for the UDP socket
            // SOCKET
            if (events[i].data.fd == socket_desc_udp)
            {
                // There's incoming data to be read from the socket
                struct sockaddr_in sender_addr;
                socklen_t sender_addr_len = sizeof(sender_addr);

                // REVIEW - ssize_t?
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
            else if (events[i].data.fd == STDIN_FILENO)
            {
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
                    // handle_input_command_udp();
                }
                else // input is a message
                {
                    // REVIEW - надо?
                    //  if (!AUTHENTIFIED)
                    //  {
                    //      // should enter the /auth command -> check input again
                    //      fprintf(stderr, "ERR: You must be authentified first\n");
                    //      continue;
                    //  }
                    //  if (!is_valid_parameter(input, true))
                    //  {
                    //      fprintf(stderr, "ERR: Invalid message content\n");
                    //      continue;
                    //  }

                    // create a message

                    // send the message
                }
            }
        }
    }

    return EXIT_SUCCESS;
}