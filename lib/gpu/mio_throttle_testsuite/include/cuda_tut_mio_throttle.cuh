#pragma once
#ifndef _CUDA_TUT_MIO_THROTTLE_CUH_
#define _CUDA_TUT_MIO_THROTTLE_CUH_
#include <cuda_runtime.h>

__global__ void stall_mio_worse();
__global__ void stall_mio_better();
__global__ void stall_mio_good();

#endif //_CUDA_TUT_MIO_THROTTLE_CUH_
