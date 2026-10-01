cmake_minimum_required(VERSION 3.10)
project(LIBHPC_benchmark LANGUAGES CXX C)

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

if(LIBHPC_HAS_CUDA)
  add_subdirectory(gpu_benchmark)
endif()

add_subdirectory(hpc_benchmark)
add_subdirectory(queue_benchmark)
