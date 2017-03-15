#include "fs.h"
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

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
		} else
			printf("%s/%s\n", name, entry->d_name);
	} while (entry = readdir(dir));
	closedir(dir);
}


struct file_names *scan(const char *dir) {

	listdir(".", 15);

}
