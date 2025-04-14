#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMANDS 10

char *history[MAX_COMMANDS];
int history_count = 0;

void add_to_history(char *command)
{
    if (history_count < MAX_COMMANDS)
    {
        history[history_count] = strdup(command);
        history_count++;
    }
    else
    {
        // Discard the first command in the history array
        free(history[0]);

        // Shift all entries to the left one place
        for (int i = 0; i < MAX_COMMANDS - 1; i++)
        {
            history[i] = history[i + 1];
        }

        history[MAX_COMMANDS - 1] = strdup(command);
    }
}

void print_history()
{
    for (int i = 0; i < history_count; i++)
    {
        printf("%d: %s\n", i + 1, history[i]);
    }
}