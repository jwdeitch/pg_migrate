#ifndef PG_MIGRATE_PG_H
#define PG_MIGRATE_PG_H
#include <libpq-fe.h>

void cleanup(PGconn *connection, PGresult *res);
PGconn *connect();

#endif //PG_MIGRATE_PG_H
