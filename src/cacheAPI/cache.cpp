#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <cstring>
#include "../utils/cache_meta.h"

#define PAGE_COUNT 3
#define NANO 1000000000

using namespace std;

int dirty_count = 0;
vector<page_t> pages_cache(PAGE_COUNT);
unordered_map<uint64_t, page_meta_t> meta_map;

using namespace std;

inline static double get_last_time(struct timespec update) {
    return (double) update.tv_sec + (double) update.tv_nsec / NANO;
}

uint64_t lab2_open(const string& path) {
    if (!filesystem::exists(path))
        return -1;
    struct stat file_stat{};
    if (stat(path.c_str(), &file_stat) == -1)
        return -1;

    uint64_t inode = file_stat.st_ino;
    if (meta_map.find(inode) == meta_map.end())
        meta_map[inode] = {.filepath = path, .size = file_stat.st_size, .opened = true};
    return inode;
}

int lab2_close(uint64_t inode) {
    auto it = meta_map.find(inode);
    if (it == meta_map.end() || !it->second.opened)
        return -1;
    it->second.opened = false;
    it->second.cursor = 0;
    return 0;
}

static vector<int> find_place(const size_t rq) {
    int found = 0;
    vector<int> free_pages;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (!pages_cache[i].busy) {
            found++;
            free_pages.push_back(i);
        }
        if (found == rq)
            return free_pages;
    }
    return {};
}

static uint64_t get_buffer(char* buf, page_meta_t& meta, size_t count) {
    auto pages = meta.pages;
    uint64_t first_page = meta.cursor / PAGE_SIZE;
    uint64_t total_bytes = 0;
    uint16_t offset = meta.cursor % PAGE_SIZE;
    for (uint64_t i = first_page; i < pages.size() && total_bytes < count; ++i) {
        auto& page = pages_cache[meta.pages[i]];
        char* src = page.data + offset;
        uint16_t n = count - total_bytes > page.chars ? page.chars - offset : count - total_bytes;
        strncpy(buf + total_bytes, src, n);
        meta.cursor += n;
        total_bytes += n;
        offset = 0;
    }
    return total_bytes;
}

inline static void clear_pages(const vector<int>& pages) {
    for (int i : pages)
        pages_cache[i].busy = false;
}

static int open_direct(const char* path, char** aligned_buffer) {
    int fd = open(path, O_RDONLY | O_DIRECT);
    if (fd == -1) {
        perror("Error opening file!");
        return -1;
    }
    if (posix_memalign((void**) aligned_buffer, 512, PAGE_SIZE) != 0) {
        perror("Can not allocate buffer");
        return -1;
    }
    return fd;
}

static uint64_t read_direct(int fd, char* aligned_buffer, char* buf, int64_t cursor, size_t count) {
    int i = 0;
    uint64_t total = 0, bytes_read;
    lseek(fd, cursor, SEEK_SET);
    while ((bytes_read = read(fd, aligned_buffer, PAGE_SIZE)) > 0 && total < count) {
        if (bytes_read == -1) {
            free(aligned_buffer);
            return -1;
        }
        size_t n = count - total > PAGE_SIZE ? PAGE_SIZE : count - total;
        strncpy(buf + PAGE_SIZE * i, aligned_buffer, n);
        total += n;
    }
    free(aligned_buffer);
    close(fd);
    return total;
}

static bool read_to_cache(page_meta_t& meta, int fd, char* aligned_buffer) {
    for (int i : meta.pages) {
        page_t& page = pages_cache[i];
        ssize_t bytes_read = read(fd, aligned_buffer, PAGE_SIZE);
        if (bytes_read == -1) {
            free(aligned_buffer);
            return false;
        }
        strncpy(page.data, aligned_buffer, bytes_read);
        page.busy = true;
        page.chars = bytes_read;
    }
    return true;
}

uint64_t lab2_read(uint64_t inode, char* buf, size_t count) {
    auto it = meta_map.find(inode);
    if (it == meta_map.end())
        return -1;
    auto& meta = it->second;

    struct stat file_stat{};
    if (stat(meta.filepath.c_str(), &file_stat) == -1)
        return -1;
    if (get_last_time(file_stat.st_mtim) != meta.last_update) {
        char *aligned_buffer;
        int fd = open_direct(meta.filepath.c_str(), &aligned_buffer);
        if (fd == -1) return 0;
        int64_t rq = (PAGE_SIZE + file_stat.st_size - 1) / PAGE_SIZE;

        if (rq > meta.pages.size()) {
            vector<int> find = find_place(rq - meta.pages.size());
            if (!find.empty())
                meta.pages.insert(meta.pages.end(), find.begin(), find.end());
            else
                return read_direct(fd, aligned_buffer, buf, meta.cursor, count);
        } else {
            clear_pages(vector(meta.pages.begin() + rq, meta.pages.end()));
            meta.pages.resize(meta.pages.size() - rq);
            meta.cursor = meta.cursor > file_stat.st_size ? file_stat.st_size : meta.cursor;
        }

        meta.last_update = get_last_time(file_stat.st_mtim);
        read_to_cache(meta, fd, aligned_buffer);
        free(aligned_buffer);
        close(fd);
    }
    return get_buffer(buf, meta, count);
}

void lab2_lseek(uint64_t inode, off_t offset, int whence) {
    auto& meta = meta_map[inode];
    switch (whence) {
        case 0:
            meta.cursor = offset < meta.size ? offset : meta.size;
            break;
        case 1:
            meta.cursor = meta.size - offset >= 0 ? meta.size - offset : 0;
            break;
        default: meta.cursor = meta.cursor + offset < meta.size ? meta.cursor + offset : meta.size;
    }
}