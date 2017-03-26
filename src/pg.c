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
	while (strcmp(migrationsToBeRan[i], "\0") != 0) {
		FILE *f = fopen(migrationsToBeRan[i], "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);  //same as rewind(f);

		if (fsize == 0) {
			printf("skipping %s - seems the file is empty\n", migrationsToBeRan[i]);
			continue;
		}

		char *fileContents = malloc(fsize + 1);
		fread(fileContents, fsize, 1, f);
		fclose(f);

		fileContents[fsize] = 0;


		PGresult *res = PQexec(connection,fileContents);

		if (PQresultStatus(res) != PGRES_TUPLES_OK) {
			cleanup(connection, res);
		}

		printf("Migrated -- %s\n", migrationsToBeRan[i]);
		i++;
	}

}