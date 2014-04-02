#include "postman.h"

#define DEBUG 1

int main(int argc, char *argv[])
{
	// Seed rand from microtime.
	struct timeval time;
	gettimeofday(&time, NULL);
	long microsec = ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
	srand(microsec);

	// Setup game characters and cards.
	int characters_count = 8;
	struct character characters[8] = {
		{8, "Princess", 1},
		{7, "Minister", 1},
		{6, "General", 1},
		{5, "Wizard", 2},
		{4, "Priestess", 2},
		{3, "Knight", 2},
		{2, "Clown", 2},
		{1, "Soldier", 5}
	};
	struct card *cards;
	int cards_drawn = 0, cards_length;
	cards_length = character_cards_init(characters_count, characters, &cards);

	// Setup game players as provided in argv.
	int i, player_count = argc - 1;
	struct player *players = malloc(player_count * sizeof(struct player));
	players_init(player_count, players, argv+1);

	// Play game until cards run out.
	struct card *picked_card;
	int current_player_index = 0, initial_cards_drawn = 0;
	while ((picked_card = choose_card(cards, cards_drawn, cards_length)) != NULL)
	{
		// Check we have at least 2 players not out. End game if appropriate.
		int p, active_players_count = 0;
		for (p = 0; p < player_count; p++)
		{
			if (players[p].playing)
			{
				active_players_count++;
			}
		}
		if (active_players_count < 2)
		{
			#if DEBUG==1
				printf("\nEnd of game, only %d players remaining.\n", active_players_count);
			#endif
			break;
		}

		struct player *current_player = &players[current_player_index];
		if (current_player->playing)
		{
			current_player->protected = 0;
			cards_drawn++;

			player_draw(player_count, players, current_player, picked_card);
			if (initial_cards_drawn && player_move(current_player, player_count, players, characters_count, characters) == -1)
			{
				// @TODO: player did an invalid move, forfeit the game.
				printf("\nPlayer %s did an invalid move.\n", current_player->name);
				current_player->playing = 0;
			}
		} else {
			#if DEBUG==1
				printf("\nPlayer %d is out.\n", current_player->index);
			#endif
		}

		// This can be done in one line except detection of when initial cards
		// have been distributed to all players is easiest here.
		current_player_index++;
		if (current_player_index >= player_count) {
			initial_cards_drawn = 1;
			current_player_index = current_player_index % player_count;
		}
	}

	// Once cards run out determine the winning player.
	int top_score = 0;
	struct card *top_card = NULL;
	for (i = 0; i < player_count; i++)
	{
		struct player *current_player = &players[i];
		int current_score = current_player->hand[0]->character->score;
		if (current_player->playing && current_score > top_score)
		{
			top_score = current_score;
			top_card = current_player->hand[0];
		}

		kill(players[i].pid, SIGTERM);
	}
	printf("Player %d %s won with a %s card.\n", top_card->player->index, top_card->player->name, top_card->character->name);

	for (i = 0; i < player_count; i++)
	{
		struct player *current_player = &players[i];

		free(current_player->hand);
		free(current_player->name);
		fclose(current_player->stdin);
		fclose(current_player->stdout);
	}
	free(players);
	free(cards);

	return 0;
}

int character_cards_init(int characters_length, struct character *characters, struct card **ret_cards)
{
	struct character *current_character;
	int i, cards_length = 0;
	for (i = 0; i < characters_length; i++)
	{
		current_character = &characters[i];
		cards_length += current_character->cards;
	}

	struct card *cards = malloc(cards_length * sizeof(struct card));

	int card_index = 0;
	for (i = 0; i < characters_length; i++)
	{
		current_character = &characters[i];
		int character_card_index;
		for (character_card_index = 0; character_card_index < current_character->cards; character_card_index++)
		{
			struct card *current_card = &cards[card_index];
			current_card->character = current_character;
			current_card->player = NULL;
			card_index++;
		}
	}

	(*ret_cards) = cards;
	return cards_length;
}

void players_init(int player_count, struct player *players, char **programs)
{
	int i;
	for (i = 0; i < player_count; i++)
	{
		// Fork off the player program then save pid/stdin/stdout for our use.
		struct player *current_player = &players[i];
		current_player->index = i;
		current_player->playing = 1;
		current_player->protected = 0;
		current_player->program = programs[i];
		struct pipexec *p = new_pipexec(current_player->program);
		current_player->pid = p->pid;
		current_player->stdin = p->stdin;
		current_player->stdout = p->stdout;
		free(p);

		// Finish player setup
		current_player->hand = malloc(2 * sizeof(*current_player->hand));
		current_player->hand[0] = current_player->hand[1] = NULL;

		// Get name of player as it output on stdout.
		char ai_name[31];
		fgets(ai_name, 31, current_player->stdout);
		strtok(ai_name, "\n");
		players[i].name = malloc(strlen(ai_name) + 1);
		strcpy(current_player->name, ai_name);

		printf("Started player %s from %s\n", current_player->name, current_player->program);
	}
}

