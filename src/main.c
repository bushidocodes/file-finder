/* POSIX.1-2024 (IEEE Std 1003.1-2024).  glibc 2.40+ honours this value;
   earlier versions fall back to POSIX 2008 behaviour. */
#define _POSIX_C_SOURCE 202311L

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "worker.h"
#include "dumper.h"
#include "shell.h"
#include "con_str_vec.h"

struct con_str_vec matches;

/* How often (seconds) the dumper thread flushes pending matches. */
constexpr int DUMPER_QUANTUM_SECS = 1;

static void
validate_root_directory(const char *dir)
{
	DIR *root = opendir(dir);
	if (root == nullptr) {
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
		.substrings = (const char *const *)&argv[2],
		.count      = (size_t)(argc - 2),
	};

	if (con_str_vec_init(&matches, 0) != 0) {
		perror("con_str_vec_init");
		exit(EXIT_FAILURE);
	}

	pthread_t worker;
	int rc = pthread_create(&worker, nullptr, worker_main, &wargs);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		con_str_vec_destroy(&matches);
		exit(EXIT_FAILURE);
	}

	pthread_t dumper;
	int quantum = DUMPER_QUANTUM_SECS;
	rc = pthread_create(&dumper, nullptr, dumper_main, &quantum);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		pthread_cancel(worker);
		pthread_join(worker, nullptr);
		con_str_vec_destroy(&matches);
		exit(EXIT_FAILURE);
	}

	pthread_t shell;
	rc = pthread_create(&shell, nullptr, shell_main, nullptr);
	if (rc) {
		errno = rc;
		perror("pthread_create");
		pthread_cancel(dumper);
		pthread_join(dumper, nullptr);
		pthread_cancel(worker);
		pthread_join(worker, nullptr);
		con_str_vec_destroy(&matches);
		exit(EXIT_FAILURE);
	}

	pthread_join(shell, nullptr);

	pthread_cancel(dumper);
	pthread_join(dumper, nullptr);

	pthread_cancel(worker);
	void *worker_retval = nullptr;
	pthread_join(worker, &worker_retval);

	con_str_vec_destroy(&matches);

	return worker_retval == WORKER_FAILURE ? EXIT_FAILURE : EXIT_SUCCESS;
}
