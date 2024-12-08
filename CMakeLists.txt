cmake_minimum_required(VERSION 3.22)
project(lab2 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 99)

set(TESTS tests)

file(GLOB SOURCE_FILES "src/cacheAPI/*.c")
file(GLOB HEADER_FILES "src/cacheAPI/*.h")
add_library(myCache SHARED ${SOURCE_FILES} ${HEADER_FILES})

function(add_module target_name target_dir)
    file(GLOB SOURCE_FILES "${target_dir}/*.c" "${SOURCE}/utils/*.c")
    file(GLOB HEADER_FILES "${target_dir}/*.h" "${SOURCE}/utils/*.h")
    add_executable(${target_name} ${SOURCE_FILES} ${HEADER_FILES})
endfunction()

add_module(cacheManager "src/cacheManager")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()
find_package(GTest REQUIRED)

file(GLOB TEST_SOURCE_FILES "src/**/*.c")
file(GLOB TEST_HEADER_FILES "src/**/*.h")
list(FILTER TEST_SOURCE_FILES EXCLUDE REGEX ".*/main_.*\\.c$")

add_executable(${TESTS} test/test.cpp ${TEST_SOURCE_FILES} ${TEST_HEADER_FILES})
target_link_libraries(${TESTS} ${GTEST_LIBRARIES} pthread)