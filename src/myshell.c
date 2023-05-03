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

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 20
#define INPUT_MAX 2048

typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is running the command*/
    int status;           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;

cmdLine * cmdSemiCopy(cmdLine *origin)
{
    cmdLine *copy = (cmdLine *)malloc(sizeof(cmdLine));
    char *prompt = malloc(32);
    ((char**)copy->arguments)[0] = strcpy(prompt, origin->arguments[0]);
    return copy;
}

process * makeProcess(cmdLine *recievedCmd, pid_t recievedPid)
{
    process *newProc = (struct process *)malloc(sizeof(struct process));
    newProc->cmd = cmdSemiCopy(recievedCmd);
    newProc->next = NULL;
    newProc->pid = recievedPid;
    newProc->status = RUNNING;
    return newProc;
}

void addProcess(process **process_list, cmdLine *recievedCmd, pid_t recievedPid)
{
    process *toAdd = makeProcess(recievedCmd, recievedPid);
    if (!(*process_list))
    {
        *process_list = toAdd;
        return;
    }

    process *current = *process_list;
    while (current->next)
    {
        current = current->next;
    }
        current->next = toAdd;
}

char *intToStatus(int status)
{
    char *statusString = malloc(20 * sizeof(char));
    switch (status)
    {
    case -1:
        strcpy(statusString, "Terminated");
        break;
    case 0:
        strcpy(statusString, "Suspended");
        break;
    case 1:
        strcpy(statusString, "Running");
        break;
    default:
        strcpy(statusString, "Unknown");
        break;
    }
    return statusString;
}

void removeProcessFromList(process **process_list, pid_t toRemove)
{
    if (!(*process_list))
    {
        return;
    }

    process *prev = *process_list, *current = (*process_list)->next;
    if (prev->pid == toRemove)
    {
        *process_list = current;
        return;
    }

    while (current)
    {
        process *next_process = current->next;
        if (current->pid == toRemove)
        {
            prev->next = next_process;
            break; /*don't worry, it is freed from outside this function!*/
        }
        prev = current;
        current = next_process;
    }
}

void freeProcessList(process **process_list)
{
    process *current = *process_list;
    while (current)
    {
        free(current);
        current = current->next;
    }
}

void updateProcessStatus(process **process_list, pid_t toChange, int new_status)
{
    process *current = *process_list;
    while (current)
    {
        if (current->pid == toChange)
        {
            current->status = new_status;
        }
        current = current->next;
    }
}

void updateProcessList(process **process_list)
{
    process *current = *process_list;
    while (current)
    {
        int currentStatus;
        int result = waitpid(current->pid, &currentStatus, WNOHANG | WUNTRACED);
        if (result == 0)
        {
            currentStatus = RUNNING;
        }
        else
        {
            if (WIFSTOPPED(currentStatus))
            {
                currentStatus = SUSPENDED;
            }
            else if (WIFCONTINUED(currentStatus))
            {
                currentStatus = RUNNING;
            }
            else if (WIFEXITED(currentStatus) || WIFSIGNALED(currentStatus))
            {
                currentStatus = TERMINATED;
            }
        }

        if (currentStatus != current->status)
        {
            updateProcessStatus(process_list, current->pid, currentStatus);
        }
        current = current->next;
    }
}

void printProcess(process *proc)
{
    char *status = intToStatus(proc->status);
    printf("%-*d %-*s   %-*s\n", 8, proc->pid,
           8, proc->cmd->arguments[0],
           8, status);
    free(status);
}

process *removeProcess(process **process_list, process * procToRemove)
{
    removeProcessFromList(process_list, procToRemove->pid);
    process *next_proc = procToRemove->next;
    freeCmdLines(procToRemove->cmd); /*it's a copy anyway*/
    free(procToRemove);
    return next_proc;
}

void printProcessListAndDeleteIfTerminated(process **process_list)
{
    process *current = *process_list;
    while (current)
    {
        printProcess(current);
        if (current->status == TERMINATED)
        {
            current = removeProcess(process_list, current);
        }
        else
        {
            current = current->next;
        }
    }
}

void onProcs(process **process_list)
{
    printf("%-*s %-*s   %-*s\n", 8, "PID", 8, "Command", 8, "STATUS");
    updateProcessList(process_list);
    printProcessListAndDeleteIfTerminated(process_list);
}

char containsDebugFlag(int argc, char const *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
            return 1;
    }
    return 0;
}

