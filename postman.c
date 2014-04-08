#include "postman.h"

int main(int argc, char *argv[])
{
	// Seed rand from microtime.
	struct timeval time;
	gettimeofday(&time, NULL);
	long microsec = ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
	srand(microsec);
	#if DEBUG==1
		fprintf(stderr, "Postman seed is %lo\n", microsec);
	#endif

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
	postman->characters[0] = (struct character) {8, "Princess", 1, played_princess, 0};
	postman->characters[1] = (struct character) {7, "Minister", 1, NULL, 0};
	postman->characters[2] = (struct character) {6, "General", 1, played_general, PLAY_PARSE_TARGET_PLAYER};
	postman->characters[3] = (struct character) {5, "Wizard", 2, played_wizard, PLAY_PARSE_TARGET_PLAYER};
	postman->characters[4] = (struct character) {4, "Priestess", 2, played_priestess, 0};
	postman->characters[5] = (struct character) {3, "Knight", 2, played_knight, PLAY_PARSE_TARGET_PLAYER};
	postman->characters[6] = (struct character) {2, "Clown", 2, played_clown, PLAY_PARSE_TARGET_PLAYER};
	postman->characters[7] = (struct character) {1, "Soldier", 5, played_soldier, PLAY_PARSE_TARGET_PLAYER | PLAY_PARSE_TARGET_CHARACTER};

	// Setup game cards from characters
	postman->cards_drawn = 0;
	character_cards_init(postman);

	// Setup game players as provided in argv
	postman->players_count = players_count;
	postman->first_player_index = rand() % postman->players_count;
	players_init(postman, programs);

	postman->current_move = malloc(sizeof(struct move));
	*postman->current_move = (struct move) {NULL, NULL, NULL};

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

		// Tell player its player ID, the ID of the player to go first, and how many
		// players are in this game. `ident` message prompts for reply of player name.
		tell_player(current_player, "ident %d %d %d\n", i, postman->first_player_index, postman->players_count);

		// Get name of player as it output on stdout.
		current_player->name = receive_player(current_player, 30);
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
	int current_player_index = postman->first_player_index, initial_cards_drawn = 0;

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
				printf("End of game, only %d players remaining.\n", active_players_count);
			#endif
			break;
		}

		postman->current_player = &postman->players[current_player_index];
		if (postman->current_player->playing)
		{
			postman->current_player->protected = 0;
			*postman->current_move = (struct move) {NULL, NULL, NULL};
			postman->cards_drawn++;

			// For initial drawing we're not going to issue `player %d` messages.
			// We'll just send the `draw %s` messages and leave it at that.
			if (initial_cards_drawn)
			{
				player_turn(postman);
			}
			player_draw(postman, postman->current_player, picked_card);
			// player_draw can disqualify players with or who get the Minister.
			// Hence the need to recheck if playing.
			if (initial_cards_drawn && postman->current_player->playing)
			{
				player_move(postman);
			}
		}

		// This can be done in one line except detection of when initial cards
		// have been distributed to all players is easiest here.
		current_player_index = (current_player_index + 1) % postman->players_count;
		if (postman->cards_drawn == postman->players_count) {
			initial_cards_drawn = 1;
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

void player_turn(struct postman *postman)
{
	// Tell all players whose turn it is.
	tell_all(postman, "player %d\n", postman->current_player->index);
}

void player_draw(struct postman *postman, struct player *player, struct card *current_card)
{
	// Assign the drawn card to the player. Update their hand.
	current_card->player = player;

	int i;
	for (i = 0; i < 2; i++)
	{
		if (player->hand[i] == NULL)
		{
			player->hand[i] = current_card;
			break;
		}
	}

	// Check Minister hasn't disqualified player.
	if (strcmp(current_card->character->name, "Minister") == 0)
	{
		int score_sum = 0;
		for (i = 0; i < 2; i++)
		{
			if (player->hand[i] != NULL)
			{
				score_sum += player->hand[i]->character->score;
			}
		}
		if (score_sum > 12)
		{
			forfeit_player(postman, player);
		}
	}

	// Tell the current player what card they have drawn.
	tell_player(player, "draw %s\n", current_card->character->name);
}

void player_move(struct postman *postman)
{
	// Now we have drawn the player a card on their turn, get player move from stdout.
	char *ai_move = receive_player(postman->current_player, 30);
	strtok(ai_move, " ");

	// Forfeit if chosen.
	if (strcmp(ai_move, "forfeit") == 0) {
		forfeit_player(postman, postman->current_player);
	}

	// Parse out play action if chosen.
	if (strcmp(ai_move, "play") == 0) {
		// Get location in string after play token and null byte.
		char *ai_move_location = ai_move + 5;

		// Get chosen character.
		strtok(ai_move_location, "\n ");
		postman->current_move->played_character = play_get_character(postman, &ai_move_location);

		// Check the chosen character is in the player's hand.
		if (postman->current_move->played_character != NULL && remove_character_from_hand(postman->current_player, postman->current_move->played_character) == 0)
		{
			// Parse according to character's play_fieldmask.
			int parse_status = parse_play(postman, ai_move_location, postman->current_move->played_character->play_fieldmask);
			if (parse_status == 0)
			{
				// Output `played %d %s %s`-type messages to all players according to
				// fields parsed into postman->current_move.
				print_play(postman);

				// Run character callback for all those with one (all except Minister).
				if (postman->current_move->played_character->play_handler != NULL)
				{
					postman->current_move->played_character->play_handler(postman);
				}
			}
		}
	}

	free(ai_move);

	// If execution passes to here we have failed to parse the message. This is fine
	// if it is a message we don't care about, but a misformed `play` message might
	// lead to deadlock waiting for a non-malformed message.
	// @TODO: Implement watchdog timer to limit time players have to decide move.
}

int remove_character_from_hand(struct player *player, struct character *character)
{
	struct card **hand = player->hand;

	if (hand[0] != NULL && hand[0]->character == character)
	{
		hand[0] = hand[1];
		hand[1] = NULL;
		return 0;
	}

	if (hand[1] != NULL && hand[1]->character == character)
	{
		hand[1] = NULL;
		return 0;
	}

	return 1;
}

struct player *parse_player_from_index(struct postman *postman, char *player_index_chars)
{
	// Parse out the player index to target, return that player.
	struct player *p = NULL;
	int player_index = (*player_index_chars - '0');
	if (player_index >= 0 && player_index < 4) {
		p = &postman->players[player_index];
	}
	return p;
}

struct character *parse_character_from_name(struct postman *postman, char *character_name)
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

struct player *play_get_player(struct postman *postman, char **arguments)
{
	strtok(*arguments, "\n ");

	struct player *player = parse_player_from_index(postman, *arguments);
	if (player == NULL)
	{
		#if DEBUG==1
			printf("Player %s specified an invalid player.\n", postman->current_player->name);
		#endif
		forfeit_player(postman, postman->current_player);
	}
	else
	{
		*arguments += 2;
	}
	return player;
}

struct character *play_get_character(struct postman *postman, char **arguments)
{
	strtok(*arguments, "\n ");

	struct character *character = parse_character_from_name(postman, *arguments);
	if (character == NULL)
	{
		#if DEBUG==1
			printf("Player %s specified an invalid character.\n", postman->current_player->name);
		#endif
		forfeit_player(postman, postman->current_player);
	}
	else
	{
		*arguments += strlen(character->name) + 1;
	}
	return character;
}

int parse_play(struct postman *postman, char *arguments, int flags)
{
	if (flags & PLAY_PARSE_TARGET_PLAYER)
	{
		postman->current_move->target_player = play_get_player(postman, &arguments);
		if (postman->current_move->target_player == NULL)
		{
			return 1;
		}
	}
	if (flags & PLAY_PARSE_TARGET_CHARACTER)
	{
		postman->current_move->target_character = play_get_character(postman, &arguments);
		if (postman->current_move->target_character == NULL)
		{
			return 1;
		}
	}
	return 0;
}

void print_play(struct postman *postman)
{
	if (postman->current_move->target_player == NULL)
	{
		tell_all(postman, "played %d %s\n", postman->current_player->index, postman->current_move->played_character->name);
		return;
	}
	if (postman->current_move->target_character == NULL)
	{
		tell_all(postman, "played %d %s %d\n", postman->current_player->index, postman->current_move->played_character->name, postman->current_move->target_player->index);
		return;
	}
	tell_all(postman, "played %d %s %d %s\n", postman->current_player->index, postman->current_move->played_character->name, postman->current_move->target_player->index, postman->current_move->target_character->name);
}

void forfeit_player(struct postman *postman, struct player *target_player)
{
	// Tell all playing players this one is out, then mark as not playing.
	if (target_player->hand[0] == NULL)
	{
		// If the player has no cards in hand.
		tell_all(postman, "out %d\n", target_player->index);
	}
	else
	{
		if (target_player->hand[1] == NULL)
		{
			// If the player only has one card in hand.
			tell_all(postman, "out %d %s\n", target_player->index, target_player->hand[0]->character->name);
		}
		else
		{
			// If the player has two cards in hand.
			tell_all(postman, "out %d %s %s\n", target_player->index, target_player->hand[0]->character->name, target_player->hand[1]->character->name);
		}
	}
	target_player->playing = 0;
}

void tell_all_player_was_princessed(struct postman *postman, struct player *target_player)
{
	tell_all(postman, "protected %d\n", target_player->index);
}

void tell(FILE *target_pipe, char *message)
{
	fprintf(target_pipe, "%s", message);
	fflush(target_pipe);
}

void tell_all(struct postman *postman, const char *format, ...)
{
	char message[100];
	va_list arg;
	va_start(arg, format);
	vsnprintf(message, 100, format, arg);
	va_end(arg);

	#if DEBUG==1
		printf("postman->all: %s", message);
		fflush(stdout);
	#endif

	int p;
	for (p = 0; p < postman->players_count; p++)
	{
		if (postman->players[p].playing)
		{
			tell(postman->players[p].pipexec->stdin, message);
		}
	}
}

void tell_player(struct player *player, const char *format, ...)
{
	char message[100];
	va_list arg;
	va_start(arg, format);
	vsnprintf(message, 100, format, arg);
	va_end(arg);

	#if DEBUG==1
		printf("postman->%d: %s", player->index, message);
		fflush(stdout);
	#endif

	tell(player->pipexec->stdin, message);
}

char *receive_player(struct player *player, int length)
{
	char *message = malloc(length + 1);
	fgets(message, length + 1, player->pipexec->stdout);
	// @TODO: detect read errors for when AIs crash?
	strtok(message, "\n");
	#if DEBUG==1
		printf("%d->postman: %s\n", player->index, message);
		fflush(stdout);
	#endif
	return message;
}

void played_princess(struct postman *postman)
{
	forfeit_player(postman, postman->current_player);
}

void played_general(struct postman *postman)
{
	struct player *target_player = postman->current_move->target_player;

	if (target_player->playing && !target_player->protected)
	{
		tell_player(postman->current_player, "swap %s\n", target_player->hand[0]->character->name);
		tell_player(target_player, "swap %s\n", postman->current_player->hand[0]->character->name);

		struct card *temp_card = postman->current_player->hand[0];
		postman->current_player->hand[0] = target_player->hand[0];
		target_player->hand[0] = temp_card;
	}
	else
	{
		tell_all_player_was_princessed(postman, target_player);
	}
}

void played_wizard(struct postman *postman)
{
	struct player *target_player = postman->current_move->target_player;

	if (target_player->playing && !target_player->protected)
	{
		// The target player must discard their hand and draw a new card.
		// Run `discard` for the card in their hand, revealing it to all.
		tell_all(postman, "discard %d %s\n", target_player->index, target_player->hand[0]->character->name);
		target_player->hand[0] = target_player->hand[1] = NULL;

		// Draw a new card for the target player.
		struct card *replacement_card = choose_card(postman);
		if (replacement_card == NULL)
		{
			// If the player is forced to discard when no cards remain, they're out.
			// Can happen when one player has had final turn and others have not.
			forfeit_player(postman, target_player);
		}
		else
		{
			postman->cards_drawn++;
			player_draw(postman, target_player, replacement_card);
		}
	}
	else
	{
		tell_all_player_was_princessed(postman, target_player);
	}
}

void played_priestess(struct postman *postman)
{
	// We unprotect a player each time their turn comes up.
	postman->current_player->protected = 1;
}

void played_knight(struct postman *postman)
{
	struct player *target_player = postman->current_move->target_player;

	if (target_player->playing && !target_player->protected)
	{
		// Tell both players what the other player's card is. This happens regardless
		// of whether they or anyone wins.
		tell_player(postman->current_player, "reveal %d %s\n", target_player->index, target_player->hand[0]->character->name);
		tell_player(target_player, "reveal %d %s\n", postman->current_player->index, postman->current_player->hand[0]->character->name);

		// Internally compare value of card in each player's hand,
		// `out` iff one player scores lower than other.
		int current_score = postman->current_player->hand[0]->character->score;
		int target_score = target_player->hand[0]->character->score;
		struct player *out_player = NULL;
		if (current_score < target_score)
		{
			out_player = postman->current_player;
		}
		else if (target_score < current_score)
		{
			out_player = target_player;
		}

		if (out_player != NULL)
		{
			forfeit_player(postman, out_player);
		}
	}
	else
	{
		tell_all_player_was_princessed(postman, target_player);
	}
}

void played_clown(struct postman *postman)
{
	struct player *target_player = postman->current_move->target_player;

	if (target_player->playing && !target_player->protected)
	{
		// Reveal card in target_player hand to current_player only.
		tell_player(postman->current_player, "reveal %d %s\n", target_player->index, target_player->hand[0]->character->name);
	}
	else
	{
		tell_all_player_was_princessed(postman, target_player);
	}
}

void played_soldier(struct postman *postman)
{
	struct player *target_player = postman->current_move->target_player;
	struct character *target_character = postman->current_move->target_character;

	if (target_player->playing && !target_player->protected)
	{
		// Check if target_player has target_character card. If they
		// do have the card, `out`.
		if (target_player->hand[0]->character == target_character)
		{
			forfeit_player(postman, target_player);
		}
	}
	else
	{
		tell_all_player_was_princessed(postman, target_player);
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
		if (postman->current_player->playing)
		{
			int current_score = postman->current_player->hand[0]->character->score;
			if (current_score > top_score)
			{
				top_score = current_score;
				top_card = postman->current_player->hand[0];
			}
		}
	}
	printf("Player %d %s won with a %s card.\n", top_card->player->index, top_card->player->name, top_card->character->name);
}

void cleanup_game(struct postman *postman)
{
	// End and free players.
	int i;
	for (i = 0; i < postman->players_count; i++)
	{
		postman->current_player = &postman->players[i];

		kill(postman->current_player->pipexec->pid, SIGTERM);
		fclose(postman->current_player->pipexec->stdin);
		fclose(postman->current_player->pipexec->stdout);
		free(postman->current_player->pipexec);

		free(postman->current_player->hand);
		free(postman->current_player->name);
	}
	free(postman->players);

	free(postman->current_move);
	free(postman->characters);
	free(postman->cards);
	free(postman);
}
