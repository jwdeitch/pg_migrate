#include <libpq-fe.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>
#include <string.h>

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

	PGresult *res = PQexec(connection,
						   query
	);

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


void getUpMigrations(PGconn *connection) {
	PGresult *res = PQexec(connection,
			"SELECT *"
			" FROM ("
			" SELECT filename"
			" FROM pg_migrate"
			" LIMIT 20) AS subq"
			" WHERE right(subq.filename, 7) = '-up.sql';");

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cleanup(connection, res);
	}

	int rows = PQntuples(res);

	printf("Filename\n");
	printf("------------------------------------\n");
	for (int i = rows - 1; i > -1; i--) {
		printf("%s\n", PQgetvalue(res, i, 0));
	}

	PQclear(res);
}