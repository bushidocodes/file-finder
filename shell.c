#define _POSIX_C_SOURCE 200809

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "con_str_vec.h"

extern struct con_str_vec matches;

static void
shell_dump()
{
	flockfile(stdout);
	con_str_vec_foreach_del(&matches, con_str_vec_puts);
	funlockfile(stdout);
}

void *
shell_main(void *argument)
{
	(void)argument;
	flockfile(stdout);
	printf(">> ");
	fflush(stdout);
	funlockfile(stdout);

	char   *line  = NULL;
	size_t  len   = 0;
	ssize_t nread = 0;
	while ((nread = getline(&line, &len, stdin)) != -1) {
		/* strip trailing newline */
		if (nread >= 1 && line[nread - 1] == '\n') line[nread - 1] = '\0';

		if (strcmp(line, "dump") == 0) {
			shell_dump();
		} else if (strcmp(line, "exit") == 0) {
			break;
		} else {
			flockfile(stdout);
			printf("Unknown command: '%s'\n", line);
			printf("Valid commands: dump, exit\n");
			funlockfile(stdout);
		}

		flockfile(stdout);
		printf(">> ");
		fflush(stdout);
		funlockfile(stdout);

		free(line);
		line = NULL;
	}

	free(line);

	pthread_exit(NULL);
	return NULL;
}
