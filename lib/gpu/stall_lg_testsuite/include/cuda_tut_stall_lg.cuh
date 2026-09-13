#pragma once
#ifndef _CUDA_TUT_STALL_LG_HPP_
#define _CUDA_TUT_STALL_LG_HPP_
#include <cuda_runtime.h>
#include <iostream>

__global__ void stall_lg_worse(const int8_t *__restrict__ ptr1,
                               int8_t *__restrict__ ptr2);
__global__ void stall_lg_coalesced_32(const int8_t *__restrict__ ptr1,
                                      int8_t *__restrict__ ptr2);
__global__ void stall_lg_coalesced_128(const int8_t *__restrict__ ptr1,
                                       int8_t *__restrict__ ptr2);
__global__ void stall_lg_coalesced_512(const int8_t *__restrict__ ptr1,
                                       int8_t *__restrict__ ptr2);

#endif //_CUDA_TUT_STALL_LG_HPP_
