#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../include/LineParser.h"

char containsDebugFlag(int argc, char const *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
            return 1;
    }
    return 0;
}

void execute(cmdLine *pCmdLine)
{
    if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
    {
        perror("Failed command execution");
        exit(1);
    }
}

int main(int argc, char const *argv[])
{
    char debug = containsDebugFlag(argc, argv);

    while (1)
    {
        char buffer[PATH_MAX], input[2048];
        getcwd(buffer, PATH_MAX);
        printf("~%s$ ", buffer);
        fgets(input, 2048, stdin);

        if (strcmp(input, "quit\n") == 0)
            break;
        if (strcmp(input, "\n") == 0)
            continue;

        cmdLine *cmd;
        cmd = parseCmdLines(input);

        if (debug == 1)
            printf("Executing: %s", input);

        if (strcmp(cmd->arguments[0], "cd") == 0)
        {
            if (chdir(cmd->arguments[1]) == -1)
                perror("cd failed");
            continue;
        }

        if (strcmp(cmd->arguments[0], "suspend") == 0)
        {
            int toStop = atoi(cmd->arguments[1]);
            kill(toStop, SIGTSTP);
            usleep(10000);
            continue;
        }

        if (strcmp(cmd->arguments[0], "wake") == 0)
        {
            int toContinue = atoi(cmd->arguments[1]);
            kill(toContinue, SIGCONT);
            usleep(10000);
            continue;
        }

        if (strcmp(cmd->arguments[0], "kill") == 0)
        {
            int toTerminate = atoi(cmd->arguments[1]);
            kill(toTerminate, SIGINT);
            usleep(10000);
            continue;
        }

        int fd[2];
        int needToPipe = 0;
        if (cmd->next != NULL)
        { /*PIPELINE!*/
            needToPipe = 1;
            if (pipe(fd) == -1)
            {
                perror("Piping unsuccessful");
                return 1;
            }
        }

        /*Create child*/
        pid_t pid1 = fork();

        if (pid1 == 0)
        { /*Child 1 process*/
            if (needToPipe)
            {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            if (cmd->inputRedirect)
            {
                freopen(cmd->inputRedirect, "r", stdin);
            }
            if (cmd->outputRedirect)
            {
                freopen(cmd->outputRedirect, "w", stdout);
            }
            execute(cmd);
        }
        else
        { /*Main process*/
            if (cmd->blocking == 1)
            {
                waitpid(pid1, NULL, WUNTRACED);
            }
            else
                usleep(10000);

            if (debug == 1)
            {
                printf("Child PID1: %d\n", pid1);
            }
           
            if (needToPipe)
            {
                pid_t pid2 = fork();
                if (pid2 != 0)
                { /*Main process*/
                    if (debug == 1)
                    {
                        printf("Child PID2: %d\n", pid2);
                    }
                    close(fd[1]);
                    close(fd[0]);
                    waitpid(pid1, NULL, 0);
                    waitpid(pid2, NULL, 0);
                    freeCmdLines(cmd);
                    usleep(10000);
                }
                else
                { /*Child 2 process*/
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[1]);
                    close(fd[0]);
                    if (cmd->next->inputRedirect)
                    {
                        freopen(cmd->next->inputRedirect, "r", stdin);
                    }
                    if (cmd->next->outputRedirect)
                    {
                        freopen(cmd->next->outputRedirect, "w", stdout);
                    }
                    execute(cmd->next);
                }
            }

        }
    }
    return 0;
}
