#ifndef PG_MIGRATE_PG_H
#define PG_MIGRATE_PG_H
#include <libpq-fe.h>

void cleanup(PGconn *connection, PGresult *res);
PGconn *getConnection(PGconn *connection);
char *getLatest(PGconn *connection, int num);
char *getMigrationsFromDb(PGconn *connection);

#endif //PG_MIGRATE_PG_H
