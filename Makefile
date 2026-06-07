TARGET   = file-finder
SRCDIR   = src
INCDIR   = include
SRCS     = $(addprefix $(SRCDIR)/, main.c worker.c shell.c dumper.c)
OBJS     = $(SRCS:.c=.o)
DEPFILES = $(SRCS:.c=.d)
CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O3 -flto -pthread -MMD -MP -I$(INCDIR)

.PHONY: all clean debug install
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile — -MMD -MP writes a .d dependency file alongside each .o
$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Include generated header-dependency rules.
# Leading dash suppresses errors when .d files don't exist yet (first build).
-include $(DEPFILES)

# Debug build: no optimisation, full debug info, ThreadSanitizer.
# Separate output so it does not clobber the release binary.
$(TARGET).debug: $(SRCS)
	$(CC) -std=c11 -Wall -Wextra -O0 -g -pthread -fsanitize=thread -I$(INCDIR) -o $@ $^

debug: $(TARGET).debug

PREFIX ?= /usr/local
install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).debug $(OBJS) $(DEPFILES)
