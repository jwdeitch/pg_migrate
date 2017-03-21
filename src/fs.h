#ifndef PG_MIGRATE_FS_H
#define PG_MIGRATE_FS_H

#include <linux/limits.h>
#include <stdbool.h>

struct fs_discovered_migrations {
	char name[PATH_MAX];
	bool up;
};

struct fs_discovered_migrations* getMigrationsFromFs(const char *dir);

#endif //PG_MIGRATE_FS_H
