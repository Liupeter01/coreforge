#pragma once

#ifndef LIBHPC_USE_OPENMP
#define LIBHPC_USE_OPENMP 0
#endif

#if LIBHPC_USE_OPENMP
#include <omp.h>
#else
inline int omp_get_max_threads() noexcept { return 1; }
inline int omp_get_thread_num() noexcept { return 0; }
inline void omp_set_num_threads(int) noexcept {}
#endif
