# C23 requires GCC 14+.  Override on the command line if needed:
#   make CC=gcc
CC       = gcc-14

TARGET   = file-finder
SRCDIR   = src
INCDIR   = include
TESTDIR  = tests
UNITY    = $(TESTDIR)/unity/unity.c
SRCS     = $(addprefix $(SRCDIR)/, main.c worker.c shell.c dumper.c)
OBJS     = $(SRCS:.c=.o)
DEPFILES = $(SRCS:.c=.d)
CFLAGS   = -std=c23 -Wall -Wextra -O3 -flto -pthread -MMD -MP -I$(INCDIR)
TESTCFLAGS = -std=c23 -Wall -Wextra -O0 -g -pthread -I$(INCDIR) -I$(TESTDIR)/unity

.PHONY: all clean debug install test

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile — -MMD -MP writes a .d dependency file alongside each .o
$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Include generated header-dependency rules.
-include $(DEPFILES)

# Debug build: no optimisation, full debug info, ThreadSanitizer.
$(TARGET).debug: $(SRCS)
	$(CC) -std=c23 -Wall -Wextra -O0 -g -pthread -fsanitize=thread \
	      -I$(INCDIR) -o $@ $^

debug: $(TARGET).debug

PREFIX ?= /usr/local
install: $(TARGET)
	install -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

# Unit tests
$(TESTDIR)/test_con_str_vec: $(TESTDIR)/test_con_str_vec.c $(UNITY)
	$(CC) $(TESTCFLAGS) -o $@ $^

$(TESTDIR)/test_worker: $(TESTDIR)/test_worker.c $(UNITY)
	$(CC) $(TESTCFLAGS) -o $@ $^

test: $(TESTDIR)/test_con_str_vec $(TESTDIR)/test_worker
	@echo "--- test_con_str_vec ---"
	@$(TESTDIR)/test_con_str_vec
	@echo "--- test_worker ---"
	@$(TESTDIR)/test_worker

clean:
	rm -f $(TARGET) $(TARGET).debug $(OBJS) $(DEPFILES) \
	      $(TESTDIR)/test_con_str_vec $(TESTDIR)/test_worker
