cmake_minimum_required(VERSION 3.19)
project(BidirectionalMap)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wpedantic -Wextra -fsanitize=address")

add_executable(BidirectionalMap main.cpp)