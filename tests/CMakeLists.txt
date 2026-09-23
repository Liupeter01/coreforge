cmake_minimum_required(VERSION 3.10)
project(LIBHPC_test LANGUAGES CXX C)

include(FetchContent)

# Sanitizers are useful for correctness testing, but they invalidate benchmark
# timings. Keep Release-family builds uninstrumented and enable them only for
# the Debug configuration. Generator expressions also work with multi-config
# generators such as Xcode and Visual Studio.
if(UNIX AND NOT APPLE)
  message(STATUS "Configuring tests for Linux (sanitizers enabled in Debug)")
  add_compile_options($<$<CONFIG:Debug>:-fsanitize=address,leak>)
  add_link_options($<$<CONFIG:Debug>:-fsanitize=address,leak>)
elseif(APPLE)
  message(STATUS "Configuring tests for macOS (AddressSanitizer enabled in Debug)")
  add_compile_options($<$<CONFIG:Debug>:-fsanitize=address>)
  add_link_options($<$<CONFIG:Debug>:-fsanitize=address>)
elseif(WIN32)
  message(WARNING "Sanitizers not yet configured for Windows Debug builds")
  # ASan for MSVC or clang-cl requires compiler-specific handling.
endif()

FetchContent_Declare(
googletest
GIT_REPOSITORY https://github.com/google/googletest.git
GIT_TAG  v1.18.0
GIT_SHALLOW    ON   
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Universal Package
add_subdirectory(test_hpc_array)
add_subdirectory(test_sparse)
add_subdirectory(test_hpc_benchmark)
add_subdirectory(test_radixsort_cpu)

if(LIBHPC_HAS_CUDA)
  add_subdirectory(test_stall_lg_testsuite)
  add_subdirectory(test_overlap_pipeline_sweep_testsuite)
  add_subdirectory(test_compute_intensity_latency_sweep)
  add_subdirectory(test_mio_throttle_testsuite)
  add_subdirectory(test_stall_short_testsuite)
  add_subdirectory(test_radixsort_gpu)
  add_subdirectory(test_gpu_benchmark)
  add_subdirectory(test_radixsort_gpu_global_offset)
  add_subdirectory(test_radixsort_gpu_local_count)
  add_subdirectory(test_radixsort_gpu_local_offset)
  add_subdirectory(test_sm80_cp_async)
endif()

# Only enable in debug mode!
if(LIBHPC_BUILD_TESTING)
  add_subdirectory(test_hierarchical_lock)
  add_subdirectory(test_threadpool_ref)
  add_subdirectory(test_circular_queue_lk)
  add_subdirectory(test_circular_queue_lockfree)
  add_subdirectory(test_linklist_lk)
  add_subdirectory(test_stack_lockfree)
  add_subdirectory(test_queue_lockfree)
  add_subdirectory(test_single_mempool)
endif()
