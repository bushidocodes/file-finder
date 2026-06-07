#define _POSIX_C_SOURCE 200809
#define _DEFAULT_SOURCE

#include <assert.h>
#include <dirent.h>
#include <errno.h>
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

/*
 * Allocate and return "dir/name".  Calls pthread_exit on allocation
 * failure so the caller never receives a NULL — cleanup handlers
 * registered at that point will still fire normally.
 */
static char *
make_path(const char *dir, const char *name)
{
	char *path = NULL;
	if (asprintf(&path, "%s/%s", dir, name) < 0) {
		perror("asprintf");
		pthread_exit((void *)(intptr_t)-1);
	}
	return path;
}

static void
search_filenames(const char *dir_path, const char *const *substrings, size_t count)
{
	DIR *dir = opendir(dir_path);
	if (dir == NULL) {
		/* Permission denied on subdirectories is normal; report anything else */
		if (errno != EACCES && errno != EPERM)
			perror(dir_path);
		return;
	}
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
			char *joined_path = make_path(dir_path, entry->d_name);
			pthread_cleanup_push(free, joined_path);
			search_filenames(joined_path, substrings, count);
			pthread_cleanup_pop(1); /* free(joined_path) */
		} else {
			for (size_t i = 0; i < count; i++) {
				if (strstr(entry->d_name, substrings[i]) == NULL) continue;

				char *copy = make_path(dir_path, entry->d_name);
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
