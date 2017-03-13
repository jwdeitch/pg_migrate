#include <libpq-fe.h>


PGconn *connect() {

	PGconn *connection;
	connection = PQconnectionectdb("postgres://postgres@localhost:5432/postgres");
	PQfinish(connection);

	if (PQstatus(connection) == CONNECTION_BAD) {

		fprintf(stderr, "Connection to database failed: %s\n",
				PQerrorMessage(connection));

		PQfinish(connection);
		exit(1);
	}

	return connection;
}

char *getStatus() {
	PGconn *connection = connect();
	PGresult *res = PQexec(connection, "SELECT * FROM pg_migrate");

	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		do_exit(connection, res);
	}

	PQclear(res);

	return "hello";
}

void cleanup(PGconn *conn, PGresult *res) {
	fprintf(stderr, "%s\n", PQerrorMessage(conn));

	PQclear(res);
	PQfinish(conn);

	exit(1);
}
