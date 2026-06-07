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
#include <sys/types.h>

#include "con_str_vec.h"
#include "worker.h"

extern char              *root_directory;
extern struct con_str_vec matches;

static void closedir_cleanup(void *dir) { closedir((DIR *)dir); }

static inline void
search_filenames(char *dir_path, char **substrings, size_t count)
{
	DIR *dir = opendir(dir_path);
	if (dir == NULL) return;
	pthread_cleanup_push(closedir_cleanup, dir);

	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
		// Skip links, and . and ..
		if (entry->d_type == DT_LNK || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		if (entry->d_type == DT_UNKNOWN) {
			struct stat st;
			if (fstatat(dirfd(dir), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) == 0) {
				if (S_ISDIR(st.st_mode))      entry->d_type = DT_DIR;
				else if (S_ISLNK(st.st_mode)) entry->d_type = DT_LNK;
				else                           entry->d_type = DT_REG;
			}
		}

		if (entry->d_type == DT_DIR) {
			char *joined_path = NULL;
			if (asprintf(&joined_path, "%s/%s", dir_path, entry->d_name) < 0) {
				perror("asprintf");
				pthread_exit((void *)(intptr_t)-1);
			}
			search_filenames(joined_path, substrings, count);
			free(joined_path);
		} else {
			for (size_t i = 0; i < count; i++) {
				if (strstr(entry->d_name, substrings[i]) == NULL) continue;

				char *copy = NULL;
				if (asprintf(&copy, "%s/%s", dir_path, entry->d_name) < 0) {
					perror("asprintf");
					pthread_exit((void *)(intptr_t)-1);
				}

				int rc = con_str_vec_push(&matches, copy);
				if (rc != 0) {
					perror("realloc");
					free(copy);
					pthread_exit((void *)(intptr_t)-1);
				}

				break; // report each file at most once
			}
		}
	}

	pthread_cleanup_pop(1);
}

void *
worker_main(void *argument)
{
	struct worker_args *args = (struct worker_args *)argument;

	assert(root_directory != NULL);
	assert(args->count > 0);

	search_filenames(root_directory, args->substrings, args->count);

	pthread_exit(NULL);
	return NULL;
}
