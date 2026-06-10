CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = msh

all: $(TARGET)

$(TARGET): src/msh.c
	$(CC) $(CFLAGS) -o $(TARGET) src/msh.c

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
