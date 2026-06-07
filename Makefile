TARGET = file-finder
SRCS   = main.c worker.c shell.c dumper.c
CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -O3 -flto -pthread

.PHONY: all clean debug
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Debug build: no optimisation, full debug info, ThreadSanitizer.
# Output is file-finder.debug so it does not clobber the release binary.
$(TARGET).debug: $(SRCS)
	$(CC) -std=c11 -Wall -Wextra -O0 -g -pthread -fsanitize=thread -o $@ $^

debug: $(TARGET).debug

clean:
	rm -f $(TARGET) $(TARGET).debug
