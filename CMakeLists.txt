cmake_minimum_required(VERSION 3.18)

project(
  libHPC
  VERSION 0.0.1
  LANGUAGES C CXX)

include(cmake/libHPC_preset.cmake)

find_package(OpenMP QUIET)

add_library(libhpc_tbb INTERFACE)
if(LIBHPC_ENABLE_TBB)
  target_link_libraries(libhpc_tbb INTERFACE TBB::tbb)
  target_compile_definitions(libhpc_tbb INTERFACE LIBHPC_USE_TBB=1)
else()
  target_compile_definitions(libhpc_tbb INTERFACE LIBHPC_USE_TBB=0)
endif()

add_library(libhpc_openmp INTERFACE)
if(OpenMP_CXX_FOUND)
  target_link_libraries(libhpc_openmp INTERFACE OpenMP::OpenMP_CXX)
  target_compile_definitions(libhpc_openmp INTERFACE LIBHPC_USE_OPENMP=1)
else()
  target_compile_definitions(libhpc_openmp INTERFACE LIBHPC_USE_OPENMP=0)
  message(STATUS "OpenMP not found: using serial CPU fallbacks")
endif()

if(LIBHPC_HAS_CUDA)
  add_subdirectory(external/pybind11)
endif()

add_subdirectory(lib/sparse)
add_subdirectory(lib/hpc)
add_subdirectory(lib/concurrency)
add_subdirectory(lib/mempool)
add_subdirectory(lib/sort)

if(LIBHPC_HAS_CUDA)
  add_subdirectory(lib/gpu)
endif()

if(LIBHPC_BUILD_TESTING)
  enable_testing()
  add_subdirectory(external/benchmark)
  add_subdirectory(external/libmorton)
  add_subdirectory(tests)
endif()
