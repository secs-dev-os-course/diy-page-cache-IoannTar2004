#ifndef LAB2_CACHE_H
#define LAB2_CACHE_H

#include <string>
#include "../utils/cache_meta.h"

#define SET 0
#define END 1
#define CUR 2

extern uint64_t lab2_open(const std::string& path);
extern int lab2_close(uint64_t inode);
extern uint64_t lab2_read(uint64_t inode, char* buf, int64_t count);
extern void lab2_lseek(uint64_t inode, int64_t offset, int whence);
extern int64_t lab2_write(uint64_t inode, char* buf, int64_t count);
extern void lab2_fsync(uint64_t inode);
extern access_hint_t lab2_advice(uint64_t inode, access_hint_t hint);
extern void model();

#endif //LAB2_CACHE_H
