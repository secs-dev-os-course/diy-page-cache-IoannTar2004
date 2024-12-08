#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define SHARED_MEMORY_NAME "/my_cache"
#define CMD_SIZE 32
#define MiB (long) pow(2, 23)

void read_commands() {
    char* cmd = NULL;
    ssize_t read;
    size_t len = 0;
    while ((read = getline(&cmd, &len, stdin)) != -1) {
        if (!strcmp(cmd, "exit\n"))
            break;
    }
    if (read == -1)
        perror("getline failed!");
    free(cmd);
}

int main(int argc, char** args) {
    if (argc < 2)
        printf("Usage: ./cacheManager <max cache size (MB)>");
    else {
        printf("> Cache initializing...\n");

        const int MAX_SIZE = (int) (strtol(args[1], NULL, 10) * MiB);
        int fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
        if (fd == -1) {
            perror("Error opening shared memory");
            return 0;
        }
        if (ftruncate(fd, MAX_SIZE) == -1) {
            perror("Error truncation shared memory");
            close(fd);
            return 0;
        }
        void* shm = mmap(NULL, MAX_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (shm == MAP_FAILED) {
            perror("Mapping error");
            close(fd);
            return 0;
        }
        printf("> Success!\n");
        read_commands();

        printf("> Exiting...\n");
        munmap(shm, MAX_SIZE);
        close(fd);
        shm_unlink(SHARED_MEMORY_NAME);
    }
}