# CC=gcc
# CFLAGS=-I.

# OBJ = main.o tcp.o udp.o

# %.o: %.c $(DEPS)
# 	$(CC) -c -o $@ $< $(CFLAGS)

# ipk24: $(OBJ)
# 	$(CC) -o $@ $^ $(CFLAGS)


CC=gcc
CFLAGS=#-Wall -Wextra -Werror -std=c99

run:
	$(CC) $(CFLAGS) $(shell find ./* -name '*.c') -o ipk24 

clean:
	rm ipk24 