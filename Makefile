CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -pthread -Isrc
LDFLAGS = -pthread -lm

SRCS = src/main.c src/sched/sched.c
OBJS = $(SRCS:.c=.o)
TARGET = minios


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)