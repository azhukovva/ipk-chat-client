#include "tcp.h"

#define OPT_T "-t"
#define OPT_S "-s"
#define OPT_P "-p"
#define OPT_D "-d"
#define OPT_R "-r"
#define OPT_H "-h"

#define UDP_LITERAL "udp"
#define TCP_LITERAL "tcp"


void print_help_main()
{
	printf("Usage: ipk24chat-client -t protocol -s server [-p port] [-d timeout] [-r retransmissions] [-h]\n");
	printf("Options:\n");
	printf("  -t protocol\t\tSpecify protocol (TCP/UDP)\n");
	printf("  -s server\t\tSpecify server to connect to\n");
	printf("  -p port\t\tSpecify port to connect to (default 4567)\n");
	printf("  -d timeout\t\tSet timeout for retransmissions in ms(default 250)\n");
	printf("  -r retransmissions\tSet number of retransmissions(default 3)\n");
	printf("  -h\t\tDisplay this help message\n");
}

// 127.0.0.1  IPv4 address

enum protocol_t
{
	UDP,
	TCP
};

struct args_t
{
	enum protocol_t protocol;
	char *server;
	uint16_t port;
	uint16_t timeout;
	uint8_t retransmissions;
	int help;
};

//REVIEW - 
struct args_t args = {
	.port = 4567,
	.timeout = 250, // *1000
	.retransmissions = 3
};

int main(int argc, char *argv[])
{ // prog.c -t udp -s google.com -p 3000

	debug("Reading program arguments...");
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], OPT_T) == 0)
		{
			if (i + 1 >= argc)
			{
				return EXIT_FAILURE; // is not value after flag
			}
			if (strcmp(argv[i + 1], UDP_LITERAL) == 0)
				args.protocol = UDP;
			else if (strcmp(argv[i + 1], TCP_LITERAL) == 0)
				args.protocol = TCP;
			else
				return EXIT_FAILURE; // not udp, not tcp ERROR
			i++;
		}
		else if (strcmp(argv[i], OPT_S) == 0)
		{
			if (i + 1 >= argc)
			{
				return EXIT_FAILURE; // is not value after flag
			}
			args.server = malloc(strlen(argv[i + 1])); // char = 1 -> size = number of chars
			strcpy(args.server, argv[i + 1]);
			// error
			i++;
		}
		else if (strcmp(argv[i], OPT_P) == 0)
		{
			if (i + 1 >= argc)
			{
				return EXIT_FAILURE; // is not value after flag
			}
			args.port = atoi(argv[i + 1]);
			// error, not num will be -1 from atoi
			i++;
		}
		else if (strcmp(argv[i], OPT_D) == 0)
		{
			if (i + 1 >= argc)
			{
				return EXIT_FAILURE; // is not value after flag
			}
			args.timeout = atoi(argv[i + 1]);
			// error
			i++;
		}
		else if (strcmp(argv[i], OPT_R) == 0)
		{
			if (i + 1 >= argc)
			{
				return EXIT_FAILURE; // is not value after flag
			}
			args.retransmissions = atoi(argv[i + 1]);
			// error
			i++;
		}
		else if (strcmp(argv[i], OPT_H) == 0)
		{
			args.help = 1;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}

	debug("-t: %d", args.protocol);
	debug("-s: %s", args.server);
	debug("-p: %d", args.port);
	debug("-d: %d", args.timeout);
	debug("-r: %d", args.retransmissions);

	debug("Querying Ipv4 address for server \'%s\'", args.server);

	struct addrinfo hints, *res; // *res - packet
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	char server_ip[INET_ADDRSTRLEN]; // max IP address's length

	int err_getaddrinfo = getaddrinfo(args.server, NULL, &hints, &res);
	if (err_getaddrinfo != 0)
	{
		fprintf(stderr, "ERR: getaddrinfo: %s\n", gai_strerror(err_getaddrinfo));
		return EXIT_FAILURE;
	}

	struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
	void *addr = &(ipv4->sin_addr);
	inet_ntop(res->ai_family, addr, server_ip, sizeof server_ip); // IP to send inf to

	freeaddrinfo(res);

	debug("Ipv4 address found: %s", server_ip);

	if (args.protocol == UDP)
	{
		// volani udp
	}
	else if (args.protocol == TCP)
	{
		tcp_connect(server_ip, args.port);
	}
}
