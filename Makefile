all:
	mkdir -p bin/players
	clang postman.c -o bin/postman
	clang players/random.c -o bin/players/random

clean:
	rm -rf bin
