cmake_minimum_required(VERSION 3.10)
project(LIBHPC_test LANGUAGES CXX C)

include(FetchContent)

# Sanitizers are useful for correctness testing, but they invalidate benchmark
# timings and ASan cannot be combined with TSan. Select exactly one sanitizer
# for Debug builds; Release-family builds remain uninstrumented.
set(LIBHPC_SANITIZER
    "address"
    CACHE STRING "Sanitizer for Debug tests: none, address, undefined, or thread")
set_property(CACHE LIBHPC_SANITIZER PROPERTY STRINGS none address undefined thread)
string(TOLOWER "${LIBHPC_SANITIZER}" LIBHPC_SANITIZER_NORMALIZED)

if(NOT LIBHPC_SANITIZER_NORMALIZED MATCHES "^(none|address|thread|undefined)$")
  message(
    FATAL_ERROR
      "Unsupported LIBHPC_SANITIZER='${LIBHPC_SANITIZER}'. Use none, address, undefined, or thread."
  )
endif()

set(LIBHPC_SANITIZER_FLAG "")
if(UNIX AND NOT APPLE)
  if(LIBHPC_SANITIZER_NORMALIZED STREQUAL "address")
    set(LIBHPC_SANITIZER_FLAG -fsanitize=address,leak)
  elseif(LIBHPC_SANITIZER_NORMALIZED STREQUAL "thread")
    set(LIBHPC_SANITIZER_FLAG -fsanitize=thread)
  elseif(LIBHPC_SANITIZER_NORMALIZED STREQUAL "undefined")
    set(LIBHPC_SANITIZER_FLAG -fsanitize=undefined)
  endif()
elseif(APPLE)
  if(LIBHPC_SANITIZER_NORMALIZED STREQUAL "address")
    set(LIBHPC_SANITIZER_FLAG -fsanitize=address)
  elseif(LIBHPC_SANITIZER_NORMALIZED STREQUAL "thread")
    set(LIBHPC_SANITIZER_FLAG -fsanitize=thread)
  elseif(LIBHPC_SANITIZER_NORMALIZED STREQUAL "undefined")
    set(LIBHPC_SANITIZER_FLAG -fsanitize=undefined)
  endif()
elseif(NOT LIBHPC_SANITIZER_NORMALIZED STREQUAL "none")
  message(WARNING "Sanitizers are not configured for this platform")
endif()

if(LIBHPC_SANITIZER_FLAG)
  message(
    STATUS
      "Configuring Debug tests with ${LIBHPC_SANITIZER_NORMALIZED} sanitizer")
  add_compile_options($<$<CONFIG:Debug>:${LIBHPC_SANITIZER_FLAG}>
                      $<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
  add_link_options($<$<CONFIG:Debug>:${LIBHPC_SANITIZER_FLAG}>)
else()
  message(STATUS "Configuring tests without sanitizers")
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
