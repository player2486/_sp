CC = gcc
CFLAGS = -Wall -Wextra

SRCS = $(wildcard *.c)
PROGS = $(SRCS:.c=)

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS)

.PHONY: all clean
