#include <algorithm>
#include <thread>
#include <iostream>

#include "matches.hpp"

void dumper_main(void)
{
	while (true)
	{
		using namespace std::chrono_literals;

		/* Sleep for two minutes */
		std::this_thread::sleep_for(2000ms);

		/* Try to dump all results to stdout */
		std::string res = matches.dump();
		if (res.length() > 0)
			std::cout << '\n'
					  << res << ">> " << std::flush;
	}
}