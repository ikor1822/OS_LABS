#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SHM_SIZE 1024

int main() {
    char filename[256];
    printf("файл: ");
    if (scanf("%255s", filename) != 1) {
        fprintf(stderr, "Неверное имя файла\n");
        exit(EXIT_FAILURE);
    }
    int shm_fd = shm_open("/prime_shm", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    char *shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    shm_ptr[0] = '\0';

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        execl("./child", "child", filename, (char *)NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
        printf("Результат от дочернего процесса:\n%s", shm_ptr);
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink("/prime_shm");
    }

    return 0;
}