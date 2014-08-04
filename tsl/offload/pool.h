/*
  Copyright (c) 2013, Phil Vachon <phil@cowpig.ca>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  - Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __INCLUDED_FOUNDATION_OFFLOAD_POOL_H__
#define __INCLUDED_FOUNDATION_OFFLOAD_POOL_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/result.h>

struct work_pool;
struct work_endpoint;

/**
 * Create a basic work pool with no threads
 * \param pool Reference to a pointer to receive the new pool
 */
aresult_t work_pool_create(struct work_pool **pool);

/**
 * Spin up a thread in the work pool, return a thread ID
 * \param pool The work pool to add the work thread to
 * \param core_num The core ID the thread should attach to
 * \param thread_id Return by reference the thread handle
 * \return A_OK on success, an error code otherwise
 */
aresult_t work_pool_add_thread(struct work_pool *pool,
                               unsigned int core_num,
                               unsigned int *thread_id);

/**
 * Add a work endpoint to a work pool on the specified thread
 */
aresult_t work_pool_add_endpoint(struct work_pool *pool,
                                 unsigned int thread_id,
                                 struct work_endpoint *endpoint);

/**
 * Release resources used by the work pool. Only call after signalling
 * a shutdown to all threads with `work_pool_shutdown`
 */
aresult_t work_pool_destroy(struct work_pool **pool);

/**
 * Signal that a work pool should indicate to its threads that they should
 * gracefully shut down.
 */
aresult_t work_pool_shutdown(struct work_pool *pool);

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_FOUNDATION_OFFLOAD_POOL_H__ */

