#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

struct player {
	char *name;
	char *program;
	struct card **hand;
	pid_t pid;
	FILE *stdin;
	FILE *stdout;
};

struct character {
	int score;
	char *name;
	int cards;
};

struct card {
	struct character *character;
	struct player *player;
};

int main(int argc, char *argv[], char **envp);

int character_cards_init(int characters_length, struct character *characters, struct card **cards, struct player *players);

struct card *choose_card(struct card *cards, int *cards_drawn, int cards_length);
