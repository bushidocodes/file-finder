#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include <iostream>

#include "con_str_vec.hpp"

extern struct con_str_vec matches;

void *
dumper_main(void *argument)
{
	int quantum = *(int *)argument;
	assert(quantum > 0);

	while (true)
	{
		sleep(quantum);
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