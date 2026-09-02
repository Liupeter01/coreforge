cmake_minimum_required(VERSION 3.15)

find_package(Threads REQUIRED)

add_executable(test_sparse test_sparse.cpp)
target_link_libraries(test_sparse PRIVATE sparse_data_structure Threads::Threads)

add_test(NAME sparse_portable_backend COMMAND test_sparse)
