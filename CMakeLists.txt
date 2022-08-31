cmake_minimum_required(VERSION 3.22)
project(RPC)

set(CMAKE_CXX_STANDARD 20)

add_executable(RPC main.cpp)
add_executable(test_serializer ./test/test_serializer.cpp)

