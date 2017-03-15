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
//	PGconn *connection = connect();

	PGresult *res = PQexec(connection, "SELECT * FROM pg_migrate");

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		cleanup(connection, res);
	}

	PQclear(res);

	return "hello";
}
