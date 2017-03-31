#include "fs.h"
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct fs_discovered_migrations *file_names_arr;
int *itr;

//http://stackoverflow.com/a/37188697/4603498
int string_ends_with(const char *str, const char *suffix) {
	int str_len = strlen(str);
	int suffix_len = strlen(suffix);

	return
			(str_len >= suffix_len) &&
			(0 == strcmp(str + (str_len - suffix_len), suffix));
}


// http://stackoverflow.com/a/8438663/4603498
void listdir(const char *name, int level) {
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name)))
		return;
	if (!(entry = readdir(dir)))
		return;

	do {
		if (entry->d_type == DT_DIR) {
			char path[1024];
			int len = snprintf(path, sizeof(path) - 1, "%s/%s", name, entry->d_name);
			path[len] = 0;
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
			}
			listdir(path, level + 1);
		} else {
			char path[PATH_MAX];
			strcpy(path, name);
			strcat(path, "/");
			strcat(path, entry->d_name);
			if (string_ends_with(entry->d_name, "-up.sql")) {
				file_names_arr[*itr].up = true;
				realpath(path, file_names_arr[*itr].name);
				*itr = *itr + 1;
			}
		}

	} while ((entry = readdir(dir)));
	closedir(dir);
}


struct fs_discovered_migrations *getMigrationsFromFs(const char *dir) {
	file_names_arr = calloc(1000, sizeof(*file_names_arr));
	itr = calloc(1000, sizeof(int));
	*itr = 0;
	listdir(".", 15);
	strcpy(file_names_arr[*itr].name, "\0");
	return file_names_arr;

}