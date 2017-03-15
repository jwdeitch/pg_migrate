#ifndef PG_MIGRATE_FS_H
#define PG_MIGRATE_FS_H

struct file_names {
	char name[250];
};

struct file_names* scan(const char* dir);

#endif //PG_MIGRATE_FS_H