struct pipexec *new_pipexec(char *program)
{
	struct pipexec *p = malloc(sizeof(struct pipexec));
	pid_t stdin_pipe[2], stdout_pipe[2];
	if (pipe(stdin_pipe) || pipe(stdout_pipe))
	{
		return NULL;
	}

	p->program = program;
	pid_t pid = fork();
	if (pid == 0)
	{
		dup2(stdin_pipe[0], 0);
		dup2(stdout_pipe[1], 1);
		execl(program, program, NULL);
	}
	p->pid = pid;
	p->stdin = fdopen(stdin_pipe[1], "w");
	p->stdout = fdopen(stdout_pipe[0], "r");
	return p;
}

struct card *choose_card(struct card *cards, int cards_drawn, int cards_length)
{
	struct card *chosen_card = NULL;
	if (cards_drawn < cards_length)
	{
		while (1)
		{
			int chosen_card_index = rand() % cards_length;
			chosen_card = &(cards[chosen_card_index]);
			if (chosen_card->player == NULL)
			{
				break;
			}
		}
	}
	return chosen_card;
}

void player_draw(int player_count, struct player *players, struct player *current_player, struct card *current_card)
{
	current_card->player = current_player;

	if (current_player->hand[0] == NULL) {
		current_player->hand[0] = current_card;
	} else {
		current_player->hand[1] = current_card;
	}

	#if DEBUG==1
		printf("\nplayer %d\n", current_player->index);
		printf("draw %s\n", current_card->character->name);
	#endif

	int p;
	for (p = 0; p < player_count; p++)
	{
		if (players[p].playing)
		{
			fprintf(players[p].stdin, "player %d\n", current_player->index);
		}
	}
	fprintf(current_player->stdin, "draw %s\n", current_card->character->name);
	fflush(current_player->stdin);
}

