cmake_minimum_required(VERSION 3.18)

include(${CMAKE_SOURCE_DIR}/cmake/TorchSetup.cmake)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

message(INFO
        "In Order To Review The Performance, You Have to use Profiling Tools")

if(NOT CMAKE_CUDA_COMPILER)
  message(WARNING "CUDA not supported! Stall LG Testsuite Disabled!")
  return()
endif()

file(GLOB_RECURSE source_file CONFIGURE_DEPENDS src/*.cpp src/*.cc src/*.c)
file(GLOB_RECURSE header_file CONFIGURE_DEPENDS include/*.hpp include/*.h)
file(GLOB_RECURSE cuda_file CONFIGURE_DEPENDS include/*.cuh src/*.cu)

project(
  cuda_operator
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

find_package(CUDA REQUIRED)

add_library(cuda_operator STATIC ${source_file} ${header_file} ${cuda_file})
target_include_directories(cuda_operator PUBLIC include ${CUDA_INCLUDE_DIRS})
target_link_libraries(cuda_operator PUBLIC libCudaUtilities ${TORCH_LIBRARIES})
set_property(TARGET cuda_operator PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(cuda_operator PROPERTIES LINKER_LANGUAGE CXX)
