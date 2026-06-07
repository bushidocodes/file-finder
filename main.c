#define _POSIX_C_SOURCE 200809

#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "worker.h"
#include "dumper.h"
#include "shell.h"
#include "con_str_vec.h"

struct con_str_vec matches;

#define DUMPER_QUANTUM_SECS 1

static void
validate_root_directory(const char *dir)
{
	DIR *root = opendir(dir);
	if (root == NULL) {
		perror(dir);
		exit(EXIT_FAILURE);
	}
	closedir(root);
}

int
main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr, "Usage: file-finder <dir> <substring> [<substring>...]\n");
		exit(EXIT_FAILURE);
	}

	validate_root_directory(argv[1]);

	struct worker_args wargs = {
		.root_dir   = argv[1],
		.substrings = &argv[2],
		.count      = (size_t)(argc - 2),
	};

	if (con_str_vec_init(&matches, 0) != 0) {
		perror("con_str_vec_init");
		exit(EXIT_FAILURE);
	}

	pthread_t worker;
	int rc = pthread_create(&worker, NULL, worker_main, &wargs);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		con_str_vec_destroy(&matches);
		exit(EXIT_FAILURE);
	}

	pthread_t dumper;
	int quantum = DUMPER_QUANTUM_SECS;
	rc = pthread_create(&dumper, NULL, dumper_main, &quantum);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		pthread_cancel(worker);
		pthread_join(worker, NULL);
		con_str_vec_destroy(&matches);
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
		con_str_vec_destroy(&matches);
		exit(EXIT_FAILURE);
	}

	pthread_join(shell, NULL);

	pthread_cancel(dumper);
	pthread_join(dumper, NULL);

	pthread_cancel(worker);
	void *worker_retval = NULL;
	pthread_join(worker, &worker_retval);

	con_str_vec_destroy(&matches);

	/* worker signals failure by exiting with (void*)(intptr_t)-1 */
	exit(worker_retval == (void *)(intptr_t)-1 ? EXIT_FAILURE : EXIT_SUCCESS);
}
