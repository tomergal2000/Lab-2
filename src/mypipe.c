#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


int main(int argc, char const *argv[])
{
    int fd[2];
    if(pipe(fd) == -1){
        perror("Piping unsuccessful");
        return 1;
    }
    
    pid_t pid = fork();
    
    if (pid == 0)
    { /*Child process*/
        close(fd[0]);
        char *randomString = "Hello parent!";
        printf("Child: Sending parent: %s\n", randomString);
        
        write(fd[1], randomString, strlen(randomString) + 1);
        close(fd[1]);
    }
    else
    {/*Parent process*/
        close(fd[1]);
        char fromChild[1024];
        
        read(fd[0], fromChild, 1024);
        printf("Parent: Got from child: %s\n", fromChild);
        
        close(fd[0]);
        waitpid(pid, NULL, WUNTRACED);
    }
    
    return 0;
}

