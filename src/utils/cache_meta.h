#ifndef LAB2_CACHE_META_H
#define LAB2_CACHE_META_H

typedef struct {
    int fd;
    char* filepath;
    int fd_valid;
    int offset;
    int count;
    int is_dirty;
    char* data;
    int is_busy;

    time_t last_update;
    long hint;
} cache_meta_t;

typedef struct {
    int page_count;
    int dirty_count;
} cache_stat_t;

int page_struct_size();

#endif
