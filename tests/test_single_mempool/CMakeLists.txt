cmake_minimum_required(VERSION 3.10)
project(test_single_mempool LANGUAGES CXX C)

include(GoogleTest)

if(NOT COREFORGE_BUILD_TESTING)
  return()
endif()

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

file(GLOB TEST_SOURCES *.cc)
add_executable(test_single_mempool ${TEST_SOURCES})
target_link_libraries(test_single_mempool PRIVATE single-mempool GTest::gtest)
gtest_discover_tests(test_single_mempool)
