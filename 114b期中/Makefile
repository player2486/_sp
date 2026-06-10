CC = gcc
CFLAGS = -Wall -Wextra

SRC = src
TARGET = hw7-top

$(TARGET): $(SRC)/main.c $(SRC)/procfs.c $(SRC)/display.c
	$(CC) $(CFLAGS) -o $(TARGET) $^

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
