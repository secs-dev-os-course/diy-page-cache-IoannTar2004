#include <iostream>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <cstring>
#include "../utils/cache_meta.h"

#define PAGE_COUNT 3
#define STAT_MEMORY_NAME "my_cache_stat"
#define META_MEMORY_NAME "my_cache_meta"
#define STAT_SIZE sizeof (cache_stat_t)
#define NANO 1000000000

using namespace std;

int dirty_count = 0;
vector<cache_meta_t> pages_cache(3);
unordered_map<uint64_t, pair<int, string>> i_files;

using namespace std;

unsigned long lab2_open(const string& path) {
    if (!filesystem::exists(path))
        return -1;
    struct stat file_stat{};
    if (stat(path.c_str(), &file_stat) == -1)
        return -1;

    uint64_t inode = file_stat.st_ino;
    if (i_files.find(inode) == i_files.end())
        i_files[inode] = make_pair(-1, path);
    return inode;
}

int lab2_close(uint64_t inode) {
    auto it = i_files.find(inode);
    if (it == i_files.end() || it->first == -1)
        return -1;
    else {
        int i = pages_cache[it->first].next_page;
        while (i != -1) {
            pages_cache[i].opened = false;
            i = pages_cache[i].next_page;
        }
    }
    return 0;
}

//#define page_init(page, count) \
//    pages_cache[page].inode = inode;            \
//    strcpy(pages_cache[page].filepath, filepath.c_str()); \
//    pages_cache[page].opened++;                 \
//    pages_cache[page].total = count;
//
//static void init_pages(uint32_t first, uint32_t count, uint64_t inode, const string& filepath, bool free) {
//    for (uint32_t i = 0; i < count; ++i) {
//        cache_meta_t* cur = &pages_cache[first + i];
//        if (free) {
//            cur->next_page = count - i > 1 ? first + i + 1 : -1;
//            page_init(first + i, count)
//        }
//    }
//    stat_cache.free -= count;
//}
//
//static cache_meta_t* alloc_pages(uint64_t inode) {
//    struct stat file_stat{};
//    if (stat(i_files[inode].c_str(), &file_stat) == -1)
//        return nullptr;
//    auto page_req = (uint32_t) ((PAGE_SIZE + file_stat.st_size) / PAGE_SIZE);
//    uint32_t first;
//    if (stat_cache.free >= page_req) {
//        first = stat_cache.page_count - stat_cache.free;
//        init_pages(first, page_req, inode,i_files[inode], true);
//        pages_map[inode] = first;
//    }
//    return &pages_cache[first];
//}
//
inline static double get_last_time(struct timespec update) {
    return (double) update.tv_sec + (double) update.tv_nsec / NANO;
}

static vector<int> find_place(const size_t rq) {
    int found = 0;
    vector<int> free_pages;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (pages_cache[i].inode == 0) {
            found++;
            free_pages.push_back(i);
        }
        if (found == rq)
            return free_pages;
    }
    return {};
}

size_t round_up(size_t size, size_t block_size) {
    return (size + block_size - 1) & ~(block_size - 1);
}

static bool read_file(vector<int>& pages, const string& path, char** buf, size_t count) {
    int fd = open(path.c_str(), O_RDONLY | O_DIRECT);
    if (fd == -1) {
        perror("Error opening file!");
        return false;
    }
    size_t buffer_size = round_up(count, 512);
    void* aligned_buffer;
    if (posix_memalign(&aligned_buffer, 512, 4096) != 0) {
        perror("Can not alloc buffer");
        close(fd);
        return false;
    }
    ssize_t bytes_read = read(fd, aligned_buffer, 4096);
    char* d = (char*) aligned_buffer;
    if (bytes_read == -1) {
        free(aligned_buffer);
        return false;
    }
    for (int i = 0; i < pages.size(); i++) {
        strncpy(pages_cache[pages[i]].data, (char*) aligned_buffer + i * PAGE_SIZE, count + 1);
    }
    size_t a = strlen(pages_cache[*pages.end()].data);
    pages_cache[*pages.end()].data[a] = '\0';
    *buf = (char*)(aligned_buffer);
    close(fd);

    return true;
}

ssize_t lab2_read(uint64_t inode, char* buf, size_t count) {
    auto it = i_files.find(inode);
    if (it == i_files.end())
        return -1;
    int page = it->second.first;

    struct stat file_stat{};
    if (stat(i_files[inode].second.c_str(), &file_stat) == -1)
        return -1;
    size_t rq = file_stat.st_size <= count ? (size_t)(PAGE_SIZE + file_stat.st_size) / PAGE_SIZE : count;
    vector<int> free_pages;
    if (page == -1)
        free_pages = find_place(rq);
    else if ((get_last_time(file_stat.st_mtim) != pages_cache[page].last_update)) {
        if (rq > pages_cache[page].total)
            free_pages = find_place(rq - pages_cache[page].total);
//        else if (rq < pages_cache[page].total)
//            nullptr;
    }
    if (!free_pages.empty()) {
        read_file(free_pages, i_files[inode].second, &buf, rq);
        free((char *) buf);
    }
}
