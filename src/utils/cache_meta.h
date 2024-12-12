#ifndef LAB2_CACHE_META_H
#define LAB2_CACHE_META_H

#include <string>

#define PAGE_SIZE 3   //TODO временно не 4096

typedef struct {
    uint64_t inode;
    std::string filepath;
    int total;
    bool opened;
    bool is_dirty;
    double last_update;
    int hint;
    int begin;
    int end;
    int next_page;

    char data[PAGE_SIZE];
} cache_meta_t;

typedef struct {
    uint32_t page_count;
    uint32_t dirty_count;
} cache_stat_t;

#endif
