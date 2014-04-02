#include "postman.h"

#define DEBUG 1

int main(int argc, char *argv[])
{
	// Seed rand from microtime.
	struct timeval time;
	gettimeofday(&time, NULL);
	long microsec = ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
	srand(microsec);

	// Setup Postman state
	struct postman *postman = postman_init(argc - 1, argv + 1);
	play_game(postman);
	score_game(postman);
	cleanup_game(postman);

	return 0;
}

struct postman *postman_init(int players_count, char **programs)
{
	struct postman *postman = malloc(sizeof(struct postman));

	// Setup game characters
	postman->characters_count = 8;
	postman->characters = malloc(postman->characters_count * sizeof(struct character));
	postman->characters[0] = (struct character) {8, "Princess", 1};
	postman->characters[1] = (struct character) {7, "Minister", 1};
	postman->characters[2] = (struct character) {6, "General", 1};
	postman->characters[3] = (struct character) {5, "Wizard", 2};
	postman->characters[4] = (struct character) {4, "Priestess", 2};
	postman->characters[5] = (struct character) {3, "Knight", 2};
	postman->characters[6] = (struct character) {2, "Clown", 2};
	postman->characters[7] = (struct character) {1, "Soldier", 5};

	// Setup game cards from characters
	postman->cards_drawn = 0;
	character_cards_init(postman);

	// Setup game players as provided in argv
	postman->players_count = players_count;
	players_init(postman, programs);

	return postman;
}

void character_cards_init(struct postman *postman)
{
	struct character *current_character;

	// Count how many cards we need across all the characters.
	int i;
	postman->cards_count = 0;
	for (i = 0; i < postman->characters_count; i++)
	{
		current_character = &postman->characters[i];
		postman->cards_count += current_character->cards_count;
	}

	postman->cards = malloc(postman->cards_count * sizeof(struct card));

	// Setup all the cards for each character.
	int card_index = 0;
	for (i = 0; i < postman->characters_count; i++)
	{
		current_character = &postman->characters[i];
		int character_card_index;
		for (character_card_index = 0; character_card_index < current_character->cards_count; character_card_index++)
		{
			struct card *current_card = &postman->cards[card_index];
			current_card->character = current_character;
			current_card->player = NULL;
			card_index++;
		}
	}
}

void players_init(struct postman *postman, char **programs)
{
	postman->players = malloc(postman->players_count * sizeof(struct player));

	int i;
	for (i = 0; i < postman->players_count; i++)
	{
		// Initialise the player.
		struct player *current_player = &postman->players[i];
		current_player->index = i;
		current_player->playing = 1;
		current_player->protected = 0;
		current_player->hand = malloc(2 * sizeof(*current_player->hand));
		current_player->hand[0] = current_player->hand[1] = NULL;

		// Fork off the player program, getting access to pid/stdin/stdout.
		current_player->pipexec = new_pipexec(programs[i]);

		// Get name of player as it output on stdout.
		char ai_name[31];
		fgets(ai_name, 31, current_player->pipexec->stdout);
		strtok(ai_name, "\n");
		current_player->name = malloc(strlen(ai_name) + 1);
		strcpy(current_player->name, ai_name);

		printf("Started player %s from %s\n", current_player->name, current_player->pipexec->program);
	}
}

struct pipexec *new_pipexec(char *program)
{
	// Setup pipes for hooking on stdin/stdout.
	struct pipexec *p = malloc(sizeof(struct pipexec));
	pid_t stdin_pipe[2], stdout_pipe[2];
	if (pipe(stdin_pipe) || pipe(stdout_pipe))
	{
		return NULL;
	}

