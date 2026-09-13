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
  test_stall_short_testsuite
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

file(GLOB_RECURSE source_file CONFIGURE_DEPENDS src/*.cpp src/*.cc src/*.c)
file(GLOB_RECURSE header_file CONFIGURE_DEPENDS include/*.hpp include/*.h)

set(CUDA_FILE src/baseline.cu src/dim3.cu src/dim3_shared.cu src/solve_conflict.cu include/common.cuh src/register.cu) 
find_package(CUDA REQUIRED)

add_executable(test_stall_short_testsuite ${source_file} ${header_file} ${CUDA_FILE})
target_include_directories(test_stall_short_testsuite PUBLIC include)

target_link_libraries(test_stall_short_testsuite PUBLIC libStallShortTest libCudaUtilities)
