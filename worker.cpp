#define _POSIX_C_SOURCE 200809
#define _DEFAULT_SOURCE

#include <filesystem>

#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "con_str_vec.hpp"

extern std::filesystem::path root_directory;
extern struct con_str_vec matches;

static inline void
search_filenames(std::filesystem::path dir_path, char *substring)
{
	DIR *dir = opendir(dir_path.c_str());
	if (dir == NULL)
	{
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	struct dirent *entry;

	while ((entry = readdir(dir)) != nullptr)
	{
		// Skip links, and . and ..
		if (entry->d_type == DT_LNK || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		if (entry->d_type == DT_DIR)
		{
			// dirent has a static buffer of 256, so doubling to prevent clang nits
			char joined_path[513] = {0};
			snprintf(joined_path, 512, "%s/%s", dir_path.c_str(), entry->d_name);
			search_filenames(joined_path, substring);
		}

		if (strstr(entry->d_name, substring) != nullptr)
		{
			char *copy = strdup(entry->d_name);
			if (copy == nullptr)
			{
				perror("strdup");
				exit(EXIT_FAILURE);
			}

			int rc = matches.push(copy);
			if (rc != 0)
			{
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

	// TODO: Assert that was set
	// assert(root_directory != nullptr);

	search_filenames(root_directory, substring);

	pthread_exit(nullptr);
}