CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -pthread -Isrc
LDFLAGS = -pthread -lm

all: minios

minios: src/main.c
	$(CC) $(CFLAGS) -o minios src/main.c $(LDFLAGS)

clean:
	rm -f minios