#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

int main(int argc, char *argv[], char **envp)
{
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

		if (strcmp(play, "draw") == 0)
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

			printf("play %s\n", hand[which]);
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
