#define _POSIX_C_SOURCE 200809

#include <errno.h>
#include <dirent.h>
#include <pthread.h>

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "worker.hpp"
#include "dumper.hpp"
#include "shell.hpp"
#include "con_str_vec.hpp"

struct con_str_vec matches;
char *root_directory = NULL;

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
	auto root = opendir(dir);
	if (root == NULL)
	{
		std::cerr << "Failed to open " << dir << " with: ";
		std::perror("");
		std::exit(EXIT_FAILURE);
	}
	closedir(root);

	root_directory = dir;
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "file-finder <dir> <substring1> [<substring2> [<substring3>]...]\n";
		std::exit(EXIT_FAILURE);
	}

	set_root_directory(argv[1]);

	auto substrings_len = argc - 2;
	auto substrings = &argv[2];

	auto rc = matches_init();
	if (rc != 0)
	{
		std::perror("calloc");
		std::exit(EXIT_FAILURE);
	}

	std::vector<pthread_t> workers{};
	workers.reserve(substrings_len);

	for (auto i = 0; i < substrings_len; i++)
	{
		auto rc = pthread_create(&workers[i], NULL, worker_main, (void *)substrings[i]);
		if (rc)
		{
			errno = rc;
			std::perror("pthread_create");
			for (auto worker : workers)
			{
				pthread_cancel(worker);
				pthread_join(worker, NULL);
			}

			matches_free();
			std::exit(EXIT_FAILURE);
		}
	}

	pthread_t dumper;
	auto quantum = 1;
	rc = pthread_create(&dumper, NULL, dumper_main, (void *)&quantum);
	if (rc)
	{
		errno = rc;
		std::perror("pthread_create");
		for (auto i = 0; i < substrings_len; i++)
		{
			pthread_cancel(workers[i]);
			pthread_join(workers[i], NULL);
		}

		matches_free();
		std::exit(EXIT_FAILURE);
	}

	pthread_t shell;
	rc = pthread_create(&shell, NULL, shell_main, NULL);
	if (rc)
	{
		errno = rc;
		std::perror("pthread_create");
		pthread_cancel(dumper);
		pthread_join(dumper, NULL);
		for (int i = 0; i < substrings_len; i++)
		{
			pthread_cancel(workers[i]);
			pthread_join(workers[i], NULL);
		}

		matches_free();
		std::exit(EXIT_FAILURE);
	}

	pthread_join(shell, NULL);

	pthread_cancel(dumper);
	pthread_join(dumper, NULL);
	for (int i = 0; i < substrings_len; i++)
	{
		pthread_cancel(workers[i]);
		pthread_join(workers[i], NULL);
	}

	matches_free();

	std::exit(EXIT_SUCCESS);
}