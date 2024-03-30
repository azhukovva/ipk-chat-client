CC=gcc
CFLAGS=#-Wall -Wextra -Werror -std=c99

run:
	$(CC) $(CFLAGS) $(shell find ./* -name '*.c') -o ipk24 

clean:
	rm ipk24 

test:
	python3 tests/ipk-client-test-server/testo.py ./ipk24 -d