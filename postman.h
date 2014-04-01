#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

struct pipexec {
	char *program;
	pid_t pid;
	FILE *stdin;
	pid_t stdin_pipe[2];
	FILE *stdout;
	pid_t stdout_pipe[2];
};

struct player {
	int index;
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

int character_cards_init(int characters_length, struct character *characters, struct card **cards);

void players_init(int player_count, struct player *players, char **programs);

struct pipexec *new_pipexec(char *program);

struct card *choose_card(struct card *cards, int *cards_drawn, int cards_length);

void player_draw(struct player *current_player, struct card *current_card);

int player_move(struct player *current_player, int player_count, struct player *players, int character_count, struct character *characters);

struct character *player_played_character(struct player *current_player, char *character_name);

struct player *player_targeted_player(int player_count, struct player *players, char *player_index_chars);

struct character *player_targeted_character(int character_count, struct character *characters, char *character_name);
