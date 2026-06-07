/* POSIX.1-2024 */
#define _POSIX_C_SOURCE 202311L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"

static void
shell_dump(void)
{
	flockfile(stdout);
	con_str_vec_foreach_del(&matches, con_str_vec_puts);
	funlockfile(stdout);
}

static void
print_prompt(void)
{
	flockfile(stdout);
	printf(">> ");
	fflush(stdout);
	funlockfile(stdout);
}

void *
shell_main([[maybe_unused]] void *argument)
{
	print_prompt();

	char   *line = nullptr;
	size_t  len  = 0;
	ssize_t nread;
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

		print_prompt();

		free(line);
		line = nullptr;
	}

	free(line);
	return nullptr;
}
