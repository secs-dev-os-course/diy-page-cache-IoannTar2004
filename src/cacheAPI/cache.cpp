#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <unordered_map>
#include <list>
#include "../utils/cache_meta.h"

using namespace std;

cache_stat_t* stat_cache = nullptr;
cache_meta_t* meta_cache = nullptr;
int fd_stat, fd_meta;
size_t meta_size = 0;
unordered_map<uint64_t, list<uint32_t>> pages_map;

#define STAT_MEMORY_NAME "my_cache_stat"
#define META_MEMORY_NAME "my_cache_meta"
#define STAT_SIZE sizeof (cache_stat_t)

using namespace std;

void munmap_memory() {
    munmap(stat_cache, STAT_SIZE);
    munmap(meta_cache, meta_size);
    close(fd_stat);
    close(fd_meta);
    stat_cache = nullptr, meta_cache = nullptr;
}

static bool get_memory() {
    if (stat_cache == nullptr) {
        fd_stat = shm_open(STAT_MEMORY_NAME, O_RDWR, 0666);
        if (fd_stat == -1) {
            cerr << "Can not get cache!\n";
            return false;
        }
        stat_cache = (cache_stat_t*) (mmap(nullptr, STAT_SIZE, PROT_READ | PROT_WRITE,
                                                      MAP_SHARED, fd_stat, 0));
        if (stat_cache == MAP_FAILED) {
            cerr << "Mapping error\n";
            close(fd_stat);
            return false;
        }
    }

    if (meta_cache == nullptr) {
        fd_meta = shm_open(META_MEMORY_NAME, O_RDWR, 0666);
        if (fd_meta == -1) {
            cerr << "Can not get cache!\n";
            munmap_memory();
            return false;
        }
        struct stat meta_stat{};
        if (fstat(fd_stat, &meta_stat) == -1) {
            cerr << "Can not get cache!\n";
            munmap_memory();
            return false;
        }
        meta_size = sizeof (cache_meta_t) * stat_cache->page_count;
        meta_cache = (cache_meta_t*) (mmap(nullptr, meta_size,PROT_READ | PROT_WRITE,
                                                      MAP_SHARED, fd_meta, 0));
        if (meta_cache == MAP_FAILED) {
            cerr << "Mapping error\n";
            munmap_memory();
            return false;
        }
    }
    return true;
}

uint64_t lab2_open(const string& path) {
    if (!filesystem::exists(path) || !get_memory())
        return -1;
    struct stat file_stat{};
    if (stat(path.c_str(), &file_stat) == -1)
        return -1;

    uint32_t page = stat_cache->page_count - stat_cache->free;
    if (page < stat_cache->page_count) {
        meta_cache[page].inode = file_stat.st_ino;
        stat_cache->free--;
    }
    meta_cache[page].opened = true;
    path.copy(meta_cache[page].filepath, path.length());
    meta_cache[page].filepath[path.length()] = '\0';
    pages_map[file_stat.st_ino].push_back(page);

    return file_stat.st_ino;
}

int lab2_close(uint64_t inode) {
    list<uint32_t> pages = pages_map[inode];
    for (const auto& o : pages)
        meta_cache[o].opened = false;
    return 0;
}
