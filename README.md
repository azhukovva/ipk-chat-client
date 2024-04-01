## Client for a chat server using IPK24-CHAT protocol
### Introduction
The program (client) is designed to connect to any TCP and UDP server. It's written in C programming language and uses [socket API](https://www.geeksforgeeks.org/socket-programming-cc/) to create and manage network connections. The program can be compiled on both Linux and Windows platforms.

### Features
- Connects to any TCP or UDP server
- Sends and recieves data over network
- Handles primitive errors and exceptions, deals with invalid ports and hosts.

### Compilation
This project can be compiled using Makefile:
```bash
make
```
If you are using windows, you might want to compile like this:
```bash
make -f .\Makefile
```

The project is configured to be able to compile on Linux or Windows.

### Usage

You can run client using following command:
```bash
./ipk24 -t <protocol> -s <server> -p <port> -p <port> -d <timeout> -r <retransmissions> -h
```

`-t` <br>transport protocol used for connection (tcp or udp)<br/>
`-s` <br>server IP or hostname<br/>
`-p` <br>server port<br/>
`-d` <br>UDP confirmation timeout<br/>
`-r` <br>maximum number of UDP retransmissions<br/>
`-h <br>prints program help output and exits prints usage for the user<br/>

> **Note**<br/>
All the options described above are **required** in order to connect to the server.

Once the program started, if credentials you provided are valid, you can start typing to a server.


### Error Handling
The client program has handlers to validate internal network errors and input data, provided by a user. If error occurs, the program will throw error message to *STDERR*, cleaning data up.  Error message has format `[ERR]:<err-message>`. 

The program works like a [telnet](https://cs.wikipedia.org/wiki/Telnet). In udp mode, for the cases when port or ip is unreachable, there is `250ms` delay until timeout.

### Debugging
You can enable DEBUG mode, by uncommenting this line in `debug.h`
```c
//#define DEBUG
```

In this mode the program will give you detailed information of what it does at **each section of the code**. This was done in order to prevent commenting each line of the code due to project task.


### Testing
