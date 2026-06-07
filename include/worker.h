#pragma once

#include <stddef.h>

struct worker_args {
	const char        *root_dir;
	const char *const *substrings; /* argv slice — elements are never modified */
	size_t             count;
};

void *worker_main(void *argument);
