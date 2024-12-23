#ifndef LAB2_CACHE_META_H
#define LAB2_CACHE_META_H

#include <string>
#include <vector>

#define PAGE_SIZE 4096

using namespace std;

typedef struct {
    bool busy;
    bool is_dirty;
    uint16_t chars;
    char data[PAGE_SIZE];
} page_t;

typedef double access_hint_t;

typedef struct {
    vector<int> pages;
    string filepath;
    int64_t size;
    bool opened;
    int64_t cursor;
    double last_update;
    access_hint_t hint;
} page_meta_t;

#endif
