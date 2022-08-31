cmake_minimum_required(VERSION 3.22)
project(RPC)

set(CMAKE_CXX_STANDARD 20)

include_directories(".")
include_directories("C:\\Users\\HuangDongliu\\scoop\\apps\\vcpkg\\current\\packages\\zeromq_x64-windows\\include")
include_directories("D:\\Program\\cpplib\\cppzmq")
link_directories(C:\\Users\\HuangDongliu\\scoop\\apps\\vcpkg\\current\\packages\\zeromq_x64-windows\\lib)
link_directories(C:\\Users\\HuangDongliu\\scoop\\apps\\vcpkg\\current\\packages\\zeromq_x64-windows\\bin)

add_executable(test_serializer ./test/test_serializer.cpp)

add_executable(test_rpc_client ./test/test_rpc_client.cpp)
target_link_libraries(test_rpc_client libzmq-mt-4_3_4)

add_executable(test_rpc_server ./test/test_rpc_server.cpp)
target_link_libraries(test_rpc_server libzmq-mt-4_3_4)


