#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define INITIAL_SIZE 1024

int isPrime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя файла>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    int shm_fd = shm_open("/prime_shm", O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    size_t buffer_size = INITIAL_SIZE;
    char *local_buffer = malloc(buffer_size);
    if (!local_buffer) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    local_buffer[0] = '\0';
    size_t used = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        int num = atoi(line);
        if (num <= 0) {
            break;
        }
        if (!isPrime(num)) {
            char temp[20];
            int len = snprintf(temp, sizeof(temp), "%d\n", num);
            if (used + len + 1 >= buffer_size) {
                buffer_size *= 2;
                char *new_buffer = realloc(local_buffer, buffer_size);
                if (!new_buffer) {
                    perror("realloc");
                    free(local_buffer);
                    exit(EXIT_FAILURE);
                }
                local_buffer = new_buffer;
            }            
            strcat(local_buffer, temp);
            used += len;
        } else {
            break;
        }
    }
    fclose(file);
    size_t needed_size = used + 1;
    if (ftruncate(shm_fd, needed_size) == -1) {
        perror("ftruncate");
        free(local_buffer);
        exit(EXIT_FAILURE);
    }
    char *shm_ptr = mmap(NULL, needed_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        free(local_buffer);
        exit(EXIT_FAILURE);
    }
    memcpy(shm_ptr, local_buffer, needed_size);
    munmap(shm_ptr, needed_size);
    close(shm_fd);
    free(local_buffer);
    return 0;
}