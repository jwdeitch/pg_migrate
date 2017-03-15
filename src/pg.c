#include <libpq-fe.h>
#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>

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

char *getStatus(PGconn *connection) {

	PGresult *res = PQexec(connection,
			"SELECT filename"
			" FROM pg_migrate"
			" ORDER BY batch DESC, time_performed DESC"
			" LIMIT 20;"
	);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cleanup(connection, res);
	}

	int rows = PQntuples(res);

	for(int i=0; i<rows; i++) {

		printf("%s\n", PQgetvalue(res, i, 0));
	}

	PQclear(res);

	return "hello";
}
