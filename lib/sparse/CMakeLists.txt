cmake_minimum_required(VERSION 3.10)

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
  message(
    WARNING "Binary directory of CMake CANNOT be the same as Source Directory!")
endif()

file(GLOB_RECURSE source_file CONFIGURE_DEPENDS src/*.cpp src/*.cc src/*.c)
file(GLOB_RECURSE header_file CONFIGURE_DEPENDS include/*.hpp include/*.h)

project(
  SparseDS
  VERSION 0.1.4
  LANGUAGES C CXX)

add_library(sparse_data_structure STATIC ${source_file} ${header_file})
target_include_directories(sparse_data_structure PUBLIC include)
target_link_libraries(sparse_data_structure PUBLIC libhpc_tbb libhpc_openmp)
set_property(TARGET sparse_data_structure PROPERTY POSITION_INDEPENDENT_CODE ON)
set_target_properties(sparse_data_structure PROPERTIES LINKER_LANGUAGE CXX)
