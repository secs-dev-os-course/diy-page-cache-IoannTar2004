#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "../utils/cache_meta.h"

#define META_MEMORY_NAME "my_cache_meta"
#define META_SIZE sizeof (cache_meta_t)
#define PAGE_MEMORY_NAME "my_cache_pages"
#define PAGE_SIZE 4096
#define STAT_MEMORY_NAME "my_cache_stat"
#define STAT_SIZE 8

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

void cache_init(cache_stat_t* stat, cache_meta_t* meta_inf, char* pages, int count) {
    printf("> Creating pages\n");
    stat->page_count = count;
    stat->dirty_count = 0;
    for (int i = 0; i < count; ++i) {
        char* n = pages + i * PAGE_SIZE;
        cache_meta_t meta = {.filepath = "", .data = n};
        meta_inf[i] = meta;
    }
}

#define create_shm(mem, name, size, fd) \
    void* mem = create_shared_memory(name, size, &fd);    \
    if (mem == NULL | fd == -1) return 0;

#define clear_shm(mem, size, fd) \
    munmap(mem, size); \
    close(fd);

int main(int argc, char** args) {
    if (argc < 2)
        printf("Usage: ./cacheManager <page count>");
    else {
        printf("> Cache initializing\n");
        int count = (int) strtol(args[1], NULL, 10);
        int meta_size = (int) (count * META_SIZE);
        int page_size = (int) (count * PAGE_SIZE);
        int fd_stat, fd_meta, fd_page;

        create_shm(shm_stat, STAT_MEMORY_NAME, STAT_SIZE, fd_stat)
        create_shm(shm_meta, META_MEMORY_NAME, meta_size, fd_meta)
        create_shm(shm_page, PAGE_MEMORY_NAME, page_size, fd_page)
        cache_init(shm_stat, shm_meta, shm_page, count);

        printf("> Success!\n");
        read_commands();

        printf("> Exiting\n");
        clear_shm(shm_stat, STAT_SIZE, fd_stat)
        clear_shm(shm_meta, meta_size, fd_meta)
        clear_shm(shm_page, page_size, fd_page)
        shm_unlink(STAT_MEMORY_NAME);
        shm_unlink(META_MEMORY_NAME);
        shm_unlink(PAGE_MEMORY_NAME);
    }
}