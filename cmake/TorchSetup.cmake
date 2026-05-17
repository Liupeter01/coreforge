# cmake/TorchSetup.cmake
include_guard(GLOBAL)

option(USE_TORCH "Enable LibTorch / PyTorch support" OFF)
option(USE_PYTHON_TORCH "Prefer PyTorch from Python environment" OFF)
option(USE_PYBIND11 "Enable pybind11 Python extension" OFF)
option(USE_DEBUG_TORCH "Download debug LibTorch (Windows only)" OFF)

set(LIBTORCH_VERSION
    "2.11.0"
    CACHE STRING "LibTorch version for fallback download")
set(TORCH_CUDA_VARIANT
    "cu130"
    CACHE STRING "cu126/cu128/cu130/cpu for fallback download")

# ---------------------------------------------------------------------------
function(setup_torch)
  # --- 1. Try Python environment first ---
  if(USE_PYTHON_TORCH)
    find_package(
      Python3
      COMPONENTS Interpreter
      QUIET)
    if(Python3_FOUND)
      execute_process(
        COMMAND ${Python3_EXECUTABLE} -c
                "import torch; print(torch.utils.cmake_prefix_path)"
        RESULT_VARIABLE _res
        OUTPUT_VARIABLE _prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
      if(_res EQUAL 0 AND _prefix)
        list(PREPEND CMAKE_PREFIX_PATH "${_prefix}")
        set(CMAKE_PREFIX_PATH
            "${CMAKE_PREFIX_PATH}"
            PARENT_SCOPE)
        message(STATUS "[Torch] Found PyTorch in Python env: ${_prefix}")
      endif()
    endif()
  endif()

  find_package(Torch QUIET)
  if(TORCH_FOUND)
    _torch_propagate_to_parent()
    return()
  endif()

  # --- 2. Fallback: download LibTorch ---
  message(STATUS "[Torch] Python PyTorch not found ¡ú downloading LibTorch "
                 "${LIBTORCH_VERSION}+${TORCH_CUDA_VARIANT}")
  _torch_download_libtorch()

  # --- 3. CUDA toolkit + nvToolsExt shim ---
  find_package(CUDAToolkit REQUIRED)
  _torch_fix_nvtoolsext()

  # --- 4. Final find (REQUIRED) ---
  find_package(Torch REQUIRED)
  _torch_propagate_to_parent()
endfunction()

# ---------------------------------------------------------------------------
# Internal helpers (prefixed with _ to signal private use)
# ---------------------------------------------------------------------------
function(_torch_download_libtorch)
  include(ExternalProject)

  # Platform suffix + build type
  if(WIN32)
    set(_suffix "-win")
    if(USE_DEBUG_TORCH)
      set(_build "debug")
    else()
      set(_build "shared-with-deps")
    endif()
  elseif(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(_suffix "-macos-arm64")
    set(TORCH_CUDA_VARIANT
        "cpu"
        PARENT_SCOPE) # no CUDA on Apple Silicon
    set(TORCH_CUDA_VARIANT "cpu")
    set(_build "shared-with-deps")
  else()
    set(_suffix "")
    set(_build "shared-with-deps")
  endif()

  # URL
  if(TORCH_CUDA_VARIANT STREQUAL "cpu")
    set(_url
        "https://download.pytorch.org/libtorch/cpu/libtorch${_suffix}-${_build}-${LIBTORCH_VERSION}.zip"
    )
  else()
    set(_url
        "https://download.pytorch.org/libtorch/${TORCH_CUDA_VARIANT}/libtorch${_suffix}-${_build}-${LIBTORCH_VERSION}%2B${TORCH_CUDA_VARIANT}.zip"
    )
  endif()

  ExternalProject_Add(
    libtorch
    URL "${_url}"
    PREFIX "${CMAKE_BINARY_DIR}/_deps/libtorch"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "" DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

  ExternalProject_Get_Property(libtorch SOURCE_DIR)
  list(PREPEND CMAKE_PREFIX_PATH "${SOURCE_DIR}")
  set(CMAKE_PREFIX_PATH
      "${CMAKE_PREFIX_PATH}"
      PARENT_SCOPE)

  message(STATUS "[Torch] Download URL : ${_url}")
  message(STATUS "[Torch] Download path: ${SOURCE_DIR}")
endfunction()

# ---------------------------------------------------------------------------
function(_torch_fix_nvtoolsext)
  if(TARGET CUDA::nvToolsExt)
    return()
  endif()

  if(TARGET CUDA::nvtx3)
    add_library(CUDA::nvToolsExt ALIAS CUDA::nvtx3)
    message(STATUS "[Torch] CUDA::nvToolsExt ¡ú alias to CUDA::nvtx3")
  else()
    add_library(CUDA::nvToolsExt INTERFACE IMPORTED)
    target_link_libraries(
      CUDA::nvToolsExt
      INTERFACE "${CUDAToolkit_LIBRARY_DIR}/nvToolsExt64_1.lib")
    message(STATUS "[Torch] CUDA::nvToolsExt ¡ú manual interface target")
  endif()
endfunction()

# ---------------------------------------------------------------------------
macro(_torch_propagate_to_parent)
  set(TORCH_FOUND
      "${TORCH_FOUND}"
      PARENT_SCOPE)
  set(TORCH_LIBRARIES
      "${TORCH_LIBRARIES}"
      PARENT_SCOPE)
  set(TORCH_INCLUDE_DIRS
      "${TORCH_INCLUDE_DIRS}"
      PARENT_SCOPE)
  set(CMAKE_PREFIX_PATH
      "${CMAKE_PREFIX_PATH}"
      PARENT_SCOPE)

  message(STATUS "[Torch] Version : ${TORCH_VERSION}")
  message(STATUS "[Torch] Libraries : ${TORCH_LIBRARIES}")
  message(STATUS "[Torch] Includes  : ${TORCH_INCLUDE_DIRS}")

  # Optional pybind11
  if(USE_PYBIND11)
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    message(STATUS "[Torch] Python dev: ${Python3_INCLUDE_DIRS}")
  endif()
endmacro()

# ===========================================================================
# Entry point ¡ª only runs when the gate is ON
# ===========================================================================
if(USE_TORCH)
  setup_torch()
else()
  message(STATUS "[Torch] Disabled (USE_TORCH=OFF)")
endif()
