#define _POSIX_C_SOURCE 200809
#define _DEFAULT_SOURCE

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>

#include "globals.h"
#include "worker.h"

static void closedir_cleanup(void *dir) { closedir((DIR *)dir); }

static void
search_filenames(const char *dir_path, char **substrings, size_t count)
{
	DIR *dir = opendir(dir_path);
	if (dir == NULL) return;
	pthread_cleanup_push(closedir_cleanup, dir);

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
		/* skip . and .. */
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		/* resolve DT_UNKNOWN via fstatat before any type checks */
		unsigned char d_type = entry->d_type;
		if (d_type == DT_UNKNOWN) {
			struct stat st;
			if (fstatat(dirfd(dir), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
				if      (S_ISDIR(st.st_mode)) d_type = DT_DIR;
				else if (S_ISLNK(st.st_mode)) d_type = DT_LNK;
				else                           d_type = DT_REG;
			}
		}

		/* skip symlinks */
		if (d_type == DT_LNK) continue;

		if (d_type == DT_DIR) {
			char *joined_path = NULL;
			if (asprintf(&joined_path, "%s/%s", dir_path, entry->d_name) < 0) {
				perror("asprintf");
				pthread_exit((void *)(intptr_t)-1);
			}
			pthread_cleanup_push(free, joined_path);
			search_filenames(joined_path, substrings, count);
			pthread_cleanup_pop(1); /* free(joined_path) */
		} else {
			for (size_t i = 0; i < count; i++) {
				if (strstr(entry->d_name, substrings[i]) == NULL) continue;

				char *copy = NULL;
				if (asprintf(&copy, "%s/%s", dir_path, entry->d_name) < 0) {
					perror("asprintf");
					pthread_exit((void *)(intptr_t)-1);
				}

				if (con_str_vec_push(&matches, copy) != 0) {
					perror("con_str_vec_push");
					free(copy);
					pthread_exit((void *)(intptr_t)-1);
				}

				break; /* report each file at most once */
			}
		}
	}

	pthread_cleanup_pop(1); /* closedir(dir) */
}

void *
worker_main(void *argument)
{
	struct worker_args *args = (struct worker_args *)argument;

	assert(args->root_dir != NULL);
	assert(args->count > 0);

	search_filenames(args->root_dir, args->substrings, args->count);

	return NULL;
}
