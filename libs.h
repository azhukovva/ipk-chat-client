#ifndef LIBS_H
#define LIBS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <netdb.h>
#include <netinet/in.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <strings.h>

#include "debug.h"
#include "common.h"

#define MAX_ID 20
#define MAX_SECRET 128
#define MAX_CONTENT 1400
#define MAX_USERNAME 20
// Default Ethernet MTU(Maximum Transmission Unit) == 1500 bytes
#define MAX_CHAR 1500
#define MAX_EVENTS 1

#endif