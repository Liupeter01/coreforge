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
  test_mio_throttle_testsuite
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

  # For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

file(GLOB TEST_SOURCES *.cc *.cpp)
set(CUDA_FILE test_mio_throttle_testsuite.cu)

find_package(CUDA REQUIRED)

add_executable(test_mio_throttle_testsuite ${TEST_SOURCES}  ${CUDA_FILE})
target_include_directories(test_mio_throttle_testsuite PUBLIC include  ${CUDA_INCLUDE_DIRS})

target_link_libraries(test_mio_throttle_testsuite PUBLIC libMioThrottleTest GTest::gtest cudart)
gtest_discover_tests(test_mio_throttle_testsuite )
