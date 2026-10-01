cmake_minimum_required(VERSION 3.10)
project(CoreForge_tests LANGUAGES CXX C)

include(FetchContent)

# Sanitizers are useful for correctness testing, but they invalidate benchmark
# timings and ASan cannot be combined with TSan. Select exactly one sanitizer
# for Debug builds; Release-family builds remain uninstrumented.
set(COREFORGE_SANITIZER
    "address"
    CACHE STRING "Sanitizer for Debug tests: none, address, undefined, or thread")
set_property(CACHE COREFORGE_SANITIZER PROPERTY STRINGS none address undefined thread)
string(TOLOWER "${COREFORGE_SANITIZER}" COREFORGE_SANITIZER_NORMALIZED)

if(NOT COREFORGE_SANITIZER_NORMALIZED MATCHES "^(none|address|thread|undefined)$")
  message(
    FATAL_ERROR
      "Unsupported COREFORGE_SANITIZER='${COREFORGE_SANITIZER}'. Use none, address, undefined, or thread."
  )
endif()

set(COREFORGE_SANITIZER_FLAG "")
if(UNIX AND NOT APPLE)
  if(COREFORGE_SANITIZER_NORMALIZED STREQUAL "address")
    set(COREFORGE_SANITIZER_FLAG -fsanitize=address,leak)
  elseif(COREFORGE_SANITIZER_NORMALIZED STREQUAL "thread")
    set(COREFORGE_SANITIZER_FLAG -fsanitize=thread)
  elseif(COREFORGE_SANITIZER_NORMALIZED STREQUAL "undefined")
    set(COREFORGE_SANITIZER_FLAG -fsanitize=undefined)
  endif()
elseif(APPLE)
  if(COREFORGE_SANITIZER_NORMALIZED STREQUAL "address")
    set(COREFORGE_SANITIZER_FLAG -fsanitize=address)
  elseif(COREFORGE_SANITIZER_NORMALIZED STREQUAL "thread")
    set(COREFORGE_SANITIZER_FLAG -fsanitize=thread)
  elseif(COREFORGE_SANITIZER_NORMALIZED STREQUAL "undefined")
    set(COREFORGE_SANITIZER_FLAG -fsanitize=undefined)
  endif()
elseif(NOT COREFORGE_SANITIZER_NORMALIZED STREQUAL "none")
  message(WARNING "Sanitizers are not configured for this platform")
endif()

if(COREFORGE_SANITIZER_FLAG)
  message(
    STATUS
      "Configuring Debug tests with ${COREFORGE_SANITIZER_NORMALIZED} sanitizer")
  add_compile_options($<$<CONFIG:Debug>:${COREFORGE_SANITIZER_FLAG}>
                      $<$<CONFIG:Debug>:-fno-omit-frame-pointer>)
  add_link_options($<$<CONFIG:Debug>:${COREFORGE_SANITIZER_FLAG}>)
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
add_subdirectory(test_radixsort_cpu)

if(COREFORGE_HAS_CUDA)
  add_subdirectory(test_stall_lg_testsuite)
  add_subdirectory(test_overlap_pipeline_sweep_testsuite)
  add_subdirectory(test_compute_intensity_latency_sweep)
  add_subdirectory(test_mio_throttle_testsuite)
  add_subdirectory(test_stall_short_testsuite)
  add_subdirectory(test_radixsort_gpu)
  add_subdirectory(test_radixsort_gpu_global_offset)
  add_subdirectory(test_radixsort_gpu_local_count)
  add_subdirectory(test_radixsort_gpu_local_offset)
  add_subdirectory(test_sm80_cp_async)
endif()

# Only enable in debug mode!
if(COREFORGE_BUILD_TESTING)
  add_subdirectory(test_hierarchical_lock)
  add_subdirectory(test_threadpool_ref)
  add_subdirectory(test_circular_queue_lk)
  add_subdirectory(test_circular_queue_lockfree)
  add_subdirectory(test_linklist_lk)
  add_subdirectory(test_stack_lockfree)
  add_subdirectory(test_queue_lockfree)
  add_subdirectory(test_single_mempool)
endif()
