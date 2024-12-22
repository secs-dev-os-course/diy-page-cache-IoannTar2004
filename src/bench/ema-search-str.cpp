#include <string>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

ssize_t ema_search_str(const string& filepath, const string& substr, ssize_t* ptr1, ssize_t* ptr2) {
    uint64_t fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("Opening file");
        return -1;
    }
    ssize_t bytes_read, total = 0;
    char buf;
    while ((bytes_read = read(fd, &buf, 1)) > 0) {
        off_t i = lseek(fd, 0, SEEK_CUR) - 1;
        if (bytes_read == -1) {
            perror("Reading file");
            close(fd);
            return -1;
        }
        if (substr[total] == buf) {
            *ptr1 = total++ == 0 ? i : *ptr1;
            *ptr2 = i;
        } else {
            if (total > 0)
                lseek(fd, -1, SEEK_CUR);
            total = 0;
        }

        if (total == substr.length()) {
            close(fd);
            return total;
        }
    }
    close(fd);
    *ptr1 = -1;
    *ptr2 = -1;
    return 0;
}