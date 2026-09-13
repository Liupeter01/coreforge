cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

project(
  test_stall_lg_testsuite
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

file(GLOB TEST_SOURCES *.cc *.cpp)
set(CUDA_FILE test_stall_lg_testsuite.cu)

add_executable(test_stall_lg_testsuite ${TEST_SOURCES} ${CUDA_FILE})
target_include_directories(test_stall_lg_testsuite PUBLIC include)

target_link_libraries(test_stall_lg_testsuite PUBLIC libStallLGTest libCudaUtilities)
