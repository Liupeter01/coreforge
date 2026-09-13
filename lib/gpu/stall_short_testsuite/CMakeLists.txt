cmake_minimum_required(VERSION 3.18)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

message(INFO
        "In Order To Review The Performance, You Have to use Profiling Tools")

if(NOT CMAKE_CUDA_COMPILER)
  message(WARNING "CUDA not supported! Stall Short Testsuite Disabled!")
  return()
endif()

file(GLOB_RECURSE source_file CONFIGURE_DEPENDS src/*.cpp src/*.cc src/*.c)
file(GLOB_RECURSE header_file CONFIGURE_DEPENDS include/*.hpp include/*.h)

set(CUDA_FILE include/cuda_tut_stall_short.cuh src/dummy.cu)

project(
  libStallShortTest
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

find_package(CUDA REQUIRED)

add_library(libStallShortTest STATIC ${source_file} ${header_file} ${CUDA_FILE})
target_include_directories(libStallShortTest PUBLIC include
                                                    ${CUDA_INCLUDE_DIRS})
target_link_libraries(libStallShortTest PUBLIC libCudaUtilities)
set_property(TARGET libStallShortTest PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(libStallShortTest PROPERTIES LINKER_LANGUAGE CXX)