int player_move(struct player *current_player, int player_count, struct player *players, int character_count, struct character *characters)
{
	int valid = -1;
	char ai_move[31];
	fgets(ai_move, 31, current_player->stdout);
	#if DEBUG==1
		printf("%s", ai_move);
	#endif
	strtok(ai_move, "\n ");

	if (strcmp(ai_move, "forfeit") == 0) {
		valid = 1;
		printf("=> forfeited\n");
		// @TODO: make players who forfeit stop playing
	}

	if (strcmp(ai_move, "play") == 0) {
		valid = 1;

		char *ai_move_location = ai_move + 5;
		#if DEBUG==1
			printf("played %s\n", ai_move_location);
		#endif
		int p;
		for (p = 0; p < player_count; p++)
		{
			if (players[p].playing)
			{
				fprintf(players[p].stdin, "played %s\n", ai_move_location);
			}
		}

		strtok(ai_move_location, "\n ");
		struct character *played_character;
		if ((played_character = player_played_character(current_player, ai_move_location)) == NULL)
		{
			// @TODO: player did an invalid move, forfeit the game.
			printf("Player %s did an invalid move.\n", current_player->name);
			return -1;
		}

		struct player *target_player = NULL;
		struct character *target_character = NULL;
		if (played_character->score < 7 && played_character->score > 0
			&& played_character->score != 4)
		{
			ai_move_location += strlen(played_character->name) + 1;
			strtok(ai_move_location, "\n ");

			target_player = player_targeted_player(players, ai_move_location);
			if (target_player == NULL)
			{
				printf("Player %s specified an invalid target player.\n", current_player->name);
				return -1;
			}

			if (played_character->score == 1)
			{
			// @TODO: stop assuming numbers lack leading zeros, explicitly use how far
			// player_targeted_player parsed.
				int target_player_index_char_length = (target_player->index / 10) + 1;
				ai_move_location += target_player_index_char_length + 1;
				strtok(ai_move_location, "\n ");

				target_character = player_targeted_character(character_count, characters, ai_move_location);
				if (target_character == NULL)
				{
					printf("Player %s specified an invalid target character.\n", current_player->name);
					return -1;
				}
			}
		}

		switch (played_character->score)
		{
		case 8:
			// Princess
			current_player->playing = 0;
			#if DEBUG==1
				printf("out %d %s\n", current_player->index, current_player->hand[0]->character->name);
			#endif
			int p;
			for (p = 0; p < player_count; p++)
			{
				if (players[p].playing)
				{
					fprintf(players[p].stdin, "out %d %s\n", current_player->index, current_player->hand[0]->character->name);
				}
			}
			break;
		case 7:
			// Minister
			// @TODO: effect of minister takes place on draw rather than play
			break;
		case 6:
			// General
			if (target_player->playing && !target_player->protected)
			{
				fprintf(current_player->stdin, "swap %s\n", target_player->hand[0]->character->name);
				fprintf(target_player->stdin, "swap %s\n", current_player->hand[0]->character->name);
				struct card *temp_card = current_player->hand[0];
				current_player->hand[0] = target_player->hand[0];
				target_player->hand[0] = temp_card;
			} else {
				// @TODO: handle case where target player is out or has played a
				// Priestess. What do we send to the players to describe this
				// situation?
			}
			break;
		case 5:
			// Wizard
			// @TODO: target_player discards hand, draws new card
			// Run `discard` for the card in their hand. `out` describes the
			// cards in a player hand as well.
			// @TODO: Tidy drawing new card into routine that doesn't need 15
			// lines of code here.
			//fprintf(target_player->stdin, "discard %s\n", target_player->hand[0]->character->name);
			//fprintf(target_player->stdin, "draw %s\n", );
			break;
		case 4:
			// Priestess
			// We unprotect a player each time their turn comes up.
			current_player->protected = 1;
			break;
		case 3:
			// Knight
			// @TODO: Internally compare value of card in each player's hand,
			// `out` iff one player scores lower than other.
			// @TODO: why on earth does not cc nor clang parse without the semicolon??
			;
			int current_score = current_player->hand[0]->character->score;
			int target_score = target_player->hand[0]->character->score;
			struct player *out_player = NULL;
			if (current_score < target_score)
			{
				out_player = current_player;
			} else if (target_score < current_score) {
				out_player = target_player;
			} else {
				// @TODO: what to send to bots when the Knight is a draw?
			}
			if (out_player != NULL)
			{
				out_player->playing = 0;
				#if DEBUG==1
					printf("out %d %s\n", out_player->index, out_player->hand[0]->character->name);
				#endif
				int p;
				for (p = 0; p < player_count; p++)
				{
					if (players[p].playing)
					{
						fprintf(players[p].stdin, "out %d %s\n", out_player->index, out_player->hand[0]->character->name);
					}
				}
			}
			break;
		case 2:
			// Clown
			// @TODO: reveal card in target_player hand to current_player only.
			fprintf(current_player->stdin, "reveal %s\n", target_player->hand[0]->character->name);
			break;
		case 1:
			// Soldier
			// @TODO: check if target_player has target_character card. If they
			// do have the card, `out`.
			if (target_player->hand[0]->character == target_character)
			{
				target_player->playing = 0;
				#if DEBUG==1
					printf("out %d %s\n", target_player->index, target_player->hand[0]->character->name);
				#endif
				int p;
				for (p = 0; p < player_count; p++)
				{
					if (players[p].playing)
					{
						fprintf(players[p].stdin, "out %d %s\n", target_player->index, target_player->hand[0]->character->name);
					}
				}
			}
			break;
		}
	}

	return valid;
}

struct character *player_played_character(struct player *current_player, char *character_name)
{
	struct character *c = NULL;
	if (current_player->hand[0] != NULL && strcmp(current_player->hand[0]->character->name, character_name) == 0)
	{
		c = current_player->hand[0]->character;
		current_player->hand[0] = current_player->hand[1];
		current_player->hand[1] = NULL;
	} else if (current_player->hand[1] != NULL && strcmp(current_player->hand[1]->character->name, character_name) == 0)
	{
		c = current_player->hand[1]->character;
		current_player->hand[1] = NULL;
	}
	return c;
}

struct player *player_targeted_player(struct player *players, char *player_index_chars)
{
	struct player *p = NULL;
	int player_index = (*player_index_chars - '0');
	if (player_index >= 0 && player_index < 4) {
		p = &players[player_index];
	}
	return p;
}

struct character *player_targeted_character(int character_count, struct character *characters, char *character_name)
{
	struct character *c = NULL;
	int i;
	for (i = 0; i < character_count; i++)
	{
		if (strcmp(characters[i].name, character_name) == 0)
		{
			c = &characters[i];
			break;
		}
	}
	return c;
}
