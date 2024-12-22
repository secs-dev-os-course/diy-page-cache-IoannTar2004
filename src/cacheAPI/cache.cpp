#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <cstring>
#include "../utils/cache_meta.h"

#define PAGE_COUNT 5
#define NANO 1000000000
#define FIND_INODE(inode, ret) \
    auto it = meta_map.find(inode); \
    if (it == meta_map.end() || !it->second.opened) \
        return ret;

using namespace std;

vector<page_t> pages_cache(PAGE_COUNT);
unordered_map<uint64_t, page_meta_t> meta_map;

using namespace std;

inline static double get_last_time(struct timespec update) {
    return (double) update.tv_sec + (double) update.tv_nsec / NANO;
}

void lab2_fsync(uint64_t inode);
access_hint_t lab2_advice(uint64_t inode, access_hint_t hint);

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
    FIND_INODE(inode, -1)
    it->second.opened = false;
    it->second.cursor = 0;
    return 0;
}

static vector<int> cache_preempt(size_t rq) {
    struct timespec current_time{};
    clock_gettime(CLOCK_REALTIME, &current_time);
    pair<const uint64_t, page_meta_t>* meta_map_el = nullptr;
    page_meta_t* meta;

    for (auto& pair : meta_map) {
        meta = &pair.second;
        if (meta->pages.size() >= rq && !meta->opened && get_last_time(current_time) > meta->hint) {
            meta_map_el = &pair;
            break;
        }
    }
    if (meta_map_el == nullptr)
        return {};

    vector<int> free(rq);
    lab2_fsync(meta_map_el->first);
    for (int i = 0; i < meta->pages.size(); ++i) {
        auto& page = pages_cache[meta->pages[i]];
        page.chars = 0;
        page.busy = i < rq;
        if (i < rq)
            free[i] = meta->pages[i];
    }
    meta_map.erase(meta_map_el->first);
    return free;
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
    return cache_preempt(rq);
}

static uint64_t get_buffer(char* buf, page_meta_t& meta, size_t count) {
    count = meta.size - meta.cursor < count ? meta.size - meta.cursor : count;
    uint64_t total_bytes = 0;
    uint16_t offset = meta.cursor % PAGE_SIZE;
    for (uint64_t i = meta.cursor / PAGE_SIZE; total_bytes < count; ++i) {
        auto& page = pages_cache[meta.pages[i]];
        char* src = page.data + offset;
        uint16_t n = count - total_bytes > page.chars - offset ? page.chars - offset : count - total_bytes;
        strncpy(buf + total_bytes, src, n);
        meta.cursor += n;
        total_bytes += n;
        offset = 0;
    }
    return total_bytes;
}

