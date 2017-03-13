install:  src/pg.c
	cc `pkg-config --cflags libpq` `pkg-config --libs libpq` src/*
