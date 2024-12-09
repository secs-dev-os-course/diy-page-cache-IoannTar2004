#ifndef LAB2_CACHE_META_H
#define LAB2_CACHE_META_H

#include <string>

#define PAGE_SIZE 800
#define FILEPATH_SIZE 256

typedef struct {
    uint64_t inode;
    char filepath[FILEPATH_SIZE];
    uint32_t total;
    uint32_t opened;
    bool is_dirty;
    time_t last_update;
    uint32_t hint;
    uint32_t next_page;

    char data[PAGE_SIZE];
} cache_meta_t;

typedef struct {
    uint32_t page_count;
    uint32_t dirty_count;
    uint32_t free;
    bool is_busy;
} cache_stat_t;

#endif
