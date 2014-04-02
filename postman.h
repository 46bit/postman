#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>

struct postman {
	int characters_count;
	struct character *characters;

	int cards_count;
	int cards_drawn;
	struct card *cards;

	int players_count;
	struct player *players;
	struct player *current_player;
};

struct character {
	int score;
	char *name;
	int cards_count;
};

struct card {
	struct character *character;
	struct player *player;
};

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
	int playing;
	int protected;
	char *name;
	struct card **hand;
	struct pipexec *pipexec;
};

int main(int argc, char *argv[]);

struct postman *postman_init(int players_count, char **programs);

void character_cards_init(struct postman *postman);

void players_init(struct postman *postman, char **programs);

struct pipexec *new_pipexec(char *program);

void play_game(struct postman *postman);

struct card *choose_card(struct postman *postman);

void player_draw(struct postman *postman, struct card *current_card);

int player_move(struct postman *postman);

struct character *player_played_character(struct postman *postman, char *character_name);

struct player *player_targeted_player(struct postman *postman, char *player_index_chars);

struct character *player_targeted_character(struct postman *postman, char *character_name);

void forfeit_player(struct postman *postman, struct player *target_player);

void played_princess(struct postman *postman);

void played_general(struct postman *postman, struct player *target_player);

void played_wizard(struct postman *postman, struct player *target_player);

void played_priestess(struct postman *postman);

void played_knight(struct postman *postman, struct player *target_player);

void played_clown(struct postman *postman, struct player *target_player);

void played_soldier(struct postman *postman, struct player *target_player, struct character *target_character);

void score_game(struct postman *postman);

void cleanup_game(struct postman *postman);
