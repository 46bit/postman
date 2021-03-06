#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

struct character {
	int score;
	char *name;
	int cards;
};

int main()
{
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

	int idented = 0, player_id, first_player_id, players_count = 4;

	struct timeval time;
	gettimeofday(&time, NULL);
	long microsec = ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
	srand(microsec);

	char play[31];
	int has_been_drawn = 0, discarding = 0;
	char *hand[2];

	while (fgets(play, 31, stdin))
	{
		strtok(play, " \n");

		if (strcmp(play, "ident") == 0)
		{
			strtok(play+6, "\n ");
			player_id = *(play+6) - '0';
			fprintf(stderr, "ident %d\n", player_id);

			fprintf(stderr, "Random Player %d seed is %lo\n", player_id, microsec);
			printf("Hello World\n");
			fflush(stdout);
		}

		if (strcmp(play, "players") == 0)
		{
			strtok(play+8, "\n ");
			players_count = *(play+8) - '0';
			fprintf(stderr, "players %d\n", players_count);
		}

		if (strcmp(play, "start") == 0)
		{
			strtok(play+6, "\n ");
			first_player_id = *(play+6) - '0';
			fprintf(stderr, "start %d\n", first_player_id);
			idented = 1;
		}

		if (!idented) continue;

		if (strcmp(play, "draw") == 0 || strcmp(play, "swap") == 0)
		{
			if (has_been_drawn == 0)
			{
				strtok(play+5, " \n");
				char *item = malloc(strlen(play+5) + 1);
				strcpy(item, play+5);
				hand[0] = item;
				has_been_drawn++;
				continue;
			}

			strtok(play+5, " \n");
			char *item = malloc(strlen(play+5) + 1);
			strcpy(item, play+5);
			hand[1] = item;
			int which = rand() % 2;

			if (!discarding && strcmp(play, "draw") == 0) {
				char *character_name = hand[which];
				if (strcmp(character_name, "Princess") == 0
					|| strcmp(character_name, "Minister") == 0
					|| strcmp(character_name, "Priestess") == 0) {
					printf("play %s\n", character_name);
				} else {
					int target_player_index = rand() % players_count;
					if (strcmp(character_name, "Soldier") != 0) {
						printf("play %s %d\n", character_name, target_player_index);
					} else {
						char *target_character_name = characters[rand() % characters_count].name;
						printf("play %s %d %s\n", character_name, target_player_index, target_character_name);
					}
				}
			} else {
				which = 0;
			}

			fflush(stdout);
			if (which == 0)
			{
				free(hand[0]);
				hand[0] = hand[1];
			} else {
				free(hand[1]);
			}
			hand[1] = NULL;
			discarding = 0;
		}

		if (strcmp(play, "discard") == 0)
		{
			strtok(play+8, "\n ");
			int discard_player_id = *(play+8) - '0';
			if (player_id == discard_player_id)
			{
				discarding = 1;
			}
		}
	}

	return 0;
}
