#include <assert.h>
#include <unistd.h>
#include <pthread.h>

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
			printf("\n");
		con_str_vec_foreach_del_nolock(&matches, (con_str_vec_foreach_cb)puts);
		pthread_mutex_unlock(&matches.lock);
		if (did_print)
			printf(">> ");
		fflush(stdout);
	}
}