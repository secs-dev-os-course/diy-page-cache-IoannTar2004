cmake_minimum_required(VERSION 3.22)
project(lab2 CXX)
set(CMAKE_CXX_STANDARD 23)

set(TESTS tests)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")

file(GLOB SOURCE_FILES "src/cacheAPI/*.cpp")
file(GLOB HEADER_FILES "src/cacheAPI/*.h")
add_library(myCache SHARED ${SOURCE_FILES} ${HEADER_FILES})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
find_package(GTest REQUIRED)

file(GLOB TEST_SOURCE_FILES "src/**/*.cpp")
file(GLOB TEST_HEADER_FILES "src/**/*.h")
list(FILTER TEST_SOURCE_FILES EXCLUDE REGEX ".*/main.*\\.cpp$")

add_executable(${TESTS} test/test.cpp ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES})
target_link_libraries(${TESTS} ${GTEST_LIBRARIES} pthread)