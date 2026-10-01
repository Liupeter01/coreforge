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
  queue_benchmark
  VERSION 0.0.1
  LANGUAGES C CXX)

file(GLOB TEST_SOURCES *.cc *.cpp)

add_executable(queue_benchmark ${TEST_SOURCES})
target_include_directories(
  queue_benchmark PUBLIC include lib ${LIBHPC_INCLUDE_DIRS}
                         ${CMAKE_SOURCE_DIR}/external/sse2neon)
target_link_libraries(
  queue_benchmark
  PUBLIC ${OpenCV_LIBRARIES} benchmark::benchmark circular_queue queue)

if(MSVC)
    target_compile_options(  queue_benchmark PRIVATE
        /openmp:experimental
    )
endif()
