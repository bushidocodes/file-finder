#pragma once

#include <stddef.h>

struct worker_args {
	const char  *root_dir;
	char       **substrings;
	size_t       count;
};

void *worker_main(void *argument);
