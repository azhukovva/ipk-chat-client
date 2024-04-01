CC=gcc
CFLAGS=-Wall -Wextra -Werror -Werror=type-limits -Werror=overflow -pedantic #-std=c99 #-pedantic

run:
	$(CC) $(CFLAGS) $(shell find ./* -name '*.c') -o ipk24chat-client

clean:
	rm ipk24chat-client 

test:
	python3 tests/ipk-client-test-server/testo.py ./ipk24chat-client -d -p udp