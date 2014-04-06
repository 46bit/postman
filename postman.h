#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <stdarg.h>

#define PLAY_PARSE_TARGET_PLAYER 1
#define PLAY_PARSE_TARGET_CHARACTER 2

#define DEBUG 1

struct postman {
	int characters_count;
	struct character *characters;

	int cards_count;
	int cards_drawn;
	struct card *cards;

	int players_count;
	int first_player_index;
	struct player *players;
	struct player *current_player;

	struct move *current_move;
};

struct character {
	int score;
	char *name;
	int cards_count;
	void (*play_handler)(struct postman *postman);
	int play_fieldmask;
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

struct move {
	struct character *played_character;
	struct player *target_player;
	struct character *target_character;
};

int main(int argc, char *argv[]);

struct postman *postman_init(int players_count, char **programs);

void character_cards_init(struct postman *postman);

void players_init(struct postman *postman, char **programs);

struct pipexec *new_pipexec(char *program);

void play_game(struct postman *postman);

struct card *choose_card(struct postman *postman);

void player_turn(struct postman *postman);

void player_draw(struct postman *postman, struct player *player, struct card *current_card);

void player_move(struct postman *postman);

int remove_character_from_hand(struct player *player, struct character *character);

struct player *parse_player_from_index(struct postman *postman, char *player_index_chars);

struct character *parse_character_from_name(struct postman *postman, char *character_name);

struct player *play_get_player(struct postman *postman, char **arguments);

struct character *play_get_character(struct postman *postman, char **arguments);

int parse_play(struct postman *postman, char *arguments, int flags);

void print_play(struct postman *postman);

void forfeit_player(struct postman *postman, struct player *target_player);

void tell_all_player_was_princessed(struct postman *postman, struct player *target_player);

void tell(FILE *target_pipe, char *message);

void tell_all(struct postman *postman, const char *format, ...);

void tell_player(struct player *player, const char *format, ...);

char *receive_player(struct player *player, int length);

void played_princess(struct postman *postman);

void played_general(struct postman *postman);

void played_wizard(struct postman *postman);

void played_priestess(struct postman *postman);

void played_knight(struct postman *postman);

void played_clown(struct postman *postman);

void played_soldier(struct postman *postman);

void score_game(struct postman *postman);

void cleanup_game(struct postman *postman);
