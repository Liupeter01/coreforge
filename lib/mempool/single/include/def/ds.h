/*
 * ds.h - A lightweight single-threaded memory pool
 *
 * This implementation is inspired by the design of ngx_palloc.c from the nginx
 * project.
 *
 * Copyright (c) 2025 Jonathan Liu
 * Portions derived from Nginx (https://nginx.org), which is licensed under the
 * BSD-2-Clause License.
 *
 * This file is part of a larger project licensed under the MIT License.
 * See LICENSE for details.
 */

#pragma once
#ifndef _DS_H_
#define _DS_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef void (*ngx_pool_cleanup_pt)(void *data);

struct ngx_pool_cleanup_s {
  ngx_pool_cleanup_pt handler;
  void *data;
  struct ngx_pool_cleanup_s *next;
};

struct ngx_pool_large_s {
  struct ngx_pool_large_s *next;
  void *alloc;
};

typedef struct {
  unsigned char *last;
  unsigned char *end;
  struct ngx_pool_s *next;
  uintptr_t failed;
} ngx_pool_data_t;

struct ngx_pool_s {
  ngx_pool_data_t d;
  size_t max;
  struct ngx_pool_s *current;
  struct ngx_pool_large_s *large;
  struct ngx_pool_cleanup_s *cleanup;
};

#endif //_DS_H_
