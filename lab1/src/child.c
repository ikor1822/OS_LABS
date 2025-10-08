#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int isPrime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    dup2(fd, STDIN_FILENO);
    close(fd);

    char *buffer = (char *)malloc(256);
    int buffer_size = 256;
    int total_read = 0;
    while (1) {
        if (fgets(buffer + total_read, buffer_size - total_read, stdin) != NULL) {
            total_read += strlen(buffer + total_read);
            if (total_read >= buffer_size - 1 || buffer[total_read - 1] == '\n') {
                int num = atoi(buffer);
                if (num <= 0) {
                    free(buffer);
                    exit(EXIT_SUCCESS);
                }
                if (!isPrime(num)) {
                    printf("%d\n", num);
                    fflush(stdout);
                } else {
                    free(buffer);
                    exit(EXIT_SUCCESS);
                }
                total_read = 0;
            } else if (total_read >= buffer_size - 1) {
                buffer_size *= 2;
                char *temp = (char *)realloc(buffer, buffer_size);
                if (temp == NULL) {
                    free(buffer);
                    perror("realloc");
                    exit(EXIT_FAILURE);
                }
                buffer = temp;
            }
        } else {
            break;
        }
    }

    free(buffer);
    return 0;
}