#include <libpq-fe.h>
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

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

	int t_length = 0;
	for (int j = rows - 1; j > -1; j--) {
		int length = strlen(PQgetvalue(res, j, 0));
		if (length > t_length) {
			t_length = length;
		}
	}
	printf("%*s Filename %*s  |  Batch  |      Time Performed\n", (t_length/2)-4, "", (t_length/2)-7, "");
	for (int i = rows - 1; i > -1; i--) {
		printf("%s |    %s    | %s\n", PQgetvalue(res, i, 0), PQgetvalue(res, i, 1), PQgetvalue(res, i, 2));
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
	PQclear(batchRes);
	if (strcmp(latestBatch,"0")) {
		latestBatch = (char*)"1\n";
	}

	while (strcmp(migrationsToBeRan[i], "\0") != 0) {
		FILE *f = fopen(migrationsToBeRan[i], "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fsize == 0) {
			printf("Skipping (file is empty): %s\n", migrationsToBeRan[i]);
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
		PQclear(res);

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
		PQclear(pgMigrateInsert);

		printf("Migrated: %s\n", migrationsToBeRan[i]);
		free(fileContents);
		i++;
	}

	printf("\nFinished\n");

}

char *rollbackMigrations(PGconn *connection) {

	PGresult *downMigrationRecords = PQexec(connection,
			"SELECT filename as up, replace(filename, '-up.sql', '-down.sql') as down"
			" FROM pg_migrate"
			" WHERE batch = ((SELECT max(batch) AS batch FROM pg_migrate));");
	if (PQresultStatus(downMigrationRecords) != PGRES_TUPLES_OK) {
		cleanup(connection, downMigrationRecords);
	}

	int rows = PQntuples(downMigrationRecords);

	if (rows == 0) {
		printf("Nothing to roll back\n");
		exit(0);
	}

	for (int i = 0; i < rows; i++) {
		char* downFilepath = PQgetvalue(downMigrationRecords, i, 1);
		char* upFilepath = PQgetvalue(downMigrationRecords, i, 0);

		if( access( downFilepath, F_OK ) == -1 ) {
			printf("Skipping (file does not exists): %s", downFilepath);
			continue;
		}

		FILE *f = fopen(downFilepath, "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fsize == 0) {
			printf("Skipping (file is empty): %s\n", downFilepath);
			i++;
			continue;
		}

		char *fileContents = malloc(fsize + 1);
		fread(fileContents, fsize, 1, f);
		fclose(f);

		fileContents[fsize] = 0;

		PGresult *res = PQexec(connection,fileContents);
		if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
			printf("\nFILE: %s\n", downFilepath);
			cleanup(connection, res);
		}
		PQclear(res);

		/*
		 *  TODO: concat to end of user input file
		 */
		char *insertQuery[PATH_MAX + 100];
		sprintf(insertQuery, "DELETE FROM pg_migrate WHERE filename = '%s';", upFilepath);
		PGresult *pgMigrateInsert = PQexec(connection, insertQuery);

		if (PQresultStatus(pgMigrateInsert) != PGRES_COMMAND_OK) {
			cleanup(connection, pgMigrateInsert);
		}
		PQclear(pgMigrateInsert);

		printf("Rolled back: %s\n", downFilepath);
		free(fileContents);
		i++;
	}
	PQclear(downMigrationRecords);

	printf("\nFinished\n");

}

int checkIfSetup(PGconn *connection) {

	PGresult *isSetupRes = PQexec(connection,"SELECT CASE WHEN EXISTS ("
			" SELECT 1"
			" FROM information_schema.tables"
			" WHERE table_name = 'pg_migrate'"
	") THEN 1 ELSE 0 END AS PROVISIONED;");

	if (PQresultStatus(isSetupRes) != PGRES_TUPLES_OK) {
		cleanup(connection, isSetupRes);
	}

	int provisioned = atoi(PQgetvalue(isSetupRes, 0, 0))==1;

	PQclear(isSetupRes);

	return provisioned;

}

void setup(PGconn *connection) {
	PGresult *checkIfProvisioned = PQexec(connection,
			"CREATE TABLE pg_migrate ("
					"filename       VARCHAR,"
					"batch          INT,"
	                "time_performed TIMESTAMP DEFAULT now()"
	         ");");
	if (PQresultStatus(checkIfProvisioned) != PGRES_COMMAND_OK) {
		cleanup(connection, checkIfProvisioned);
	}
}