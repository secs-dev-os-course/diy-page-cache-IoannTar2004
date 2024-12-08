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
    const char *filename = "../example.txt";  // Имя файла
    int fd = open(filename, O_RDWR);        // Открытие файла для записи и чтения
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // Получаем размер файла
    off_t filesize = lseek(fd, 0, SEEK_END);

    // Отображаем файл в память
    void *mapped_data = mmap(NULL, 60, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Теперь данные файла отображены в память, и мы можем их изменять
    printf("Original content: %s\n", (char *)mapped_data);
    
    // Изменяем данные в памяти
    ftruncate(fd, strlen("Hello, mmap!") + 1);
    strcpy((char *)mapped_data, "Hello, mmap!");

    // Данные сразу записываются обратно в файл, так как используется MAP_SHARED
    printf("Modified content: %s\n", (char *)mapped_data);

    // Освобождаем память, когда больше не нужно
    if (munmap(mapped_data, filesize) == -1) {
        perror("munmap");
    }

    // Закрываем файл
    close(fd);

    return 0;
}