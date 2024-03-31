#ifndef UDP_H
#define UDP_H

#include "libs.h"

int udp_connect(char *server_ip, int port, int timeout, int retransmissions);

#endif

void NewFunction(int index, char * message);
