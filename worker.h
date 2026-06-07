#pragma once

#include <stddef.h>

struct worker_args {
	char   **substrings;
	size_t   count;
};

void *worker_main(void *argument);
