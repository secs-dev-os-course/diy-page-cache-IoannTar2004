#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fstream>
#include <fcntl.h>
#include "../src/cacheAPI/cache.h"
using namespace std;
using namespace testing;

class LabTest : public Test {

    void SetUp() override {
        const TestInfo* test_info = UnitTest::GetInstance()->current_test_info();
        cout << "Test " << test_info->name() << " starts\n";
    }
    void TearDown() override {

    }
};

TEST_F(LabTest, foo) {
    sleep(5);
}

void write1(string& path) {
    ofstream file(path);
    file << "abcdef";
    file.close();
}

void write2(string& path) {
    ofstream file(path);
    char i = 65;
    for (int j = 0; j < 4096; ++j) {
        file << i;
        i = i == 122 ? 65 : i + 1;
    }
    file.close();
}

int main(int argc, char** args) {
//    if (argc < 2) {
//        cout << "Usage: ./tests <working directory>\n";
//        exit(1);
//    }
//    chdir(args[1]);
//    InitGoogleTest(&argc, args);
//    return RUN_ALL_TESTS();
    string path = "../a";
    write1(path);
    model();
    uint64_t f1 = lab2_open(path);
    char* buf = (char*) (malloc(4100));
    lab2_read(f1, buf, 4096);
//    lab2_lseek(f1, 4092, BEGIN);

    lab2_close(f1);
    free(buf);
    return 0;
}