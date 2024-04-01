#include "udp.h"
#include "common.h"

int socket_desc_udp = -1;
int epollfd_udp = -1;

struct epoll_event event_udp;
struct sockaddr_in server_addr;
struct epoll_event events[MAX_EVENTS];

uint16_t message_id = 0;

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
            // printf("%ld\n", strlen(message));
            // for (int i = 0; message[i] != '\0'; i++) {
            //     // Print the hexadecimal representation of each character
            //     printf("%02X ", message[i]);
            // }
            // printf("\n");

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

void add(int *index, char *message, char *to_add)
{
    int length = strlen(to_add);
    memcpy(message + (*index), to_add, length);
    *index += length;
    message[(*index)++] = '\0';
}

int create_auth_message_udp(uint16_t message_id, char *message, char *username, char *display_name, char *secret)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = 0x02; // AUTH -> add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(&index, message, username);
    add(&index, message, display_name);
    add(&index, message, secret);

    return index;
}

int create_join_message_udp(uint16_t message_id, char *message, char *channel_id, char *display_name)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    // index++ -> where the next data should be written
    message[index++] = 0x03; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(&index, message, channel_id);
    add(&index, message, DISPLAY_NAME);

    return index;
}

int create_confirm_message_udp(uint16_t received_message_id, char *message)
{
    char received_message_id_char[2];
    uint16_to_char_array(received_message_id, received_message_id_char);

    int index = 0;
    message[index++] = 0x00; // Add identifier
    memcpy(message + index, received_message_id_char, 2);
    index += 2;

    return index;
}

int create_msg_message_udp(uint16_t message_id, char *message, char *display_name, char *message_content)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = 0x04; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(&index, message, DISPLAY_NAME);
    add(&index, message, message_content);

    return index;
}

int create_err_message_udp(uint16_t message_id, char *message, char *display_name, char *message_content)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = 0xFE; // Add identifier
    memcpy(message + index, message_id_char, 2);
    index += 2;

    add(&index, message, DISPLAY_NAME);
    add(&index, message, message_content);

    return index;
}

