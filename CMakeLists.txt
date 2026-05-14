cmake_minimum_required(VERSION 3.18)

include(cmake/libHPC_preset.cmake)

project(
  libHPC
  VERSION 0.0.1
  LANGUAGES C CXX)

add_subdirectory(external/benchmark)
add_subdirectory(external/libmorton)
add_subdirectory(external/pybind11)
add_subdirectory(lib/sparse)
add_subdirectory(lib/hpc)
add_subdirectory(lib/concurrency)
add_subdirectory(lib/mempool)
add_subdirectory(lib/sort)
add_subdirectory(tests)

if(CMAKE_CUDA_COMPILER)
  add_subdirectory(lib/gpu)
endif()

if(LIBHPC_BUILD_TESTING)
  enable_testing()
endif()
