#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include "pg.h"
#include "fs.h"
#include "diff.h"

void printHelp();

int main(int argc, char *argv[]) {
	extern char *optarg;
	extern int optind, opterr, optopt;

	/*
	 * s-status, H-Host, u-up, d-down, v-version, p-soft, g-provision, h-help
	 */
	int c = 0, u = 0, d = 0, s = 0, g = 0, p = 0, H = 0, err = 0;
	char *connStr =  (char*)malloc(PATH_MAX*sizeof(char*));
	if( connStr == NULL ) {
		printf("malloc failed to dimension connStr");
		exit(1);
	}

	while ((c = getopt(argc, argv, "sH:udvhgp")) != -1) {
		switch (c) {
			case 'd':
				if (d == 1 || u == 1) {
					printf("invalid flag entry\n");
					exit(1);
				}
				d = 1;
				break;
			case 'p':
				if (d == 1) {
					printf("duplicate -p flag\n");
					exit(1);
				}
				p = 1;
				break;
			case 'H':
				if (H == 1) {
					printf("duplicate -H flag\n");
					exit(1);
				}
				H = 1;
				connStr = optarg;
				break;
			case 'v':
				printf("pg_migrate version 0.0.1\n");
				exit(0);
			case 'u':
				if (d == 1 || u == 1) {
					printf("invalid flag entry\n");
					exit(1);
				}
				u = 1;
				break;
			case 's':
				if (s == 1) {
					printf("duplicate -s flag");
				}
				s = 1;
				break;
			case 'g':
				if (g == 1) {
					printf("duplicate -g flag");
				}
				g = 1;
				break;
			case 'h':
				printHelp();
				exit(1);
		}
	}

	if (H == 0) {
		printf("No Host url argument provided\n");
		return 1;
	}

	PGconn *connection;
	connection = getConnection(connection, connStr);

	int is_setup = checkIfSetup(connection);

	if (g) {
		if (is_setup == 1) {
			printf("Already previously provisioned\n");
			return 1;
		}
		setup(connection);
		printf("Setup successful\n");
		return 1;
	}

	if (is_setup == 0) {
		printf("ERROR: pg_migrate table not found in public schema\nRun `pg_migrate -H [host url] -g`\n");
		return 2;
	}

	if (s) {
		getLatest(connection, 20);
		return 1;
	}

	if (u + d == 0) {
		printf("Unspecified migration direction: up or down\n");
		return 1;
	}

	if (d) {
		rollbackMigrations(connection, p);
		return 1;
	}

	if (u) {
		char *file = argv[optind];
		char *path[PATH_MAX];
		realpath(file, path);
		DIR* dir = opendir(path);
		if (!dir) {
			fprintf(stderr, "No valid directory provided %s\n", path);
			exit(1);
		}
		closedir(dir);
		char **migrationToBeRan = missing_from_db(getMigrationsFromDb(connection), getMigrationsFromFs(path));
		if (strcmp(migrationToBeRan[0], "\0") == 0) {
			printf("Nothing to migrate\n");
			return 1;
		}
		runMigrations(connection, migrationToBeRan, p);
	}

	return 1;
}

void printHelp() {

	printf("pg_migrate 0.0.1\n"
			"https://github.com/jwdeitch/pg_migrate\n"
			"MIT 2017\n\n"

			"usage: pg_migrate -H [options]... dir\n\n"

		"   -H        Host (in postgres URI format)\n"
		"   -s        Show last 10 forward migrations ran\n"
		"   -u        Migrate forward. Recursively traverses provided directory for -up.sql files\n"
		"   -d        Migrate rollback. Will attempt to locate matching -down.sql files to migrate backwards\n"
		"   -p        Soft run. Will display migrations to be ran / rolled back\n"
		"   -g        Provisions the public schema with the pg_migrate table, used to track migrations\n"

	);

}

