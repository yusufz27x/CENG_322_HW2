#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "history.h"

#define MAX_COMMAND_LENGTH 100

int main()
{
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_COMMAND_LENGTH];
    char *token;
    bool background;
    int and_pos;
    int pipe_pos;

    while (true)
    {
        // Reset values
        background = false;
        and_pos = -1;
        pipe_pos = -1;

        printf("myshell> ");

        // Get input
        fgets(input, MAX_COMMAND_LENGTH, stdin);

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        add_to_history(input);

        // Tokenize the command into the args array
        token = strtok(input, " ");
        int i = 0;
        while (token != NULL)
        {
            args[i] = token;

            // Check for logical AND and pipe operator
            if (strcmp(args[i], "&&") == 0)
            {
                and_pos = i;
            }
            if (strcmp(args[i], "|") == 0)
            {
                pipe_pos = i;
            }

            token = strtok(NULL, " ");
            i++;
        }

        args[i] = NULL;

        // cd command
        if (strcmp(args[0], "cd") == 0)
        {
            // cd to the home folder if there is no argument
            if (args[1] == NULL)
            {
                chdir(getenv("HOME"));
            }

            else
            {
                chdir(args[1]);
            }

            // Update environment variable for the working directory
            setenv("PWD", getcwd(NULL, 0), 1);
        }

        // pwd command
        else if (strcmp(args[0], "pwd") == 0)
        {
            printf("%s\n", getcwd(NULL, 0));
        }

        // history command
        else if (strcmp(args[0], "history") == 0)
        {
            print_history();
        }

        // exit command
        else if (strcmp(args[0], "exit") == 0)
        {
            exit(0);
        }

        // Custom command support
        else
        {
            // Background task check
            if (args[i - 1] != NULL && strcmp(args[i - 1], "&") == 0)
            {
                background = true;
                args[i - 1] = NULL;
            }

            // Logical AND operator support
            if (and_pos != -1)
            {
                args[and_pos] = NULL;

                // First command
                pid_t pid = fork();

                // Child process
                if (pid == 0)
                {
                    if (execvp(args[0], args) == -1)
                    {
                        perror("Error executing command");
                    }
                    exit(1);
                }

                else if (pid < 0)
                {
                    perror("Error forking process");
                }

                // Parent process
                else
                {
                    int status;
                    waitpid(pid, &status, 0);

                    // Check whether the first command exited successfully
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                    {
                        // Second command
                        pid_t pid2 = fork();

                        // Child process
                        if (pid2 == 0)
                        {
                            if (execvp(args[and_pos + 1], &args[and_pos + 1]) == -1)
                            {
                                perror("Error executing command");
                            }
                            exit(1);
                        }

                        else if (pid2 < 0)
                        {
                            perror("Error forking process");
                        }

                        // Parent process
                        else
                        {
                            waitpid(pid2, NULL, 0);
                        }
                    }
                }
            }

            // Pipe operator support
            else if (pipe_pos != -1)
            {
                args[pipe_pos] = NULL;

                // Initialize communication pipe
                int pipefd[2];
                pipe(pipefd);

                // First command
                pid_t pid = fork();

                // Child process
                if (pid == 0)
                {
                    // Redirect stdout to write pipe
                    dup2(pipefd[1], STDOUT_FILENO);

                    // Close pipe
                    close(pipefd[0]);
                    close(pipefd[1]);

                    if (execvp(args[0], args) == -1)
                    {
                        perror("Error executing command");
                    }
                    exit(1);
                }

                else if (pid < 0)
                {
                    perror("Error forking process");
                }

                // Parent process
                else
                {
                    // Close write pipe
                    close(pipefd[1]);

                    waitpid(pid, NULL, 0);

                    // Second command
                    pid_t pid2 = fork();

                    // Child process
                    if (pid2 == 0)
                    {
                        // Redirect stdin to read pipe
                        dup2(pipefd[0], STDIN_FILENO);

                        // Close pipe
                        close(pipefd[0]);
                        close(pipefd[1]);

                        if (execvp(args[pipe_pos + 1], &args[pipe_pos + 1]) == -1)
                        {
                            perror("Error executing command");
                        }
                        exit(1);
                    }

                    else if (pid2 < 0)
                    {
                        perror("Error forking process");
                    }

                    // Parent process
                    else
                    {
                        // Close read pipe
                        close(pipefd[0]);

                        waitpid(pid2, NULL, 0);
                    }
                }
            }

            else
            {
                pid_t pid = fork();

                // Child process
                if (pid == 0)
                {
                    if (execvp(args[0], args) == -1)
                    {
                        perror("Error executing command");
                    }
                    exit(1);
                }

                else if (pid < 0)
                {
                    perror("Error forking process");
                }

                // Parent process
                else
                {
                    if (!background)
                    {
                        waitpid(pid, NULL, 0);
                    }
                    else
                    {
                        printf("Background process with PID: %d\n", pid);
                    }
                }
            }
        }
    }
    return 0;
}