int create_bye_message_udp(uint16_t message_id, char *message)
{
    char message_id_char[2];
    uint16_to_char_array(message_id, message_id_char);

    int index = 0;
    message[index++] = 0xFF; // Add identifier
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

    debug("Detected command: %s", command);

    char data[MAX_CHAR];

    switch (cmd_type)
    {
    case AUTH:
    {

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
        int message_size = create_auth_message_udp(message_id++, data, message.username, message.display_name, message.secret);
        // send the message
        sendto(socket_desc_udp, data, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        epoll_ctl(epollfd_udp, EPOLL_CTL_DEL, STDIN_FILENO, &event_udp);
        // wait for confirmation of the previously sent message
        // because of index++ in the create_auth_message_udp function
        printf("%s\n", data);
        if (!is_confirmed(socket_desc_udp, data, message_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id - 1, timeout, retransmissions))
        {
            fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
            clean(socket_desc_udp, epollfd_udp);
            exit(EXIT_FAILURE);
        }
        break;
    }
    case JOIN:
    {
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
        int message_size = create_join_message_udp(message_id++, data, message.channel_id, message.display_name);
        // send the message
        sendto(socket_desc_udp, command, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        epoll_ctl(epollfd_udp, EPOLL_CTL_DEL, STDIN_FILENO, &event_udp);
        // wait for confirmation
        if (!is_confirmed(socket_desc_udp, data, message_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id - 1, timeout, retransmissions))
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

bool is_duplicated_message(char *recent_message_array, int received_message_id, char *responses)
{
    for (int i = 0; i < strlen(recent_message_array); i++)
    {

        if (recent_message_array[i] == received_message_id)
        {
            int message_size = create_confirm_message_udp(message_id, responses);
            sendto(socket_desc_udp, responses, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
            return true;
        }
        return false;
    }
}

static void print_error(char *data, char *message, uint16_t message_id, int timeout, int retransmissions)
{
    fprintf(stderr, "ERR: %s\n", data);
    int message_size = create_err_message_udp(message_id++, data, DISPLAY_NAME, message);
    sendto(socket_desc_udp, message, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (!is_confirmed(socket_desc_udp, message, message_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id, timeout, retransmissions))
    {
        fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_FAILURE);
    }

    int bye_message_size = create_bye_message_udp(message_id++, message);
    sendto(socket_desc_udp, message, bye_message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (!is_confirmed(socket_desc_udp, message, bye_message_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id, timeout, retransmissions))
    {
        fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_FAILURE);
    }
    clean(socket_desc_udp, epollfd_udp);
    exit(EXIT_SUCCESS);
}

void hadle_server_response_udp(char *response, int timeout, int retransmissions, int message_id)
{

    char *first_elem = strtok(response, " ");
    enum response_type_t response_type;
    

    if (first_elem != NULL)
    {
        if (strcasecmp(first_elem, "CONFIRM") == 0)
        {
            response_type = CONFIRM;
        }
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
            response_type = ERR;
        }
    }


    if (response_type == CONFIRM)
    {
        return;
    }

    char responses[MAX_CHAR];

    // CURRENT
    uint16_t received_message_id = (uint8_t)response[1] << 8 | (uint8_t)response[2];
    char recent_message_array[1000] = {0};
    int id = 0;

    // IS DUPLICATED
    if (is_duplicated_message(recent_message_array, received_message_id, responses))
    {
        return;
    }

    // IS A NEW MESSAGE
    recent_message_array[id] = received_message_id;
    id++;
    int message_size = create_confirm_message_udp(received_message_id, responses);
    sendto(socket_desc_udp, responses, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    switch (response_type)
    {
    case REPLY:
        if ((strncmp(CURRENT_STATE, "/auth", 5) == 0) || (strncmp(CURRENT_STATE, "/join", 5) == 0))
        {
            event_udp.events = EPOLLIN;
            event_udp.data.fd = STDIN_FILENO;

            if (epoll_ctl(epollfd_udp, EPOLL_CTL_ADD, STDIN_FILENO, &event_udp) == -1)
            {
                fprintf(stderr, "ERR: Error while adding descriptor\n");
                clean(socket_desc_udp, epollfd_udp);
                exit(EXIT_FAILURE);
            }
        }

        uint8_t result = response[3];
        
        if (result == 0x00)
        {
            fprintf(stderr, "Failure: %s\n", response + 5);
            break;
        }
        else if (result == 0x01)
        {

            // REVIEW - response + 5
            fprintf(stderr, "Success: %s\n", response + 5);
            if (strncmp(CURRENT_STATE, "/auth", 5) == 0)
            {
                debug("User was authorized");
                AUTHENTIFIED = true;
            }
            break;
        }
        else
        {
            print_error("Unknown result", response, message_id, timeout, retransmissions);
        }
        break;

    case MSG:
        if (!AUTHENTIFIED)
        {
            fprintf(stderr, "ERR: You must be authentified first\n");
            return;
        }
        // {DisplayName}: {MessageContent}\n
        // response + 3 is between dn and mc
        printf("%s: %s\n", response + 2, response + 3 + strlen(response + 2));
        break;
    case ERR:
        // ERR: {MessageContent}\n
        // fprintf(stderr, "ERR: %s\n", response + 3);
        fprintf(stderr, "Failure: %s\n", response + 6);
        int message_size = create_bye_message_udp(message_id++, response);
        sendto(socket_desc_udp, response, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (!is_confirmed(socket_desc_udp, response, message_size, (struct sockaddr *)&server_addr, sizeof(server_addr), message_id - 1, timeout, retransmissions))
        {
            fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
            clean(socket_desc_udp, epollfd_udp);
            exit(EXIT_FAILURE);
        }
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_SUCCESS);
        break;
    case BYE:
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_SUCCESS);
        break;
    default:
        print_error("Unknown response", response, message_id, timeout, retransmissions);
    }
}

static void handle_signal()
{
    char message[MAX_CHAR]; // Messages that will be sent to the server

    if (strlen(CURRENT_STATE) == 0) // No command has been issued before
    {
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_SUCCESS);
    }

    int message_size = create_bye_message_udp(message_id++, message);
    sendto(socket_desc_udp, message, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    // REVIEW - 1000??????
    //   wait for confirmation from the server
    if (!is_confirmed(socket_desc_udp, message, message_size, (struct sockaddr *)&server_addr, sizeof(server_addr), 0, 1000, 3))
    {
        fprintf(stderr, "ERR: Error while waiting for the server's confirmation\n");
        clean(socket_desc_udp, epollfd_udp);
        exit(EXIT_FAILURE);
    }

    clean(socket_desc_udp, epollfd_udp);
    exit(EXIT_SUCCESS);
}

int udp_connect(char *server_ip, int port, int timeout, int retransmissions)
{
    signal(SIGINT, handle_signal); // interrupting signal

    debug("Openning TCP socket");
    // Creating a socket + setting the receive timeout
    socket_desc_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_desc_udp < 0)
    {
        fprintf(stderr, "Error: Error while creating a socket\n");
        clean(socket_desc_udp, epollfd_udp);
        return EXIT_FAILURE;
    }

    struct timeval time_value;
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
        debug("Waiting for events");
        // Waiting for events
        int nfds = epoll_wait(epollfd_udp, events, MAX_EVENTS, -1); // The number of events
        if (nfds < 0)
        {
            fprintf(stderr, "Error: Error while waiting for events\n");
            clean(socket_desc_udp, epollfd_udp);
            return EXIT_FAILURE;
        }

        char server_response[MAX_CHAR];

        debug("%d event(s) occured", nfds);

        // Handling the events
        for (int i = 0; i < nfds; ++i)
        {
            // Checks if the event is for the UDP socket
            // SOCKET
            if (events[i].data.fd == socket_desc_udp)
            {
                debug("Socket descriptor event");
                // There's incoming data to be read from the socket
                struct sockaddr_in sender_addr;
                socklen_t sender_addr_len = sizeof(sender_addr);

                // REVIEW - ssize_t?
                ssize_t recvfrom_check = recvfrom(socket_desc_udp, server_response, MAX_CHAR, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
                if (recvfrom_check < 0)
                {
                    fprintf(stderr, "Error: Error while receiving data from the server\n");
                    clean(socket_desc_udp, epollfd_udp);
                    return EXIT_FAILURE;
                }

                debug("Received response: %s", server_response);

                printf("%ld\n", recvfrom_check);
                for (int i = 0; i < recvfrom_check; i++) {
                    // Print the hexadecimal representation of each character
                    printf("%02X ", server_response[i]);
                }
                printf("\n");

                // Everything is OK!
                hadle_server_response_udp(server_response, timeout, retransmissions, message_id);
            }

            // STDIN
            else if (events[i].data.fd == STDIN_FILENO)
            {
                debug("User event (stdin)");
                // There's incoming data to be read from the standard input
                char input[MAX_CHAR];
                char message_content[MAX_CHAR];
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

                debug("User input: %s", input);

                if (input[0] == '/') // input is a command
                {
                    handle_input_command_udp(socket_desc_udp, input, message_id, DISPLAY_NAME, timeout, retransmissions);
                }

                else // input is a message
                {
                    // REVIEW - надо?
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

                    // create a message
                    int message_size = create_msg_message_udp(message_id++, input, DISPLAY_NAME, message_content);

                    debug("Created message: %s", message_content);
                    int sendto_check = sendto(socket_desc_udp, message_content, message_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                    if (sendto_check < 0)
                    {
                        fprintf(stderr, "Error: Error while sending data to the server\n");
                        clean(socket_desc_udp, epollfd_udp);
                        return EXIT_FAILURE;
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}