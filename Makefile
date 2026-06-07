TARGET = file-finder
SRCS   = main.c worker.c shell.c dumper.c
CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -flto -pthread

.PHONY: all clean debug
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Debug build: no optimizations, full debug info, ThreadSanitizer
debug: CFLAGS = -std=c11 -Wall -Wextra -O0 -g -pthread -fsanitize=thread
debug: $(TARGET)

clean:
	rm -f $(TARGET)
