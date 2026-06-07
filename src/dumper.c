/* POSIX.1-2024 */
#define _POSIX_C_SOURCE 202311L

/* bool / true / false are C23 keywords — no <stdbool.h> needed. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "globals.h"

static void funlockfile_cleanup(void *arg)  { funlockfile((FILE *)arg); }
static void mutex_unlock_cleanup(void *arg) { pthread_mutex_unlock((pthread_mutex_t *)arg); }

void *
dumper_main(void *argument)
{
	int quantum = *(int *)argument;
	assert(quantum > 0);

	while (true) {
		sleep(quantum);

		flockfile(stdout);
		pthread_cleanup_push(funlockfile_cleanup, stdout);

		bool did_print; /* outer scope survives the inner pthread_cleanup_pop */
		pthread_mutex_lock(&matches.lock);
		pthread_cleanup_push(mutex_unlock_cleanup, &matches.lock);
		did_print = matches.length > 0;
		if (did_print) printf("\n");
		con_str_vec_foreach_del_nolock(&matches, con_str_vec_puts);
		pthread_cleanup_pop(1); /* pthread_mutex_unlock(&matches.lock) */

		if (did_print) printf(">> ");
		fflush(stdout);
		pthread_cleanup_pop(1); /* funlockfile(stdout) */
	}

	unreachable(); /* thread exits only via pthread_cancel from main */
}
