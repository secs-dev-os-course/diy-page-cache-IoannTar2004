#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../utils/cache_meta.h"

cache_stat_t* stat_cache = NULL;
cache_meta_t* meta_cache = NULL;
int fd_stat, fd_meta;
size_t meta_size = 0;

#define STAT_MEMORY_NAME "my_cache_stat"
#define META_MEMORY_NAME "my_cache_meta"
#define STAT_SIZE sizeof (cache_stat_t)

void munmap_memory() {
    munmap(stat_cache, STAT_SIZE);
    munmap(meta_cache, meta_size);
    close(fd_stat);
    close(fd_meta);
}
void get_memory() {
    if (stat_cache == NULL) {
        fd_stat = shm_open(STAT_MEMORY_NAME, O_RDWR, 0666);
        if (fd_stat == -1) {
            perror("Can not get cache!");
            return;
        }
        stat_cache = mmap(NULL, STAT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd_stat, 0);
        if (stat_cache == MAP_FAILED) {
            perror("Mapping error");
            close(fd_stat);
            return;
        }
    }

    if (meta_cache == NULL) {
        fd_meta = shm_open(META_MEMORY_NAME, O_RDWR, 0666);
        if (fd_meta == -1) {
            perror("Can not get cache!");
            munmap_memory();
            return;
        }
        struct stat meta_stat;
        if (fstat(fd_stat, &meta_stat) == -1) {
            perror("Can not get cache!");
            munmap_memory();
            return;
        }
        meta_size = sizeof (cache_meta_t) * stat_cache->page_count;
        meta_cache = mmap(NULL, meta_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_meta, 0);
        if (meta_cache == MAP_FAILED) {
            perror("Mapping error");
            munmap_memory();
            return;
        }
    }
}

int lab2_open(const char* path) {
    if (access(path, F_OK) != 0)
        return -1;

    get_memory();
    struct stat file_stat;
    if (stat(path, &file_stat) == -1)
        return -1;

    meta_cache[0].inode = 10;
    munmap_memory();
    return 0;
}