#define _POSIX_C_SOURCE 200809
#define _DEFAULT_SOURCE

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "con_str_vec.h"

extern char              *root_directory;
extern struct con_str_vec matches;

static inline void
search_filenames(char *dir_path, char *substring)
{
	DIR *dir = opendir(dir_path);

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
		// Skip links, and . and ..
		if (entry->d_type == DT_LNK || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		if (entry->d_type == DT_DIR) {
			// dirent has a static buffer of 256, so doubling to prevent clang nits
			char joined_path[513] = { 0 };
			snprintf(joined_path, 512, "%s/%s", dir_path, entry->d_name);
			search_filenames(joined_path, substring);
		}

		if (strstr(entry->d_name, substring) != NULL) {
			char *copy = strdup(entry->d_name);
			if (copy == NULL) {
				perror("strdup");
				exit(EXIT_FAILURE);
			}

			int rc = con_str_vec_push(&matches, copy);
			if (rc != 0) {
				perror("realloc");
				break;
			}
		}
	}

	closedir(dir);
}

void *
worker_main(void *argument)
{
	char *substring = (char *)argument;

	assert(root_directory != NULL);

	search_filenames(root_directory, substring);

	pthread_exit(NULL);
}