int handleSpecialCommands(cmdLine *cmd, process **process_list)
{
    if (strcmp(cmd->arguments[0], "cd") == 0)
    {
        if (chdir(cmd->arguments[1]) == -1)
            perror("cd failed");
        return 1;
    }
    if (strcmp(cmd->arguments[0], "suspend") == 0)
    {
        if (cmd->argCount > 1)
        {
            int toStop = atoi(cmd->arguments[1]);
            kill(toStop, SIGTSTP);
            usleep(10000);
        }
        return 1;
    }
    if (strcmp(cmd->arguments[0], "wake") == 0)
    {
        if (cmd->argCount > 1)
        {
            int toContinue = atoi(cmd->arguments[1]);
            kill(toContinue, SIGCONT);
            usleep(10000);
        }
        return 1;
    }
    if (strcmp(cmd->arguments[0], "kill") == 0)
    {
        if (cmd->argCount > 1)
        {
            int toTerminate = atoi(cmd->arguments[1]);
            kill(toTerminate, SIGINT);
            usleep(10000);
        }
        return 1;
    }
    if (strcmp(cmd->arguments[0], "procs") == 0)
    {
        onProcs(process_list);
        usleep(10000);
        return 1;
    }

    return 0;
}

void freeHistory(char** history) {
    for (int i = 0; i < HISTLEN; i++) {
        free(history[i]);
    }
}

void printHistory(char **history, int newest, int oldest)
{
    for (int i = newest; i < oldest; i++) {
        printf(history[i]);
    }
}

void historyCommands(char **history, int newest, int oldest, cmdLine *cmd)
{
    char *command = cmd->arguments[0];
    if (command == "history")
    {
        printHistory(history, newest, oldest);
    }
    if (command == "!!")
    {
        
    }
    if (command == "!n")//this is wrong (n is a number)
    {
        
    }
}

void execute(cmdLine *pCmdLine)
{
    if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
    {
        perror("Failed command execution");
        exit(1);
    }
}


//currently on: making sure printHistory works, implementing !!, !n
int main(int argc, char const *argv[])
{
    struct process *processList;
    char debug = containsDebugFlag(argc, argv);
    char *history[HISTLEN];
    int newest = -1, oldest = -1;

    while (1)
    {
        char buffer[PATH_MAX], input[INPUT_MAX];
        getcwd(buffer, PATH_MAX);
        printf("~%s$ ", buffer);
        fgets(input, 2048, stdin);

        if (strcmp(input, "quit\n") == 0)
        {
            freeProcessList(&processList);
            freeHistory(history);
            break;
        }
        if (strcmp(input, "\n") == 0)
            continue;

        char *history_line = malloc(INPUT_MAX);
        strcpy(history_line, input);
        history[++oldest] = history_line;

        cmdLine *cmd;
        cmd = parseCmdLines(input);

        if (debug == 1)
            printf("Executing: %s", input);

        if (handleSpecialCommands(cmd, &processList))
        {
            freeCmdLines(cmd);
            continue;
        }

        int fd[2];
        int needToPipe = 0;
        if (cmd->next != NULL)
        { /*PIPELINE!*/
            needToPipe = 1;
            if (pipe(fd) == -1)
            {
                freeCmdLines(cmd);
                perror("Piping unsuccessful");
                break;
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
            //correct the !n --------------------------------------------------------
            if (cmd->arguments[0] == "history" || cmd->arguments[0] == "!!" || cmd->arguments[0] == "!n")
            {
                historyCommands(history, newest, oldest, cmd);
            }
            else
                execute(cmd);
        }
        else
        { /*Main process*/
            addProcess(&processList, cmd, pid1);

            if (cmd->blocking)
            {
                waitpid(pid1, NULL, 0);
            }
            else
            {
                usleep(10000);
            }

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
                    addProcess(&processList, cmd->next, pid2);
                    close(fd[1]);
                    close(fd[0]);
                    if (cmd->next->blocking)
                    {
                        waitpid(pid2, NULL, 0);
                    }
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
                    //correct the !n --------------------------------------------------------
                    if (cmd->next->arguments[0] == "history" || cmd->next->arguments[0] == "!!" || cmd->next->arguments[0] == "!n")
                    {
                        historyCommands(history, newest, oldest, cmd->next);
                    }
                    else
                        execute(cmd->next);
                }
            }
            usleep(10000);
            freeCmdLines(cmd);
        }
    }
    return 0;
}
