#define _POSIX_C_SOURCE 200809

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>

#include "con_str_vec.h"

extern struct con_str_vec matches;

void
shell_dump()
{
	con_str_vec_foreach_del(&matches, (con_str_vec_foreach_cb)puts);
}

void *
shell_main(void *argument)
{
	printf(">> ");

	char   *line  = NULL;
	size_t  len   = 0;
	ssize_t nread = 0;
	while ((nread = getline(&line, &len, stdin)) != -1) {
		if (nread >= 1 && line[nread - 1] == '\n') line[nread - 1] = '\0';

		// Sort of silly to check for CRLF since I use POSIX APIs
		if (nread >= 2 && line[nread - 2] == '\r') line[nread - 2] = '\0';

		if (strcmp(line, "dump") == 0) {
			shell_dump();
		} else if (strcmp(line, "exit") == 0) {
			break;
		} else {
			printf("Unknown Command: '%s'\n", line);
			printf("Valid Commands: dump, exit\n");
		}

		printf(">> ");
		fflush(stdout);

		if (line != NULL) {
			free(line);
			line = NULL;
		}
	}

	if (line != NULL) {
		free(line);
		line = NULL;
	}

	pthread_exit(NULL);
}