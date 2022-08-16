#define _POSIX_C_SOURCE 200809

#include <pthread.h>

#include <string>
#include <iostream>

#include "con_str_vec.hpp"

extern struct con_str_vec matches;

void *
shell_main(void *argument)
{
	std::cout << ">> ";
	std::string line;
	while ((std::cin >> line))
	{
		if (line.compare("dump") == 0)
		{
			matches.foreach_del([](char *arg) -> void
								{ std::cout << arg << "\n"; });
		}
		else if (line.compare("exit") == 0)
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
	}

	pthread_exit(nullptr);
}