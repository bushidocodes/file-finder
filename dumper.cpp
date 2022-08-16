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
		pthread_mutex_lock(&matches.lock);
		bool did_print = matches.length > 0;
		if (did_print)
			std::cout << '\n';

		matches.foreach_del_nolock((con_str_vec_foreach_cb)puts);
		pthread_mutex_unlock(&matches.lock);
		if (did_print)
			std::cout << ">> ";
		std::cout.flush();
	}
}