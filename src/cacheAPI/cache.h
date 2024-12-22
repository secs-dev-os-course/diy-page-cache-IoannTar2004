#ifndef LAB2_CACHE_H
#define LAB2_CACHE_H

#include <string>

#define BEGIN 0
#define END 1
#define CUR 2

extern uint64_t lab2_open(const std::string& path);
extern int lab2_close(uint64_t inode);
extern uint64_t lab2_read(uint64_t inode, char* buf, int64_t count);
extern void lab2_lseek(uint64_t inode, int64_t offset, int whence);
extern int64_t lab2_write(uint64_t inode, char* buf, int64_t count);

#endif //LAB2_CACHE_H
