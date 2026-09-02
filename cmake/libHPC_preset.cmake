# ============================================
# libHPC Build Preset Cross-platform TBB, LLVM, CUDA, ccache guard
# ============================================

cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CheckLanguage)

set(LIBHPC_BUILD_TESTING_DEFAULT ON)
set(LIBHPC_ENABLE_TBB_DEFAULT ON)
set(LIBHPC_ENABLE_CUDA_DEFAULT ON)

if(APPLE)
  #set(LIBHPC_BUILD_TESTING_DEFAULT OFF)
  set(LIBHPC_ENABLE_TBB_DEFAULT OFF)
  set(LIBHPC_ENABLE_CUDA_DEFAULT OFF)
endif()

option(LIBHPC_BUILD_TESTING "Enable libHPC tests and benchmarks"
       ${LIBHPC_BUILD_TESTING_DEFAULT})
option(LIBHPC_ENABLE_TBB "Enable the oneTBB backend"
       ${LIBHPC_ENABLE_TBB_DEFAULT})
option(LIBHPC_ENABLE_CUDA "Enable CUDA modules when a compiler is available"
       ${LIBHPC_ENABLE_CUDA_DEFAULT})

# --------------------------------------------
# Common
# --------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT MSVC)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "CCache enabled: ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
  endif()
endif()

# --------------------------------------------
# Platform Adjustments
# --------------------------------------------

if(WIN32)
  add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
elseif(APPLE)
  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR};${CMAKE_OSX_ARCHITECTURES}"
         LIBHPC_TARGET_ARCHITECTURES)
  if(LIBHPC_TARGET_ARCHITECTURES MATCHES "arm64|aarch64")
    set(LIBHPC_APPLE_SILICON ON)
  else()
    set(LIBHPC_APPLE_SILICON OFF)
  endif()

  if(LIBHPC_ENABLE_TBB OR LIBHPC_ENABLE_CUDA)
    message(STATUS "Apple platform detected: disabling oneTBB and CUDA")
  endif()
  set(LIBHPC_ENABLE_TBB OFF CACHE BOOL "Enable the oneTBB backend" FORCE)
  set(LIBHPC_ENABLE_CUDA OFF CACHE BOOL
      "Enable CUDA modules when a compiler is available" FORCE)
endif()

# ------------------------------------------------------------------
# Disable heavy bench/test libs
# ------------------------------------------------------------------
set(BENCHMARK_ENABLE_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_GMOCK
    OFF
    CACHE BOOL "" FORCE)

set(TBB_TEST
    OFF
    CACHE BOOL "" FORCE)
set(TBB_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(TBB_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(TBB_BENCH
    OFF
    CACHE BOOL "" FORCE)
set(TBB_BENCHMARK
    OFF
    CACHE BOOL "" FORCE)

# --------------------------------------------
# oneTBB Split mode
# --------------------------------------------
if(LIBHPC_ENABLE_TBB)
  FetchContent_Declare(
    TBB
    GIT_REPOSITORY https://github.com/uxlfoundation/oneTBB
    GIT_TAG v2022.1.0
    GIT_SUBMODULES_RECURSE TRUE)

  FetchContent_MakeAvailable(TBB)
endif()

# --------------------------------------------
# CUDA
# --------------------------------------------
set(LIBHPC_HAS_CUDA OFF)
if(LIBHPC_ENABLE_CUDA)
  check_language(CUDA)
  if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(LIBHPC_HAS_CUDA ON)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    # Only compile targeted arch (optional: extend here)
    set(CMAKE_CUDA_ARCHITECTURES 75 86 89 90)
  else()
    message(WARNING "CUDA not detected, GPU module skipped.")
  endif()
endif()
