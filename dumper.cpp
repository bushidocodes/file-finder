#include <thread>
#include <iostream>

#include "con_str_vec.hpp"

extern struct con_str_vec matches;

void dumper_main(void)
{
	while (true)
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2000ms);
		bool not_empty = matches.getLength() > 0;
		if (not_empty)
			std::cout << '\n';

		matches.foreach_del([](char *val)
							{ std::cout << val << '\n'; });

		if (not_empty)
			std::cout << ">> ";
		std::cout.flush();
	}
}