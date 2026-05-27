
TARGET = file-finder
SRCS   = main.c worker.c shell.c dumper.c
CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -flto -pthread

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(TARGET)
