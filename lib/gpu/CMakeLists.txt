cmake_minimum_required(VERSION 3.10)

if(PROJECT_BINARY_DIR EQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

if(CMAKE_CUDA_COMPILER)
  enable_language(CUDA)
  find_package(CUDA REQUIRED)
  include_directories("${CUDA_INCLUDE_DIRS}")
  find_package(CUDAToolkit REQUIRED)

  set(CMAKE_CUDA_ARCHITECTURES 75 86 89 90)
  set(CMAKE_CUDA_STANDARD 17)
  set(CMAKE_CUDA_SEPARABLE_COMPILATION OFF)
  set(CMAKE_CUDA_FLAGS
      "${CMAKE_CUDA_FLAGS} -lineinfo --expt-relaxed-constexpr --expt-extended-lambda"
  )
  # set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -lineinfo --expt-relaxed-constexpr
  # --expt-extended-lambda -Xfatbin=-no-ptx")

  message(STATUS "CUDA toolkit include dirs: ${CUDAToolkit_INCLUDE_DIRS}")
  message(STATUS "CUDA toolkit libraries: ${CUDAToolkit_LIBRARIES}")
else()
  message(ERROR "CUDA not supported!")
  return()
endif()

project(
  gpu
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

if(CMAKE_CUDA_COMPILER)
  add_subdirectory(util)
  add_subdirectory(stall_lg_testsuite)
  add_subdirectory(transfer_overlap_testsuite)
  add_subdirectory(mio_throttle_testsuite)
  add_subdirectory(stall_short_testsuite)
  add_subdirectory(radix_gpu)
  add_subdirectory(operator)
  add_subdirectory(cp_async_copy_testsuite)
endif()