static int open_direct(const char* path, char** aligned_buffer, int flags) {
    int fd = open(path, flags | O_DIRECT);
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

static bool read_to_cache(page_meta_t& meta) {
    char* aligned_buffer;
    int fd = open_direct(meta.filepath.c_str(), &aligned_buffer, O_RDONLY);
    if (fd == -1) return false;

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
    free(aligned_buffer);
    close(fd);
    return true;
}

static uint64_t read_direct(page_meta_t& meta, char* buf, int64_t count) {
    char* aligned_buffer;
    int fd = open_direct(meta.filepath.c_str(), &aligned_buffer, O_RDONLY);
    if (fd == -1) return 0;

    int i = 0;
    uint64_t total = 0, bytes_read;
    lseek(fd, meta.cursor, SEEK_SET);
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

uint64_t lab2_read(uint64_t inode, char* buf, int64_t count) {
    FIND_INODE(inode, -1)
    auto& meta = it->second;

    struct stat file_stat{};
    if (stat(meta.filepath.c_str(), &file_stat) == -1)
        return -1;
    if (get_last_time(file_stat.st_mtim) != meta.last_update) {
        int64_t rq = (PAGE_SIZE + file_stat.st_size - 1) / PAGE_SIZE;

        if (rq > meta.pages.size()) {
            vector<int> find = find_place(rq - meta.pages.size());
            if (!find.empty())
                meta.pages.insert(meta.pages.end(), find.begin(), find.end());
            else
                return read_direct(meta, buf, count);
        } else
            meta.cursor = meta.cursor > file_stat.st_size ? file_stat.st_size : meta.cursor;

        meta.last_update = get_last_time(file_stat.st_mtim);
        read_to_cache(meta);
        meta.size = file_stat.st_size;
    }
    return get_buffer(buf, meta, count);
}

static int64_t write_to_cache(page_meta_t& meta, char* buf, int64_t count) {
    if (pages_cache[meta.pages[0]].chars > 0)
        meta.size = meta.cursor + count > meta.size ? meta.cursor + count : meta.size;
    else {
        meta.size = count;
        meta.cursor = 0;
    }
    int64_t total_bytes = 0;
    uint16_t offset = meta.cursor % PAGE_SIZE;
    for (uint64_t i = meta.cursor / PAGE_SIZE; total_bytes < count; ++i) {
        auto& page = pages_cache[meta.pages[i]];
        uint16_t n = count - total_bytes > PAGE_SIZE - offset ? PAGE_SIZE - offset : count - total_bytes;
        strncpy(page.data + offset, buf + total_bytes, n);
        meta.cursor += n;
        total_bytes += n;
        page.busy = true;
        page.is_dirty = true;
        page.chars = n > page.chars ? n - page.chars : page.chars;
        offset = 0;
    }
    return total_bytes;
}

#define WRITE_ERROR(ret) \
    if (bytes_written == -1) {  \
        perror("Error writing to file");    \
        free(aligned_buffer);   \
        close(fd);  \
        return ret;  \
    }

#define WRITE_DIRECT(mem, n, ret) \
    memset(aligned_buffer, 0, PAGE_SIZE);   \
    memcpy(aligned_buffer, mem, n);   \
    ssize_t bytes_written = write(fd, aligned_buffer, PAGE_SIZE);   \
    WRITE_ERROR(ret)


static int64_t write_direct(page_meta_t& meta, char* buf, int64_t count) {
    char *aligned_buffer;
    int fd = open_direct(meta.filepath.c_str(), &aligned_buffer, O_WRONLY | O_CREAT);
    if (fd == -1) return 0;

    uint16_t n = INT16_MAX;
    for (int i = 0; n > PAGE_SIZE; ++i) {
        auto& page = pages_cache[meta.pages[i]];
        n = meta.cursor / PAGE_SIZE > 0 ? PAGE_SIZE : meta.cursor % PAGE_SIZE;
        page.is_dirty = false;
        WRITE_DIRECT(page.data, n, -1)
    }
    n = INT16_MAX;
    for (int i = 0; n > PAGE_SIZE; ++i) {
        n = count - i * PAGE_SIZE > PAGE_SIZE ? PAGE_SIZE : count - i * PAGE_SIZE;
        WRITE_DIRECT(buf + i * PAGE_SIZE, n, -1)
    }
    free(aligned_buffer);
    close(fd);
    return count;
}

int64_t lab2_write(uint64_t inode, char* buf, int64_t count) {
    FIND_INODE(inode, -1)
    auto& meta = it->second;
    uint64_t rq = (count + meta.cursor - 1) / PAGE_SIZE + 1;
    if (rq > meta.pages.size()) {
        vector<int> find = find_place(rq - meta.pages.size());
        if (!find.empty())
            meta.pages.insert(meta.pages.end(), find.begin(), find.end());
        else
            return write_direct(meta, buf, count);
    }
    return write_to_cache(meta, buf, count);
}

void lab2_lseek(uint64_t inode, int64_t offset, int whence) {
    FIND_INODE(inode,)
    auto& meta = it->second;
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

void lab2_fsync(uint64_t inode) {
    FIND_INODE(inode,)
    auto& meta = it->second;

    char *aligned_buffer;
    int fd = open_direct(meta.filepath.c_str(), &aligned_buffer, O_WRONLY | O_CREAT);
    if (fd == -1) return;

    for (int i = 0; i < meta.pages.size(); i++) {
        auto& page = pages_cache[meta.pages[i]];
        if (page.is_dirty) {
            lseek(fd, i * PAGE_SIZE, SEEK_SET);
            page.is_dirty = false;
            WRITE_DIRECT(page.data, page.chars,)
        }
    }
    free(aligned_buffer);
    close(fd);
}

access_hint_t lab2_advice(uint64_t inode, access_hint_t hint) {
    struct timespec current_time{};
    clock_gettime(CLOCK_REALTIME, &current_time);
    meta_map[inode].hint = get_last_time(current_time) + hint;
}

void model() {
    meta_map[0] = {.pages = {0,1,2}};
    meta_map[1] = {.pages = {3, 4}};
    lab2_advice(0, 3600);
//    lab2_advice(1, 3600);
    for (int i = 0; i < 5; ++i) {
        pages_cache[i].busy = true;
    }
}