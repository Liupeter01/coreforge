cmake_minimum_required(VERSION 3.18)

project(
  CoreForge
  VERSION 0.0.1
  LANGUAGES C CXX)

include(cmake/CoreForgePreset.cmake)

find_package(OpenMP QUIET)

add_library(coreforge_tbb INTERFACE)
if(COREFORGE_ENABLE_TBB)
  target_link_libraries(coreforge_tbb INTERFACE TBB::tbb)
  target_compile_definitions(coreforge_tbb INTERFACE COREFORGE_USE_TBB=1)
else()
  target_compile_definitions(coreforge_tbb INTERFACE COREFORGE_USE_TBB=0)
endif()

add_library(coreforge_openmp INTERFACE)
if(OpenMP_CXX_FOUND)
  target_link_libraries(coreforge_openmp INTERFACE OpenMP::OpenMP_CXX)
  target_compile_definitions(coreforge_openmp INTERFACE COREFORGE_USE_OPENMP=1)
else()
  target_compile_definitions(coreforge_openmp INTERFACE COREFORGE_USE_OPENMP=0)
  message(STATUS "OpenMP not found: using serial CPU fallbacks")
endif()

if(COREFORGE_HAS_CUDA)
  add_subdirectory(external/pybind11)
endif()

add_subdirectory(lib/sparse)
add_subdirectory(lib/hpc)
add_subdirectory(lib/concurrency)
add_subdirectory(lib/mempool)
add_subdirectory(lib/sort)

if(COREFORGE_HAS_CUDA)
  add_subdirectory(lib/gpu)
endif()

if(COREFORGE_BUILD_TESTING)
  enable_testing()
  add_subdirectory(external/libmorton)
  add_subdirectory(tests)
endif()

if(COREFORGE_BUILD_BENCHMARK)
  add_subdirectory(external/benchmark)
  add_subdirectory(benchmarks)
endif()
