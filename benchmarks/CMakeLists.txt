cmake_minimum_required(VERSION 3.10)
project(CoreForge_benchmarks LANGUAGES CXX C)

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

if(COREFORGE_HAS_CUDA)
  add_subdirectory(gpu_benchmark)
endif()

add_subdirectory(hpc_benchmark)
add_subdirectory(queue_benchmark)
