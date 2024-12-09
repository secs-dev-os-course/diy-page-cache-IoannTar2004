#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <unordered_map>
#include <cstring>
#include "../utils/cache_meta.h"

using namespace std;

cache_stat_t* stat_cache = nullptr;
cache_meta_t* meta_cache = nullptr;
int fd_stat, fd_meta;
size_t meta_size = 0;
unordered_map<unsigned long, uint32_t> pages_map;
unordered_map<unsigned long, string> i_files;

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

int find_page(uint64_t inode) {
    int i = 0;
    while (i < stat_cache->page_count && meta_cache[i].inode != inode) i++;
    return i == stat_cache->page_count ? -1 : i;
}

unsigned long lab2_open(const string& path) {
    if (!filesystem::exists(path) || !get_memory())
        return -1;
    struct stat file_stat{};
    if (stat(path.c_str(), &file_stat) == -1)
        return -1;

    uint64_t inode = file_stat.st_ino;
    i_files[inode] = path;
    if (pages_map.find(inode) == pages_map.end()) {
        int find = find_page(inode);
        pages_map[inode] = find;
    }

    return inode;
}

//uint32_t page = stat_cache->page_count - stat_cache->free;
//if (page < stat_cache->page_count) {
//    meta_cache[page].inode = file_stat.st_ino;
//    stat_cache->free--;
//}
//path.copy(meta_cache[page].filepath, path.length());
//meta_cache[page].filepath[path.length()] = '\0';

int lab2_close(uint64_t inode) {
    auto it = pages_map.find(inode);
    if (it == pages_map.end())
        return -1;
    else if (it->second != -1)
        meta_cache[it->second].opened = false;
    return 0;
}

#define page_init(page, count) \
    meta_cache[page].inode = inode;            \
    strcpy(meta_cache[page].filepath, filepath.c_str()); \
    meta_cache[page].opened++;                 \
    meta_cache[page].total = count;

static void init_pages(uint32_t first, uint32_t count, uint64_t inode, const string& filepath, bool free) {
    for (uint32_t i = 0; i < count; ++i) {
        cache_meta_t* cur = &meta_cache[first + i];
        if (free) {
            cur->next_page = count - i > 1 ? first + i + 1 : -1;
            page_init(first + i, count)
        }
    }
    stat_cache->free -= count;
}

static int alloc_pages(uint64_t inode) {
    int fd = open(i_files[inode].c_str(), O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return -1;
    }
    struct stat file_stat{};
    if (fstat(fd, &file_stat) == -1)
        return -1;
    int page_req = (int) ((PAGE_SIZE + file_stat.st_size) / PAGE_SIZE);
    if (stat_cache->free >= page_req) {
        init_pages(stat_cache->page_count - stat_cache->free, page_req, inode,
                   i_files[inode], true);
        pages_map[inode] = stat_cache->page_count - stat_cache->free;
    }
    close(fd);
}

ssize_t lab2_read(uint64_t inode, char* buf, size_t count) {
    if (!get_memory())
        return -1;
    auto it = pages_map.find(inode);
    if (it == pages_map.end())
        return -1;
    else {
        uint32_t page = it->second;
        if (page == -1 || meta_cache[page].inode != inode) {
            int find = find_page(inode);
//            pages_map[inode] = find;
            if (find == -1) {
                alloc_pages(inode);
            }
        } else
            meta_cache[page].opened++;
    }
}
