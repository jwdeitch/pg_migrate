build:  src/pg.c
	cc `pkg-config --cflags libpq --libs libpq` src/*.h src/*.c -o pg_migrate

release:  src/pg.c
	cc -O3 `pkg-config --cflags libpq --libs libpq` src/*.h src/*.c -o pg_migrate
