#include <gtest/gtest.h>
#include <unistd.h>
#include <fstream>
#include "../src/cacheAPI/cache.h"
#include "../src/bench/ema-search-str.h"
#include <math.h>
using namespace std;
using namespace testing;

class LabTest : public Test {

    void SetUp() override {
        const TestInfo* test_info = UnitTest::GetInstance()->current_test_info();
        cout << "Test " << test_info->name() << " starts\n";
    }
};

void write1(const string& path, size_t count) {
    ofstream file(path);
    char i = 65;
    for (int j = 0; j < count; ++j) {
        file << i;
        i = i == 122 ? 65 : i + 1;
    }
    file.close();
}

string read_test(ssize_t count, ssize_t seek) {
    write1("test_file", count);
    usleep(10000);
    uint64_t inode = lab2_open("test_file");
    char* buf = (char*) malloc(count);
    lab2_lseek(inode, seek, SET);
    lab2_read(inode, buf, count);
    lab2_close(inode);
    string str = buf;
    free(buf);
    return str;
}

TEST_F(LabTest, read_to_cache) {
    string str = read_test(6, 0);
    ASSERT_EQ(str, "ABCDEF");
}

TEST_F(LabTest, read_direct) {
    string str = read_test(4098, 0);
    ASSERT_EQ(str.length(), 4098);
}

TEST_F(LabTest, read_with_seek) {
    string str = read_test(6, 2);
    ASSERT_EQ(str.substr(0, 4), "CDEF");
}

TEST_F(LabTest, write_to_cache_and_fsync) {
    char* buf = "ABCDEF";
    uint64_t inode = lab2_open("test_file");
    lab2_write(inode, buf, 6);
    lab2_fsync(inode);
    lab2_close(inode);

    ifstream file("test_file");
    ostringstream buffer;
    buffer << file.rdbuf();
    file.close();

    ASSERT_EQ(buffer.str().substr(0,6), "ABCDEF");
}

void generate_file(const string& path, size_t count) {
    srand(time(nullptr));
    ofstream file(path);
    for (int i = 0; i < count; ++i) {
        char letter = rand() % 57 + 65;
        file << letter;
    }
    file.close();
}

void bench() {
    ssize_t ptr1, ptr2;
    for (int i = 0; i < 10; ++i) {
        system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
        struct timespec start{}, end{};
        clock_gettime(CLOCK_REALTIME, &start);
        ema_search_str("bench_file", "z", &ptr1, &ptr2);
        clock_gettime(CLOCK_REALTIME, &end);
        printf("%lds %ldn\n", (end.tv_sec - start.tv_sec), (abs(end.tv_nsec - start.tv_nsec)));
    }
}

int main(int argc, char** args) {
    if (argc < 3) {
        cout << "Usage: ./tests <working directory>\n";
        return 0;
    }
    chdir(args[1]);
    InitGoogleTest(&argc, args);
    return RUN_ALL_TESTS();
}