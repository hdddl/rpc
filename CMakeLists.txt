cmake_minimum_required(VERSION 3.18)
project(RPC)

set(CMAKE_CXX_STANDARD 20)

include_directories("./include")

add_executable(test_serializer ./test/test_serializer.cpp)

add_executable(test_rpc_client ./test/test_rpc_client.cpp)
target_link_libraries(test_rpc_client -lzmq)

add_executable(test_rpc_server ./test/test_rpc_server.cpp)
target_link_libraries(test_rpc_server -lzmq)
