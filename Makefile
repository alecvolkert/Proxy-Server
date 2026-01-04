CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
TARGET = server client

all: $(TARGET)

server: server.c common.h
	$(CC) $(CFLAGS) server.c -o server

client: client.c common.h
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client *.o