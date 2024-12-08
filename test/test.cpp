#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;
using namespace testing;

extern "C" {
    void a(int d);
}

class LabTest : public Test {

    void SetUp() override {
        const TestInfo* test_info = UnitTest::GetInstance()->current_test_info();
        cout << "Test " << test_info->name() << " starts\n";
    }
    void TearDown() override {

    }
};

TEST_F(LabTest, foo) {
    a(50);
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


    return 0;
}