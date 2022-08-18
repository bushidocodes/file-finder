#include <iostream>
#include <string>

#include "matches.hpp"

void shell_main(void)
{
	std::cout << ">> ";
	std::string line;
	while ((std::cin >> line))
	{
		if (line.compare("dump") == 0)
		{
			std::cout << matches.dump() << ">> ";
		}
		else if (line.compare("exit") == 0)
		{
			break;
		}
		else
		{
			std::cout << "Unknown Command: " << line << "\nValid Commands: dump, exit\n>> ";
		}

		std::cout.flush();
	}
}