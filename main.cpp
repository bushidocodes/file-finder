#define _POSIX_C_SOURCE 200809

#include <errno.h>
#include <pthread.h>

#include <filesystem>
#include <iostream>
#include <vector>

#include "worker.hpp"
#include "dumper.hpp"
#include "shell.hpp"
#include "con_str_vec.hpp"

struct con_str_vec matches
{
	0
};

std::filesystem::path root_directory;

static void
set_root_directory(char *dir)
{
	root_directory = dir;

	if (!std::filesystem::exists(root_directory))
	{
		std::cerr << "Failed to open " << dir << " with: ";
		std::perror("");
		std::exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "file-finder <dir> <substring1> [<substring2> [<substring3>]...]\n";
		std::exit(EXIT_FAILURE);
	}

	set_root_directory(argv[1]);

	std::vector<std::string> substrings{};
	substrings.reserve(argc - 2);
	for (auto i = 2; i < argc; i++)
	{
		substrings.push_back(std::move(argv[i]));
	}

	std::vector<pthread_t> workers{};
	workers.reserve(substrings.size());

	for (auto i = 0; i < substrings.size(); i++)
	{
		auto rc = pthread_create(&workers[i], nullptr, worker_main, (void *)substrings[i].c_str());
		if (rc)
		{
			errno = rc;
			std::perror("pthread_create");
			for (auto worker : workers)
			{
				pthread_cancel(worker);
				pthread_join(worker, nullptr);
			}

			std::exit(EXIT_FAILURE);
		}
	}

	pthread_t dumper;
	auto quantum = 1;
	auto rc = pthread_create(&dumper, nullptr, dumper_main, (void *)&quantum);
	if (rc)
	{
		errno = rc;
		std::perror("pthread_create");
		for (auto worker : workers)
		{
			pthread_cancel(worker);
			pthread_join(worker, nullptr);
		}

		std::exit(EXIT_FAILURE);
	}

	pthread_t shell;
	rc = pthread_create(&shell, nullptr, shell_main, nullptr);
	if (rc)
	{
		errno = rc;
		std::perror("pthread_create");
		pthread_cancel(dumper);
		pthread_join(dumper, nullptr);
		for (auto worker : workers)
		{
			pthread_cancel(worker);
			pthread_join(worker, nullptr);
		}

		std::exit(EXIT_FAILURE);
	}

	pthread_join(shell, nullptr);

	pthread_cancel(dumper);
	pthread_join(dumper, nullptr);
	for (auto worker : workers)
	{
		pthread_cancel(worker);
		pthread_join(worker, nullptr);
	}

	std::exit(EXIT_SUCCESS);
}