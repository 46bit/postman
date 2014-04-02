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

	struct timeval time;
	gettimeofday(&time, NULL);
	long microsec = ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
	srand(microsec);

	printf("Hello World\n");
	fflush(stdout);
	char play[31];
	int has_been_drawn = 0;
	char *hand[2];
	while (fgets(play, 31, stdin))
	{
		strtok(play, "\n");
		strtok(play, " ");

		if (strcmp(play, "draw") == 0 || strcmp(play, "swap") == 0)
		{
			if (has_been_drawn == 0)
			{
				char *item = malloc(strlen(play+5) + 1);
				strcpy(item, play+5);
				hand[0] = item;
				has_been_drawn++;
				continue;
			}

			char *item = malloc(strlen(play+5) + 1);
			strcpy(item, play+5);
			hand[1] = item;
			int which = rand() % 2;

			if (strcmp(play, "draw") == 0) {
				char *character_name = hand[which];
				if (strcmp(character_name, "Princess") == 0
					|| strcmp(character_name, "Minister") == 0
					|| strcmp(character_name, "Priestess") == 0) {
					fprintf(stderr, "play %s\n", character_name);
					printf("play %s\n", character_name);
				} else {
					int target_player_index = rand() % 4;
					if (strcmp(character_name, "Soldier") != 0) {
						fprintf(stderr, "play %s %d\n", character_name, target_player_index);
						printf("play %s %d\n", character_name, target_player_index);
					} else {
						char *target_character_name = characters[rand() % characters_count].name;
						fprintf(stderr, "play %s %d %s\n", character_name, target_player_index, target_character_name);
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
		}

		if (strcmp(play, "end_card") == 0)
		{
			printf("end_card %s\n", hand[0]);
			fflush(stdout);
		}
	}

	return 0;
}
