#pragma once

#include <stddef.h>
#include <stdint.h>

struct worker_args {
	const char        *root_dir;
	const char *const *substrings; /* argv slice — elements are never modified */
	size_t             count;
};

/*
 * Sentinel returned (via pthread_exit / checked via pthread_join) to signal
 * that the worker thread encountered a fatal error (OOM, etc.).
 * Using -1 cast through intptr_t is the conventional non-NULL, non-zero
 * pthread exit value that cannot be confused with a valid pointer.
 */
#define WORKER_FAILURE ((void *)(intptr_t)-1)

void *worker_main(void *argument);
