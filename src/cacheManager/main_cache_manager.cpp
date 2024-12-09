#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "../utils/cache_meta.h"

#define STAT_MEMORY_NAME "my_cache_stat"
#define STAT_SIZE sizeof (cache_stat_t)
#define META_MEMORY_NAME "my_cache_meta"
#define META_SIZE sizeof (cache_meta_t)

using namespace std;

void print_info(cache_stat_t* stat, cache_meta_t* meta) {
    cout << "page count: " << stat->page_count << endl;
    cout << "dirty pages: " << stat->dirty_count << endl;
    cout << "free pages: " << stat->free << endl;
    cout << "---------------------\n\tPages\n";
    for (int i = 0; i < stat->page_count; ++i) {
        cout << "inode: " << meta[i].inode << endl;
        cout << "filepath: \'" << meta[i].filepath << "\'" << endl;
        cout << "offset: " << meta[i].offset << endl;
        cout << "is opened: " << meta[i].opened << endl;
        cout << "is dirty: " << meta[i].is_dirty << endl;
        cout << "last update: " << meta[i].last_update << endl;
        cout << "hint: " << meta[i].hint << endl << endl;
    }
}

void read_commands(cache_stat_t* stat, cache_meta_t* meta) {
    string cmd;
    do {
        cin >> cmd;
        if (cmd == "info")
            print_info(stat, meta);
    } while (cmd != "exit");
}

void* create_shared_memory(const char* mem_name, long size, int* fd) {
    *fd = shm_open(mem_name, O_CREAT | O_RDWR, 0666);
    if (*fd == -1) {
        cerr << "Error opening shared memory\n";
        return nullptr;
    }
    if (ftruncate(*fd, size * PAGE_SIZE) == -1) {
        cerr << "Error truncation shared memory\n";
        close(*fd);
        return nullptr;
    }
    void* shm = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
    if (shm == MAP_FAILED) {
        cerr << "Mapping error\n";
        close(*fd);
        return nullptr;
    }
    return shm;
}

void cache_init(cache_stat_t* stat, int count) {
    stat->page_count = count;
    stat->dirty_count = 0;
    stat->free = count;
}

#define create_shm(mem, name, size, fd) \
    void* mem = create_shared_memory(name, size, &fd);    \
    if (mem == NULL | fd == -1) return 0;

#define clear_shm(mem, size, fd) \
    munmap(mem, size); \
    close(fd);

int main(int argc, char** args) {
    if (argc < 2)
        cout << "Usage: ./cacheManager <page count>\n";
    else {
        cout << "> Cache initializing\n";
        int count = (int) stoi(args[1]);
        int meta_size = (int) (count * META_SIZE);
        int fd_stat, fd_meta;

        create_shm(shm_stat, STAT_MEMORY_NAME, STAT_SIZE, fd_stat)
        create_shm(shm_meta, META_MEMORY_NAME, meta_size, fd_meta)
        cache_init((cache_stat_t*) shm_stat, count);

        cout << "> Success!\n";
        read_commands((cache_stat_t*) shm_stat, (cache_meta_t*) shm_meta);

        cout << "> Exiting\n";
        clear_shm(shm_stat, STAT_SIZE, fd_stat)
        clear_shm(shm_meta, meta_size, fd_meta)
        shm_unlink(STAT_MEMORY_NAME);
        shm_unlink(META_MEMORY_NAME);
    }
}