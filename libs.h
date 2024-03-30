#ifndef LIBS_H
#define LIBS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <netdb.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MAX_ID 20
#define MAX_SECRET 128
#define MAX_CONTENT 1400
#define MAX_DNAME 20
#define MAX_USERNAME 20
// Default Ethernet MTU(Maximum Transmission Unit) == 1500 bytes
#define MAX_CHAR 1500
#define MAX_EVENTS 1

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

#endif