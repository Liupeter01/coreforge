cmake_minimum_required(VERSION 3.10)
include(GoogleTest)

if(NOT COREFORGE_BUILD_TESTING)
  return()
endif()

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

project(
  test_radixsort_gpu
  VERSION 0.0.1
  LANGUAGES C CXX CUDA)

find_package(CUDA REQUIRED)

file(GLOB TEST_SOURCES *.cc)
add_executable(test_radixsort_gpu ${TEST_SOURCES})

target_include_directories(test_radixsort_gpu PUBLIC include
                                                     ${CUDA_INCLUDE_DIRS})
target_link_libraries(test_radixsort_gpu PRIVATE RadixSortCPU RadixSortGPU
                                                 libCudaUtilities GTest::gtest)
set_property(TARGET test_radixsort_gpu PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(test_radixsort_gpu PROPERTIES LINKER_LANGUAGE CXX)
gtest_discover_tests(test_radixsort_gpu)
