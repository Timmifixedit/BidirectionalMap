cmake_minimum_required(VERSION 3.18)
project(BidirectionalMap)

option(BUILD_TESTS "Build unit tests" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wpedantic -Wextra -fsanitize=address")

if (${BUILD_TESTS})
    add_subdirectory(Test)
endif ()