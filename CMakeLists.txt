cmake_minimum_required(VERSION 2.8)

project(qttp)


make_directory(lib)
make_directory(bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_library(HTTPPARSER STATIC include/http_parser.o)
SET_TARGET_PROPERTIES(HTTPPARSER PROPERTIES LINKER_LANGUAGE C)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(src)
include_directories(include)

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
 
add_executable(qttp src/main.cpp src/qttp.cpp src/connection_queue.cpp
  src/connection_pool.cpp src/connection_handler_epoll.cpp src/connection_worker.cpp)
target_link_libraries(qttp pthread)
target_link_libraries(qttp HTTPPARSER)

add_executable(run_tests test/hello_test.cpp src/connection_queue.cpp)
target_link_libraries(run_tests ${GTEST_LIBRARIES} pthread)

