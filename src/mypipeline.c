#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char const *argv[])
{
    int fd[2];
    if(pipe(fd) == -1){
        perror("Piping unsuccessful");
        return 1;
    }
    fprintf(stderr, "(parent_process>forking child 1)\n");
    int pid1 = fork();
    if (pid1 != 0)
    {
        fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);
    }

    if(pid1 == 0)
    { /*Child 1*/
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        char *command1 = "ls";
        char *arguments1[] = {"ls", "-l", 0};
        fprintf(stderr, "(child1>going to execute cmd: %s %s)\n", command1, arguments1[1]);
        if (execvp(command1, arguments1) == -1)
        {
            perror("Failed command 1 execution");
            exit(1);
        }
    }
    else
    { /*Parent*/
        fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
        close(fd[1]);

        fprintf(stderr, "(parent_process>forking child 2)\n");
        int pid2 = fork();
        if (pid2 != 0)
        {
            fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);
        }

        if (pid2 == 0)
        { /*Child 2*/
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
            dup2(fd[0], STDIN_FILENO);
            close(fd[1]);
            close(fd[0]);
            char *command2 = "tail";
            char *arguments2[] = {"tail", "-n", "2", 0};
            fprintf(stderr, "(child2>going to execute cmd: %s %s %s)\n", command2, arguments2[1], arguments2[2]);
            if (execvp(command2, arguments2) == -1)
            {
                perror("Failed command 2 execution");
                exit(1);
            }        
        }

        fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
        close(fd[0]);
        fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
    return 0;
}
