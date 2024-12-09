#ifndef LAB2_CACHE_META_H
#define LAB2_CACHE_META_H

#include <string>

#define PAGE_SIZE 4096
#define FILEPATH_SIZE 256

typedef struct {
    uint64_t inode;
    char filepath[FILEPATH_SIZE];
    uint32_t offset;
    bool opened;
    bool is_dirty;
    time_t last_update;
    uint32_t hint;

    char data[PAGE_SIZE];
} cache_meta_t;

typedef struct {
    uint32_t page_count;
    uint32_t dirty_count;
    bool is_busy;
    uint32_t free;
} cache_stat_t;

#endif
