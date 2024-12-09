#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;
using namespace testing;

extern int lab2_open(const string& path);
extern int lab2_close(uint64_t inode);
ssize_t lab2_read(uint64_t inode, char* buf, size_t count);
extern void munmap_memory();

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

int main(int argc, char** args) {
//    if (argc < 2) {
//        cout << "Usage: ./tests <working directory>\n";
//        exit(1);
//    }
//    chdir(args[1]);
//    InitGoogleTest(&argc, args);
//    return RUN_ALL_TESTS();

    uint64_t f1 = lab2_open("../CMakeLists.txt");
    char* buf;
    lab2_read(f1, buf, 10);
    munmap_memory();
    return 0;
}