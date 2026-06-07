TARGET   = file-finder
SRCDIR   = src
INCDIR   = include
TESTDIR  = tests
UNITY    = $(TESTDIR)/unity/unity.c
SRCS     = $(addprefix $(SRCDIR)/, main.c worker.c shell.c dumper.c)
OBJS     = $(SRCS:.c=.o)
DEPFILES = $(SRCS:.c=.d)
CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -O3 -flto -pthread -MMD -MP -I$(INCDIR)
TESTCFLAGS = -std=c11 -Wall -Wextra -O0 -g -pthread -I$(INCDIR) -I$(TESTDIR)/unity

.PHONY: all clean debug install test

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

# Unit tests
# test_con_str_vec: header-only con_str_vec — no production .c files needed
$(TESTDIR)/test_con_str_vec: $(TESTDIR)/test_con_str_vec.c $(UNITY)
	$(CC) $(TESTCFLAGS) -o $@ $^

# test_worker: worker.c is #include'd directly inside the test file;
# main.c / shell.c / dumper.c are NOT linked.
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
