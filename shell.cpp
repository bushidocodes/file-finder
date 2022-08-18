#include <string>
#include <thread>
#include <iostream>

#include "con_str_vec.hpp"

extern struct con_str_vec matches;

void shell_main(void)
{
	std::cout << ">> ";
	std::string line;
	while ((std::cin >> line))
	{
		if (line.compare("dump") == 0)
		{
			matches.foreach_del([](std::string &arg) -> void
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
}