#ifndef PG_MIGRATE_DIFF_H
#define PG_MIGRATE_DIFF_H

#include "fs.h"

const char *missing_from_fs(char **, struct fs_discovered_migrations *);
const char *missing_from_db(char **, struct fs_discovered_migrations *);

#endif //PG_MIGRATE_DIFF_H
