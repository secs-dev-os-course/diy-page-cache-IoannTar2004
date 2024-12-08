#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../utils/cache_meta.h"

#define META_MEMORY_NAME "my_cache_meta"
#define META_SIZE sizeof (cache_meta_t)
#define STAT_SIZE 8
#define PAGE_MEMORY_NAME "my_cache_pages"
#define PAGE_SIZE 4096

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

void* create_shared_memory(char* mem_name, long size, int* fd) {
    *fd = shm_open(mem_name, O_CREAT | O_RDWR, 0666);
    if (*fd == -1) {
        perror("Error opening shared memory");
        return NULL;
    }
    if (ftruncate(*fd, size * PAGE_SIZE) == -1) {
        perror("Error truncation shared memory");
        close(*fd);
        return NULL;
    }
    void* shm = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (shm == MAP_FAILED) {
        perror("Mapping error");
        close(*fd);
        return NULL;
    }
    return shm;
}

void cache_init(cache_meta_t* meta_inf, char** pages, int count) {
    printf("> Creating pages\n");
    for (int i = 0; i < count; ++i) {
        char* n = *pages + i * PAGE_SIZE;
        cache_meta_t meta = {.filepath = "", .data = n};
        meta_inf[i] = meta;
    }
    for (int i = 0; i < count; ++i) {
        printf("%d, %p\n", meta_inf[i].fd, meta_inf[i].data);
    }
}

int main(int argc, char** args) {
    if (argc < 2)
        printf("Usage: ./cacheManager <page count>");
    else {
        printf("> Cache initializing...\n");
        int count = (int) strtol(args[1], NULL, 10);
        int meta_size = (int) (STAT_SIZE + count * META_SIZE);
        int page_size = (int) (count * PAGE_SIZE);
        int fd_meta, fd_page;

        cache_meta_t* shm_meta = create_shared_memory(META_MEMORY_NAME, meta_size, &fd_meta);
        if (shm_meta == NULL | fd_meta == -1) return 0;
        char* shm_page = create_shared_memory(PAGE_MEMORY_NAME, page_size, &fd_page);
        if (shm_page == NULL | fd_page == -1) return 0;
        cache_init(shm_meta, &shm_page, count);

        for (int i = 0; i < count; ++i) {
            printf("%d, %p\n", shm_meta[i].fd, shm_meta[i].data);
        }
        printf("> Success!\n");
        read_commands();

        printf("> Exiting...\n");
        munmap(shm_meta, meta_size);
        munmap(shm_page, page_size);
        close(fd_meta);
        close(fd_page);
        shm_unlink(META_MEMORY_NAME);
        shm_unlink(PAGE_MEMORY_NAME);
    }
}