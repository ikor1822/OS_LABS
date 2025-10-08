#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXSIZE 256

int main() {
    int pipefd[2];
    pid_t pid;
    char filename[MAXSIZE];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    printf("файл:");
    if (scanf("%255s", filename) != 1) {
        fprintf(stderr, "неверное имя файла\n");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execl("./child", "child", filename, (char *)NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else { 
        close(pipefd[1]);

        char *buffer = (char *)malloc(256);
        int buffer_size = 256;
        int total_read = 0;
        ssize_t count;
        while ((count = read(pipefd[0], buffer + total_read, buffer_size - total_read - 1)) > 0) {
            total_read += count;
            if (total_read >= buffer_size - 1) {
                buffer_size *= 2;
                char *temp = (char *)realloc(buffer, buffer_size);
                if (temp == NULL) {
                    free(buffer);
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
                buffer = temp;
            }
        }
        if (count == -1) {
            perror("read");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        buffer[total_read] = '\0';
        printf("%s", buffer);
        fflush(stdout);
        if (atoi(buffer) <= 0) {
        }

        free(buffer);
        close(pipefd[0]);
        wait(NULL);
    }

    return 0;
}