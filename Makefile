CC=gcc
CFLAGS=#-Wall -Wextra -Werror -std=c99 #-pedantic

run:
	$(CC) $(CFLAGS) $(shell find ./* -name '*.c') -o ipk24chat-client 

clean:
	rm ipk24chat-client 

test:
	python3 tests/ipk-client-test-server/testo.py ./ipk24chat-client -d