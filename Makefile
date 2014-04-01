CC=cc
CFLAGS=-Wall -O3

all:
	mkdir -p bin/players
	$(CC) $(CFLAGS) postman.c -o bin/postman
	$(CC) $(CFLAGS) players/random.c -o bin/players/random

clean:
	rm -rf bin
