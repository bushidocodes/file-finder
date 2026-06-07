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

	size_t substrings_len = argc - 2;
	char **substrings     = &argv[2];

	int rc = matches_init();
	if (rc != 0){
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	pthread_t workers[substrings_len];

	for (size_t i = 0; i < substrings_len; i++) {
		int rc = pthread_create(&workers[i], NULL, worker_main, (void *)substrings[i]);
		if (rc) {
			errno = rc;
			perror("pthread_create");
			for (size_t j = 0; j < i; j++) {
				pthread_cancel(workers[j]);
				pthread_join(workers[j], NULL);
			}

			matches_free();
			exit(EXIT_FAILURE);
		}
	}

	pthread_t dumper;
	int       quantum = 1;
	rc      = pthread_create(&dumper, NULL, dumper_main, (void *)&quantum);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		for (size_t i = 0; i < substrings_len; i++) {
			pthread_cancel(workers[i]);
			pthread_join(workers[i], NULL);
		}

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
		for (size_t i = 0; i < substrings_len; i++) {
			pthread_cancel(workers[i]);
			pthread_join(workers[i], NULL);
		}

		matches_free();
		exit(EXIT_FAILURE);
	}

	pthread_join(shell, NULL);

	pthread_cancel(dumper);
	pthread_join(dumper, NULL);

	bool worker_failed = false;
	for (size_t i = 0; i < substrings_len; i++) {
		pthread_cancel(workers[i]);
		void *retval = NULL;
		pthread_join(workers[i], &retval);
		if (retval == (void *)(intptr_t)-1) worker_failed = true;
	}

	matches_free();

	exit(worker_failed ? EXIT_FAILURE : EXIT_SUCCESS);
}