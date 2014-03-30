#include "postman.h"

int main(int argc, char *argv[], char **envp)
{
	// Seed rand from microtime.
	struct timeval time;
	gettimeofday(&time, NULL);
	long microsec = ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
	srand(microsec);

	// Setup game characters and cards.
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
	struct player *players;
	int cards_drawn = 0, cards_length;
	cards_length = character_cards_init(8, characters, &cards, players);

	// Setup game players as provided in argv.
	int i, ai_count = argc - 1;
	players = malloc(ai_count * sizeof(struct player));

	for (i = 0; i < ai_count; i++)
	{
		pid_t ai_stdin_pipe[2], ai_stdout_pipe[2];
		if (pipe(ai_stdin_pipe) || pipe(ai_stdout_pipe))
		{
			printf("Pipes of AI %d failed.\n", i);
			return 1;
		}

		pid_t pid = fork();
		if (pid == 0)
		{
			dup2(ai_stdin_pipe[0], 0);
			dup2(ai_stdout_pipe[1], 1);
			execl(argv[i + 1], argv[i + 1], NULL);
		}
		players[i].pid = pid;
		players[i].stdin = fdopen(ai_stdin_pipe[1], "w");
		players[i].stdout = fdopen(ai_stdout_pipe[0], "r");
		players[i].hand = malloc(2);
		players[i].hand[0] = players[i].hand[1] = NULL;

		char ai_name[31];
		fgets(ai_name, 31, players[i].stdout);
		strtok(ai_name, "\n");
		players[i].name = malloc(sizeof(ai_name));
		strcpy(players[i].name, ai_name);
		printf("AI %d named itself %s\n", i, players[i].name);
	}

	// Play game until cards run out.
	struct card *picked_card;
	int current_player_index = 0, initial_cards_drawn = 0;
	struct player *current_player;
	while ((picked_card = choose_card(cards, &cards_drawn, cards_length)) != NULL)
	{
		current_player = &players[current_player_index];
		picked_card->player = current_player;
		if (current_player->hand[0] == NULL) {
			current_player->hand[0] = picked_card;
		} else
		{
			current_player->hand[1] = picked_card;
		}

		printf("player %d\n", current_player_index);
		fprintf(current_player->stdin, "player %d\n", current_player_index);
		printf("draw %s\n", picked_card->character->name);
		fprintf(current_player->stdin, "draw %s\n", picked_card->character->name);
		fflush(current_player->stdin);

		if (initial_cards_drawn) {
			char ai_move[31];
			fgets(ai_move, 31, current_player->stdout);
			strtok(ai_move, "\n");
			strtok(ai_move, " ");
			if (strcmp(ai_move, "forfeit") == 0) {
				printf("=> forfeited\n");
			}
			if (strcmp(ai_move, "play") == 0) {
				printf("=> played %s\n", ai_move+5);
				// find this card, remove from hand
				if (strcmp(current_player->hand[0]->character->name, ai_move+5) == 0)
				{
					current_player->hand[0] = picked_card->player->hand[1];
				} else if (strcmp(current_player->hand[1]->character->name, ai_move+5) == 0)
				{
					current_player->hand[1] = NULL;
				}
			}
		}

		// This can be done in one line except detection of when initial cards
		// have been distributed to all players is easiest here.
		current_player_index++;
		if (current_player_index >= ai_count) {
			initial_cards_drawn = 1;
			current_player_index = current_player_index % ai_count;
		}
	}

	// Once cards run out determine the winning player.
	int top_score = 0;
	struct card *top_card = NULL;
	for (i = 0; i < ai_count; i++)
	{
		struct player *current_player = &players[i];
		int current_score = current_player->hand[0]->character->score;
		if (current_score > top_score)
		{
			top_score = current_score;
			top_card = current_player->hand[0];
		}

		kill(players[i].pid, SIGTERM);
	}
	printf("Player %s won with a %s card.\n", top_card->player->name, top_card->character->name);

	return 0;
}

int character_cards_init(int characters_length, struct character *characters, struct card **cards, struct player *players)
{
	int c, d, o, cards_length = 0;
	for (c = 0; c < characters_length; c++)
	{
		cards_length += characters[c].cards;
	}
	int size = cards_length * sizeof(struct card);
	(*cards) = malloc(size);

	int card_index = 0;
	for (c = 0; c < characters_length; c++)
	{
		int character_card_index;
		for (character_card_index = 0; character_card_index < characters[c].cards; character_card_index++)
		{
			(*cards)[card_index].character = &characters[c];
			(*cards)[card_index].player = NULL;
			card_index++;
		}
	}

	return cards_length;
}

struct card *choose_card(struct card *cards, int *cards_drawn, int cards_length)
{
	struct card *chosen_card = NULL;
	if (*cards_drawn < cards_length)
	{
		while (1)
		{
			int chosen_card_index = rand() % cards_length;
			chosen_card = &(cards[chosen_card_index]);
			if (chosen_card->player == NULL)
			{
				(*cards_drawn)++;
				break;
			}
		}
	}
	return chosen_card;
}
