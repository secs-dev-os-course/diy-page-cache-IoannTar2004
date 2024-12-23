#include <string>
#include <fcntl.h>
#include <unistd.h>
#include "../cacheAPI/cache.h"

using namespace std;

ssize_t ema_search_str(const string& filepath, const string& substr, ssize_t* ptr1, ssize_t* ptr2) {
    uint64_t fd = lab2_open(filepath);
    if (fd == -1) {
        perror("Opening file");
        return -1;
    }
    ssize_t bytes_read, total = 0;
    char buf;
    while ((bytes_read = lab2_read(fd, &buf, 1)) > 0) {
        off_t i = lab2_lseek(fd, 0, CUR) - 1;
        if (bytes_read == -1) {
            perror("Reading file");
            lab2_close(fd);
            return -1;
        }
        if (substr[total] == buf) {
            *ptr1 = total++ == 0 ? i : *ptr1;
            *ptr2 = i;
        } else {
            if (total > 0)
                lab2_lseek(fd, -1, CUR);
            total = 0;
        }

        if (total == substr.length()) {
            lab2_close(fd);
            return total;
        }
    }
    lab2_close(fd);
    *ptr1 = -1;
    *ptr2 = -1;
    return 0;
}