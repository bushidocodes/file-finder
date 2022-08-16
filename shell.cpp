#define _POSIX_C_SOURCE 200809

#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <pthread.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "con_str_vec.hpp"

extern struct con_str_vec matches;

void shell_dump()
{
	matches.foreach_del([](char *arg) -> void
						{ std::cout << arg << "\n"; });
}

void *
shell_main(void *argument)
{
	std::cout << ">> ";

	char *line = nullptr;
	std::size_t len = 0;
	ssize_t nread = 0;
	while ((nread = getline(&line, &len, stdin)) != -1)
	{
		if (nread >= 1 && line[nread - 1] == '\n')
			line[nread - 1] = '\0';

		// Sort of silly to check for CRLF since I use POSIX APIs
		if (nread >= 2 && line[nread - 2] == '\r')
			line[nread - 2] = '\0';

		if (strcmp(line, "dump") == 0)
		{
			shell_dump();
		}
		else if (strcmp(line, "exit") == 0)
		{
			break;
		}
		else
		{
			std::cout << "Unknown Command: " << line << "\n";
			std::cout << "Valid Commands: dump, exit\n";
		}

		std::cout << ">> ";
		std::cout.flush();

		if (line != nullptr)
		{
			free(line);
			line = nullptr;
		}
	}

	if (line != nullptr)
	{
		free(line);
		line = nullptr;
	}

	pthread_exit(nullptr);
}