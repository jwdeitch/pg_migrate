#include <libpq-fe.h>
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <string.h>
#include <dirent.h>

void cleanup(PGconn *connection, PGresult *res) {
	fprintf(stderr, "%s\n", PQerrorMessage(connection));

	PQclear(res);
	PQfinish(connection);

	exit(1);
}


PGconn *getConnection(PGconn *connection) {

	connection = PQconnectdb("postgres://postgres:1234@localhost:5432/postgres");

	if (PQstatus(connection) != CONNECTION_OK) {

		fprintf(stderr, "Connection to database failed: %s\n",
				PQerrorMessage(connection));

		PQfinish(connection);
		exit(1);
	}

	return connection;
}

char *getLatest(PGconn *connection, int num) {
	char str[5];
	sprintf(str, "%d", num);

	char *query[1000];
	strcpy(query, "SELECT filename, batch, to_char(time_performed, 'MM/DD/YY @ HH:MI:SS AM')"
			" FROM pg_migrate"
			" ORDER BY batch DESC, time_performed DESC"
			" LIMIT ");
	strcat(query, str);
	strcat(query, ";");

	PGresult *res = PQexec(connection, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cleanup(connection, res);
	}

	int rows = PQntuples(res);

	printf("Filename | Batch | Time performed\n");
	printf("------------------------------------\n");
	for (int i = rows - 1; i > -1; i--) {
		printf("%s | %s | %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1), PQgetvalue(res, i, 2));
	}

	PQclear(res);
}


char **getMigrationsFromDb(PGconn *connection) {
	PGresult *res = PQexec(connection, "SELECT filename FROM pg_migrate");

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cleanup(connection, res);
	}

	int rows = PQntuples(res);

	char **list = malloc(1000 * sizeof(char *));
	if (list == NULL) {
		PQfinish(connection);
		exit(1);
	}

	int i = 0;
	for (i; i < rows; i++) {
		list[i] = (char *) malloc(PATH_MAX + 1);
		strcpy(list[i], strdup(PQgetvalue(res, i, 0)));
	}
	list[i + 1] = (char *) malloc(PATH_MAX + 1);
	strcpy(list[i + 1], "\0");

	PQclear(res);

	return list;
}

// http://stackoverflow.com/a/14002993/4603498
char *runMigrations(PGconn *connection, char **migrationsToBeRan) {

	int i = 0;
	PGresult *batchRes = PQexec(connection, "select max(batch) + 1 as batch from pg_migrate");

	if (PQresultStatus(batchRes) != PGRES_TUPLES_OK) {
		cleanup(connection, batchRes);
	}

	char* latestBatch = PQgetvalue(batchRes, 0, 0);

	if (strcmp(latestBatch,"0")) {
		latestBatch = (char*)"1\n";
	}

	while (strcmp(migrationsToBeRan[i], "\0") != 0) {
		FILE *f = fopen(migrationsToBeRan[i], "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);  //same as rewind(f);

		if (fsize == 0) {
			printf("Skipping: %s - file is empty\n", migrationsToBeRan[i]);
			i++;
			continue;
		}

		char *fileContents = malloc(fsize + 1);
		fread(fileContents, fsize, 1, f);
		fclose(f);

		fileContents[fsize] = 0;

		PGresult *res = PQexec(connection,fileContents);

		if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
			printf("\nFILE: %s\n", migrationsToBeRan[i]);
			cleanup(connection, res);
		}

		/*
		 *  TODO: consider concatenating this to the end of the users input
		 *  to provide transactional support of inserting the migration record
		 */
		char *insertQuery[PATH_MAX + 100];
		sprintf(insertQuery, "INSERT INTO pg_migrate (filename, batch) VALUES ('%s', %s);", migrationsToBeRan[i], latestBatch);
		PGresult *pgMigrateInsert = PQexec(connection, insertQuery);

		if (PQresultStatus(pgMigrateInsert) != PGRES_COMMAND_OK) {
			cleanup(connection, pgMigrateInsert);
		}

		printf("Migrated: %s\n", migrationsToBeRan[i]);
		free(fileContents);
		i++;
	}

	printf("\nFinished\n");

}