	// Fork off, setup pipes, execute Player program and save file handles for usage.
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

void play_game(struct postman *postman)
{
	// Play game until cards run out.
	struct card *picked_card;
	int current_player_index = 0, initial_cards_drawn = 0;
	while ((picked_card = choose_card(postman)) != NULL)
	{
		// Check we have at least 2 players not out. End game if appropriate.
		int p, active_players_count = 0;
		for (p = 0; p < postman->players_count; p++)
		{
			if (postman->players[p].playing)
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

		postman->current_player = &postman->players[current_player_index];
		if (postman->current_player->playing)
		{
			postman->current_player->protected = 0;
			postman->cards_drawn++;

			player_draw(postman, picked_card);
			if (initial_cards_drawn && player_move(postman) == -1)
			{
				// @TODO: player did an invalid move, forfeit the game.
				printf("\n(player_move) Player %s did an invalid move.\n", postman->current_player->name);
				postman->current_player->playing = 0;
			}
		} else {
			#if DEBUG==1
				printf("\nPlayer %d is out.\n", postman->current_player->index);
			#endif
		}

		// This can be done in one line except detection of when initial cards
		// have been distributed to all players is easiest here.
		current_player_index++;
		if (current_player_index >= postman->players_count) {
			initial_cards_drawn = 1;
			current_player_index = current_player_index % postman->players_count;
		}
	}
}

struct card *choose_card(struct postman *postman)
{
	struct card *chosen_card = NULL;
	// If we've drawn all our cards, return NULL.
	if (postman->cards_drawn < postman->cards_count)
	{
		// Loop around until we pick a card that hasn't been drawn already.
		while (1)
		{
			int chosen_card_index = rand() % postman->cards_count;
			chosen_card = &postman->cards[chosen_card_index];
			if (chosen_card->player == NULL)
			{
				break;
			}
		}
	}
	return chosen_card;
}

void player_draw(struct postman *postman, struct card *current_card)
{
	// Assign the drawn card to the player. Update their hand.
	current_card->player = postman->current_player;
	if (postman->current_player->hand[0] == NULL) {
		postman->current_player->hand[0] = current_card;
	} else {
		postman->current_player->hand[1] = current_card;
	}

	#if DEBUG==1
		printf("\nplayer %d\n", postman->current_player->index);
		printf("draw %s\n", current_card->character->name);
	#endif

	// Tell all players whose turn it is, and tell the current player what card they
	// have drawn.
	int p;
	for (p = 0; p < postman->players_count; p++)
	{
		if (postman->players[p].playing)
		{
			fprintf(postman->players[p].pipexec->stdin, "player %d\n", postman->current_player->index);
		}
	}
	fprintf(postman->current_player->pipexec->stdin, "draw %s\n", current_card->character->name);
	fflush(postman->current_player->pipexec->stdin);
}

int player_move(struct postman *postman)
{
	int valid = -1;

	// Now we have drawn the player a card on their turn, get player move from stdout.
	char ai_move[31];
	fgets(ai_move, 31, postman->current_player->pipexec->stdout);
	#if DEBUG==1
		printf("%s", ai_move);
	#endif
	strtok(ai_move, "\n ");

	// Forfeit if chosen.
	if (strcmp(ai_move, "forfeit") == 0) {
		valid = 1;
		forfeit_player(postman, postman->current_player);
	}

	// Parse out play action if chosen.
	if (strcmp(ai_move, "play") == 0) {
		valid = 1;

		// Get played character name.
		// @TODO: Need to send fellow players the additional information about a
		// played move, not just the character name.
		// @TODO: Stop using ai_move_location raw, we wrap into character below.
		char *ai_move_location = ai_move + 5;
		#if DEBUG==1
			printf("played %s\n", ai_move_location);
		#endif
		int p;
		for (p = 0; p < postman->players_count; p++)
		{
			if (postman->players[p].playing)
			{
				fprintf(postman->players[p].pipexec->stdin, "played %s\n", ai_move_location);
			}
		}

		// Get character struct of chosen character name. Forfeit if invalid.
		strtok(ai_move_location, "\n ");
		struct character *played_character;
		if ((played_character = player_played_character(postman, ai_move_location)) == NULL)
		{
			// @TODO: player did an invalid move, forfeit the game.
			#if DEBUG==1
				printf("(player_played_character) Player %s did an invalid move.\n", postman->current_player->name);
			#endif
			forfeit_player(postman, postman->current_player);
			return -1;
		}

		struct player *target_player = NULL;
		struct character *target_character = NULL;
		// If a character was played that has a target player, get the target player.
		if (played_character->score < 7 && played_character->score > 0
			&& played_character->score != 4)
		{
			ai_move_location += strlen(played_character->name) + 1;
			strtok(ai_move_location, "\n ");

			target_player = player_targeted_player(postman, ai_move_location);
			if (target_player == NULL)
			{
				#if DEBUG==1
					printf("(player_targeted_player) Player %s specified an invalid target player.\n", postman->current_player->name);
				#endif
				forfeit_player(postman, postman->current_player);
				return -1;
			}

			// If a character was played that has a target character as well, get the
			// target character.
			if (played_character->score == 1)
			{
				// @TODO: stop assuming numbers lack leading zeros, explicitly use how far
				// player_targeted_player parsed.
				int target_player_index_char_length = (target_player->index / 10) + 1;
				ai_move_location += target_player_index_char_length + 1;
				strtok(ai_move_location, "\n ");

				target_character = player_targeted_character(postman, ai_move_location);
				if (target_character == NULL)
				{
					#if DEBUG==1
						printf("(player_targeted_character) Player %s specified an invalid target character.\n", postman->current_player->name);
					#endif
					forfeit_player(postman, postman->current_player);
					return -1;
				}
			}
		}

		// Apply appropriate card actions.
		// @TODO: find cleaner way to handle this. It seems easiest to have a void
		// pointer on character structs that points to a handling function. We can
		// intelligently pick the exact signature to call it with when parsing the
		// play message.
		switch (played_character->score)
		{
		case 8:
			// Princess
			played_princess(postman);
			break;
		case 7:
			// Minister
			// @TODO: effect of minister takes place on draw rather than play
			break;
		case 6:
			// General
			played_general(postman, target_player);
			break;
		case 5:
			// Wizard
			played_wizard(postman, target_player);
			break;
		case 4:
			// Priestess
			played_priestess(postman);
			break;
		case 3:
			// Knight
			played_knight(postman, target_player);
			break;
		case 2:
			// Clown
			played_clown(postman, target_player);
			break;
		case 1:
			// Soldier
			played_soldier(postman, target_player, target_character);
			break;
		}
	}

	return valid;
}

struct character *player_played_character(struct postman *postman, char *character_name)
{
	// If player picks a character we don't think is in their hand, return NULL.
	// Otherwise update their hand to just contain the unplayed card.
	struct character *c = NULL;
	if (postman->current_player->hand[0] != NULL && strcmp(postman->current_player->hand[0]->character->name, character_name) == 0)
	{
		c = postman->current_player->hand[0]->character;
		postman->current_player->hand[0] = postman->current_player->hand[1];
		postman->current_player->hand[1] = NULL;
	} else if (postman->current_player->hand[1] != NULL && strcmp(postman->current_player->hand[1]->character->name, character_name) == 0)
	{
		c = postman->current_player->hand[1]->character;
		postman->current_player->hand[1] = NULL;
	}
	return c;
}

struct player *player_targeted_player(struct postman *postman, char *player_index_chars)
{
	// Parse out the player index to target, return that player.
	struct player *p = NULL;
	int player_index = (*player_index_chars - '0');
	if (player_index >= 0 && player_index < 4) {
		p = &postman->players[player_index];
	}
	return p;
}

struct character *player_targeted_character(struct postman *postman, char *character_name)
{
	// Find the targeted character (only relevant for Soldier).
	struct character *c = NULL;
	int i;
	for (i = 0; i < postman->characters_count; i++)
	{
		if (strcmp(postman->characters[i].name, character_name) == 0)
		{
			c = &postman->characters[i];
			break;
		}
	}
	return c;
}

void forfeit_player(struct postman *postman, struct player *target_player)
{
	#if DEBUG==1
		printf("out %d %s\n", target_player->index, target_player->hand[0]->character->name);
	#endif

	// Tell all playing players this one is out, then mark as not playing.
	int p;
	for (p = 0; p < postman->players_count; p++)
	{
		if (postman->players[p].playing)
		{
			fprintf(postman->players[p].pipexec->stdin, "out %d %s\n", target_player->index, target_player->hand[0]->character->name);
		}
	}
	target_player->playing = 0;
}

void played_princess(struct postman *postman)
{
	forfeit_player(postman, postman->current_player);
}

void played_general(struct postman *postman, struct player *target_player)
{
	if (target_player->playing && !target_player->protected)
	{
		fprintf(postman->current_player->pipexec->stdin, "swap %s\n", target_player->hand[0]->character->name);
		fprintf(target_player->pipexec->stdin, "swap %s\n", postman->current_player->hand[0]->character->name);
		struct card *temp_card = postman->current_player->hand[0];
		postman->current_player->hand[0] = target_player->hand[0];
		target_player->hand[0] = temp_card;
	} else {
		// @TODO: handle case where target player is out or has played a
		// Priestess. What do we send to the players to describe this
		// situation?
	}
}

void played_wizard(struct postman *postman, struct player *target_player)
{
	// @TODO: target_player discards hand, draws new card
	// Run `discard` for the card in their hand. `out` describes the
	// cards in a player hand as well.
	// @TODO: Tidy drawing new card into routine that doesn't need 15
	// lines of code here.
	//fprintf(target_player->stdin, "discard %s\n", target_player->hand[0]->character->name);
	//fprintf(target_player->stdin, "draw %s\n", );
}

void played_priestess(struct postman *postman)
{
	// We unprotect a player each time their turn comes up.
	postman->current_player->protected = 1;
}

void played_knight(struct postman *postman, struct player *target_player)
{
	// @TODO: Internally compare value of card in each player's hand,
	// `out` iff one player scores lower than other.
	int current_score = postman->current_player->hand[0]->character->score;
	int target_score = target_player->hand[0]->character->score;
	struct player *out_player = NULL;
	if (current_score < target_score)
	{
		out_player = postman->current_player;
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
		for (p = 0; p < postman->players_count; p++)
		{
			if (postman->players[p].playing)
			{
				fprintf(postman->players[p].pipexec->stdin, "out %d %s\n", out_player->index, out_player->hand[0]->character->name);
			}
		}
	}
}

void played_clown(struct postman *postman, struct player *target_player)
{
	// @TODO: reveal card in target_player hand to current_player only.
	fprintf(postman->current_player->pipexec->stdin, "reveal %s\n", target_player->hand[0]->character->name);
}

void played_soldier(struct postman *postman, struct player *target_player, struct character *target_character)
{
	// @TODO: check if target_player has target_character card. If they
	// do have the card, `out`.
	if (target_player->hand[0]->character == target_character)
	{
		target_player->playing = 0;
		#if DEBUG==1
			printf("out %d %s\n", target_player->index, target_player->hand[0]->character->name);
		#endif
		int p;
		for (p = 0; p < postman->players_count; p++)
		{
			if (postman->players[p].playing)
			{
				fprintf(postman->players[p].pipexec->stdin, "out %d %s\n", target_player->index, target_player->hand[0]->character->name);
			}
		}
	}
}

void score_game(struct postman *postman)
{
	// Once cards run out determine the winning player.
	int i, top_score = 0;
	struct card *top_card = NULL;
	for (i = 0; i < postman->players_count; i++)
	{
		postman->current_player = &postman->players[i];
		int current_score = postman->current_player->hand[0]->character->score;
		if (postman->current_player->playing && current_score > top_score)
		{
			top_score = current_score;
			top_card = postman->current_player->hand[0];
		}
	}
	printf("Player %d %s won with a %s card.\n", top_card->player->index, top_card->player->name, top_card->character->name);
}

void cleanup_game(struct postman *postman)
{
	int i;
	for (i = 0; i < postman->players_count; i++)
	{
		postman->current_player = &postman->players[i];

		// @TODO: Do we need to kill the pipexec->stdin_pipe and pipexec->stdout_pipe?
		kill(postman->current_player->pipexec->pid, SIGTERM);
		fclose(postman->current_player->pipexec->stdin);
		fclose(postman->current_player->pipexec->stdout);
		free(postman->current_player->pipexec);

		free(postman->current_player->hand);
		free(postman->current_player->name);
	}
	free(postman->players);

	free(postman->characters);
	free(postman->cards);
	free(postman);
}
