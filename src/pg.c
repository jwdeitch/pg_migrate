/*
 * Includes logic pertaining to database interaction
 */

#include <libpq-fe.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#include <sys/syslimits.h>
#else
#include <linux/limits.h>
#endif

#include <execinfo.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

/*
 *  Write error messages, frees storages associated
 *  with PQResult and closes PQConn
 */
void cleanup(PGconn *connection, PGresult *res) {
	fprintf(stderr, "%s\n", PQerrorMessage(connection));

	PQclear(res);
	PQfinish(connection);

	exit(1);
}

/*
 * Gracefully create new PQConnection
 */
PGconn *getConnection(PGconn *connection, char* connStr) {

	connection = PQconnectdb(connStr);

	if (PQstatus(connection) != CONNECTION_OK) {

		fprintf(stderr, "Connection to database failed: %s\n",
				PQerrorMessage(connection));

		PQfinish(connection);
		exit(1);
	}

	return connection;
}

/*
 * Retrieve latest migrations from postgres,
 * prints directly to stdout
 */
void getLatest(PGconn *connection, int num) {
	char str[5];
	sprintf(str, "%d", num);

	char query[1000];
	strcpy(query, "SELECT filename, batch, to_char(time_performed, 'MM/DD/YY @ HH:MI:SS AM')"
			" FROM pgmigrate.manifest"
			" ORDER BY batch DESC, time_performed DESC"
			" LIMIT ");
	strcat(query, str);
	strcat(query, ";");

	PGresult *res = PQexec(connection, query);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cleanup(connection, res);
	}

	int rows = PQntuples(res);

	if (rows == 0) {
		printf("No migrations present in database\n");
		PQclear(res);
		exit(1);
	}

	int t_length = 0;
	for (int j = rows - 1; j > -1; j--) {
		int length = strlen(PQgetvalue(res, j, 0));
		if (length > t_length) {
			t_length = length;
		}
	}

	/*
	 * We are formatting this little ascii table by the length of the longest filesnames
	 */
	printf("%*s Filename %*s  |  Batch  |      Time Performed\n",
		   (int)ceil((t_length-strlen("filename")-1)/2), "",
		   (int)ceil((t_length-strlen("filename"))/2), "");

	for (int i = rows - 1; i > -1; i--) {
		char* filename = PQgetvalue(res, i, 0);
		printf("%s %*s  |    %s    | %s\n", filename, (int)(t_length - strlen(filename)), "", PQgetvalue(res, i, 1), PQgetvalue(res, i, 2));
	}

	PQclear(res);
}


/*
 * Retrieve all migrations from postgres
 */
char **getMigrationsFromDb(PGconn *connection) {
	PGresult *res = PQexec(connection, "SELECT filename FROM pgmigrate.manifest");

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		cleanup(connection, res);
	}

	int rows = PQntuples(res);

	char **list = malloc(1000 * sizeof(char *));
	memset(list, 0, 1000 * sizeof(char *));
	if (list == NULL) {
		PQfinish(connection);
		printf("malloc failed\n");
		exit(1);
	}

	int i = 0;
	for (; i < rows; i++) {
		list[i] = (char *) malloc(PATH_MAX + 1);
		memset(list[i], 0, PATH_MAX + 1);
		if (list[i] == NULL) {
			PQfinish(connection);
			printf("malloc failed\n");
			exit(1);
		}
		strcpy(list[i], strdup(PQgetvalue(res, i, 0)));
	}
	list[i + 1] = (char *) malloc(PATH_MAX + 1);
	memset(list[i + 1], 0, PATH_MAX + 1);
	if (list[i + 1] == NULL) {
		PQfinish(connection);
		printf("malloc failed\n");
		exit(1);
	}
	strcpy(list[i + 1], "\0");

	PQclear(res);

	return list;
}


/*
 * Run migrations on filesystem different from DB
 * Add file to manifest of ran migrations
 */
