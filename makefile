build:  src/pg.c
	cc `pkg-config --cflags libpq --libs libpq` -lm -g -Wall -Werror src/*.c -o pg_migrate

release:  src/pg.c
	cc -O3 `pkg-config --cflags libpq --libs libpq` -lm -Wall -Werror src/*.c -o pg_migrate
