#ifndef PG_MIGRATE_FS_H
#define PG_MIGRATE_FS_H

#include <linux/limits.h>
#include <stdbool.h>

struct file_names {
	char name[PATH_MAX];
	bool up;
};

struct file_names* scan(const char* dir);

#endif //PG_MIGRATE_FS_H
