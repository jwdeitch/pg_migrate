#include <stdio.h>
#include "pg.h"
#include "fs.h"
#include "diff.h"

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("Looks like you either didn't provide a postgres path, or you provided an additional argument.");
		return 0;
	}

	PGconn *connection;
	connection = getConnection(connection);

//	getLatest(connection, 10);
//	exit(0);

//	char** migrationToBeRan = missing_from_db(getMigrationsFromDb(connection),getMigrationsFromFs("."));
//
//	if (strcmp(migrationToBeRan[0], "\0") == 0) {
//		printf("Nothing to migrate\n");
//		exit(0);
//	}

//	runMigrations(connection, migrationToBeRan);
	rollbackMigrations(connection);
	return 1;
}



