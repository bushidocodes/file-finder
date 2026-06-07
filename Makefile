TARGET   = file-finder
SRCS     = main.c worker.c shell.c dumper.c
OBJS     = $(SRCS:.c=.o)
DEPFILES = $(SRCS:.c=.d)
CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O3 -flto -pthread -MMD -MP

.PHONY: all clean debug
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile — -MMD -MP writes a .d dependency file alongside each .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Include generated header-dependency rules.
# Leading dash suppresses errors when .d files don't exist yet (first build).
-include $(DEPFILES)

# Debug build: no optimisation, full debug info, ThreadSanitizer.
# Separate output so it does not clobber the release binary.
$(TARGET).debug: $(SRCS)
	$(CC) -std=c11 -Wall -Wextra -O0 -g -pthread -fsanitize=thread -o $@ $^

debug: $(TARGET).debug

clean:
	rm -f $(TARGET) $(TARGET).debug $(OBJS) $(DEPFILES) $(TARGET).d