void runMigrations(PGconn *connection, char **migrationsToBeRan, int should_simulate) {

	int i = 0;
	PGresult *batchRes = PQexec(connection, "SELECT coalesce(max(batch) + 1,1) AS batch FROM pgmigrate.manifest");
	if (PQresultStatus(batchRes) != PGRES_TUPLES_OK) {
		cleanup(connection, batchRes);
	}

	char latestBatch[15];
	strcpy(latestBatch, PQgetvalue(batchRes, 0, 0));
	PQclear(batchRes);

	/*
	 * \0 denotes the terminating element of this array
	 */
	while (strcmp(migrationsToBeRan[i], "\0") != 0) {
		FILE *f = fopen(migrationsToBeRan[i], "rb");
		if(f == NULL) {
            printf("Cannot open migration file: %s\n", migrationsToBeRan[i]);
            exit(1);
        }
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		// should we actually run these migrations?
		if (should_simulate == 0) {
			if (fsize == 0) {
				printf("Skipping (file is empty): %s\n", migrationsToBeRan[i]);
				i++;
				continue;
			}

			char *fileContents = malloc(fsize + 1);
			memset(fileContents, 0, fsize + 1);
			if (fileContents == NULL) {
				printf("malloc failed to allocate\n");
				exit(1);
			}
			fread(fileContents, fsize, 1, f);
			fclose(f);

			fileContents[fsize] = 0;

			PGresult *res = PQexec(connection, fileContents);
			if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
				printf("\nFILE: %s\n", migrationsToBeRan[i]);
				cleanup(connection, res);
			}
			PQclear(res);

			/*
			 *  TODO: consider concatenating this to the end of the users input
			 *  to provide transactional support of inserting the migration record
			 */
			char insertQuery[PATH_MAX + 100];
			sprintf(insertQuery, "INSERT INTO pgmigrate.manifest (filename, batch) VALUES ('%s', %s);", migrationsToBeRan[i],
					latestBatch);
			PGresult *pgMigrateInsert = PQexec(connection, insertQuery);

			if (PQresultStatus(pgMigrateInsert) != PGRES_COMMAND_OK) {
				cleanup(connection, pgMigrateInsert);
			}
			PQclear(pgMigrateInsert);

			printf("Migrated: %s\n", migrationsToBeRan[i]);
			free(fileContents);
		} else {
		    if (fsize == 0) {
			    printf("(simulated) Skipping (file is empty): %s\n", migrationsToBeRan[i]);
			}
            printf("(simulated) Migrate: %s\n", migrationsToBeRan[i]);
		}
		i++;
	}

	printf("\nFinished\n");

}

/*
 * Remove file to manifest of ran migrations
 */
void runRollbackFile(PGconn *connection, char* upFilepath, char* downFilepath) {
	char insertQuery[PATH_MAX + 100];
	sprintf(insertQuery, "DELETE FROM pgmigrate.manifest WHERE filename = '%s';", upFilepath);
	PGresult *pgMigrateInsert = PQexec(connection, insertQuery);

	if (PQresultStatus(pgMigrateInsert) != PGRES_COMMAND_OK) {
		cleanup(connection, pgMigrateInsert);
	}
	PQclear(pgMigrateInsert);
}

/*
 * Rollback migrations by order of batch
 */
void rollbackMigrations(PGconn *connection, int should_simulate) {

	PGresult *downMigrationRecords = PQexec(connection,
			"SELECT filename AS up, replace(filename, '-up.sql', '-down.sql') AS down"
			" FROM pgmigrate.manifest"
			" WHERE batch = ((SELECT max(batch) AS batch FROM pgmigrate.manifest));");
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
			printf("Skipping (file does not exists): %s\n", downFilepath);
			if (should_simulate == 0) {

			    /* How to handle missing down migrations on FS?
			     * Safest way to ensure data integrity is to abort down migration all-together.
			     * Previous to this change, pg_migrate would just continue as normal:
			     *      runRollbackFile(connection, upFilepath, downFilepath);
			     */

                printf("Aborting rollback - can't rollback if you're missing: %s\n", downFilepath);
                exit(1);
			}
			continue;
		}

		FILE *f = fopen(downFilepath, "rb");
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fsize == 0) {
			printf("Skipping (file is empty): %s\n", downFilepath);
			if (should_simulate == 0) {
				runRollbackFile(connection, upFilepath, downFilepath);
			}
			continue;
		}

		if (should_simulate == 0) {
			char *fileContents = malloc(fsize + 1);
			memset(fileContents, 0, fsize + 1);
			if (fileContents == NULL) {
				printf("malloc failed to allocate\n");
				exit(1);
			}
			fread(fileContents, fsize, 1, f);
			fclose(f);

			fileContents[fsize] = 0;

			PGresult *res = PQexec(connection, fileContents);
			if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
				printf("\nFILE: %s\n", downFilepath);
				cleanup(connection, res);
			}
			PQclear(res);
			free(fileContents);
			runRollbackFile(connection, upFilepath, downFilepath);
			printf("Rolled back: %s\n", downFilepath);
		} else {
    		if (fsize == 0) {
	    	    printf("(simulated) Skipping (file is empty): %s\n", downFilepath);
    		}
			printf("(simulated) Roll back: %s\n", downFilepath);
		}
	}
	PQclear(downMigrationRecords);

	printf("\nFinished\n");

}

int checkIfSetup(PGconn *connection) {

	PGresult *isSetupRes = PQexec(connection,"SELECT CASE WHEN EXISTS ("
			" SELECT 1"
			" FROM information_schema.tables"
			" WHERE table_name = 'manifest' AND table_schema = 'pgmigrate'"
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
			"CREATE SCHEMA pgmigrate;"
			"CREATE TABLE pgmigrate.manifest ("
					"filename       VARCHAR,"
					"batch          INT,"
	                "time_performed TIMESTAMP DEFAULT now()"
	         ");");
	if (PQresultStatus(checkIfProvisioned) != PGRES_COMMAND_OK) {
		cleanup(connection, checkIfProvisioned);
	}
}
