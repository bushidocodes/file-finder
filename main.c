#define _POSIX_C_SOURCE 200809

#include <errno.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "worker.h"
#include "dumper.h"
#include "shell.h"
#include "con_str_vec.h"

struct con_str_vec matches;
char              *root_directory = NULL;

static inline int
matches_init()
{
	return con_str_vec_init(&matches, 0);
}

static inline void
matches_free()
{
	con_str_vec_destroy(&matches);
}

static void
set_root_directory(char *dir)
{
	// Sanity check path before spinning up worker threads
	DIR *root = opendir(dir);
	if (root == NULL) {
		fprintf(stderr, "Failed to open %s with: ", dir);
		perror("");
		exit(EXIT_FAILURE);
	}
	closedir(root);

	root_directory = dir;
}

int
main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "file-finder <dir> <substring1>[<substring2> [<substring3>]...]\n");
		exit(EXIT_FAILURE);
	}

	set_root_directory(argv[1]);

	struct worker_args wargs = {
		.substrings = &argv[2],
		.count      = (size_t)(argc - 2),
	};

	int rc = matches_init();
	if (rc != 0){
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	pthread_t worker;
	rc = pthread_create(&worker, NULL, worker_main, &wargs);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		matches_free();
		exit(EXIT_FAILURE);
	}

	pthread_t dumper;
	int       quantum = 1;
	rc = pthread_create(&dumper, NULL, dumper_main, &quantum);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		pthread_cancel(worker);
		pthread_join(worker, NULL);
		matches_free();
		exit(EXIT_FAILURE);
	}

	pthread_t shell;
	rc = pthread_create(&shell, NULL, shell_main, NULL);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		pthread_cancel(dumper);
		pthread_join(dumper, NULL);
		pthread_cancel(worker);
		pthread_join(worker, NULL);
		matches_free();
		exit(EXIT_FAILURE);
	}

	pthread_join(shell, NULL);

	pthread_cancel(dumper);
	pthread_join(dumper, NULL);

	pthread_cancel(worker);
	void *worker_retval = NULL;
	pthread_join(worker, &worker_retval);

	matches_free();

	exit(worker_retval == (void *)(intptr_t)-1 ? EXIT_FAILURE : EXIT_SUCCESS);
}
