# cmake/TorchSetup.cmake
include_guard(GLOBAL)
include(ExternalProject)

option(USE_PYTHON_TORCH "Select PyTorch Installed in Python Env By Default" ON)
set(LIBTORCH_VERSION "2.11.0" CACHE STRING "LibTorch Version(For Fallback Download)")
set(TORCH_CUDA_VARIANT "cu130" CACHE STRING "cu126/cu128/cu130/cpu(For Fallback Download)")
option(USE_DEBUG_TORCH "Download Debug Version's' libTorch(Windows Platform Only)" OFF)
option(USE_PYBIND11 "Enable pybind11 Supported Python extension" ON)

if(USE_PYTHON_TORCH)
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -c "import torch; print(torch.utils.cmake_prefix_path)"
      RESULT_VARIABLE _python_torch_res
      OUTPUT_VARIABLE TORCH_PREFIX
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    if(_python_torch_res EQUAL 0 AND TORCH_PREFIX)
      list(PREPEND CMAKE_PREFIX_PATH ${TORCH_PREFIX})
      message(STATUS "PyTorch inside Python has been found: ${TORCH_PREFIX}")
    endif()
  endif()
endif()

find_package(Torch QUIET)

if(NOT TORCH_FOUND)
  message(STATUS "Python PyTorch Not Found → Auto Download LibTorch ${LIBTORCH_VERSION}+${TORCH_CUDA_VARIANT}")

  include(ExternalProject)

  # 平台后缀
  if(WIN32)
    set(PLATFORM_SUFFIX "-win")
    
    # ---------- Debug suffix(ONLY for windows) ----------
    if(USE_DEBUG_TORCH)
      set(BUILD_TYPE "debug")
    else()
      set(BUILD_TYPE "shared-with-deps")
    endif()
  elseif(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(PLATFORM_SUFFIX "-macos-arm64")
    set(TORCH_CUDA_VARIANT "cpu")            # macOS ARM 无 CUDA
    set(BUILD_TYPE "shared-with-deps")
  else()
    set(PLATFORM_SUFFIX "")
    set(BUILD_TYPE "shared-with-deps")
  endif()

# ---------- generate url ----------
  if(TORCH_CUDA_VARIANT STREQUAL "cpu")
    set(BASE_URL "https://download.pytorch.org/libtorch/cpu")
    set(FILENAME "libtorch${PLATFORM_SUFFIX}-${BUILD_TYPE}-${LIBTORCH_VERSION}.zip")
  else()
    set(BASE_URL "https://download.pytorch.org/libtorch/${TORCH_CUDA_VARIANT}")
    set(FILENAME "libtorch${PLATFORM_SUFFIX}-${BUILD_TYPE}-${LIBTORCH_VERSION}%2B${TORCH_CUDA_VARIANT}.zip")
  endif()

  set(LIBTORCH_URL "${BASE_URL}/${FILENAME}")

  ExternalProject_Add(libtorch
    URL                       ${LIBTORCH_URL}
    PREFIX                    ${CMAKE_BINARY_DIR}/_deps/libtorch
    CONFIGURE_COMMAND         ""
    BUILD_COMMAND             ""
    INSTALL_COMMAND           ""
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  ExternalProject_Get_Property(libtorch SOURCE_DIR)
  list(PREPEND CMAKE_PREFIX_PATH ${SOURCE_DIR})

  message(STATUS "Download URL: ${LIBTORCH_URL}")
  message(STATUS "Download Path: ${SOURCE_DIR}")
endif()

find_package(CUDAToolkit REQUIRED)

# ----------------------- Fix CUDA 12+ nvToolsExt Problem ----------------------------
if(NOT TARGET CUDA::nvToolsExt)
  if(TARGET CUDA::nvtx3)
    # CUDA 12: rename nvtx3's alias as nvToolsExt
    add_library(CUDA::nvToolsExt ALIAS CUDA::nvtx3)
    message(STATUS "CUDA::nvToolsExt ALIAS as CUDA::nvtx3")
  else()
    #  fallback link manually
    add_library(CUDA::nvToolsExt INTERFACE IMPORTED)
    target_link_libraries(CUDA::nvToolsExt INTERFACE "${CUDAToolkit_LIBRARY_DIR}/nvToolsExt64_1.lib")
    message(STATUS "CUDA::nvToolsExt INTERFACE target")
  endif()
endif()

find_package(Torch REQUIRED)
message(STATUS "Torch Setup Complete! Version: ${TORCH_VERSION}")
message(STATUS "Torch Libraries: ${TORCH_LIBRARIES}")
message(STATUS "Torch Include Dirs: ${TORCH_INCLUDE_DIRS}")

if(USE_PYBIND11)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python Development Not Found!")
  endif()

  message(STATUS "Python Development Found: ${Python3_INCLUDE_DIRS}")
endif()