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
#ifndef __INCLUDED_FOUNDATION_OFFLOAD_THREAD_H__
#define __INCLUDED_FOUNDATION_OFFLOAD_THREAD_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/errors.h>

struct work_thread;
struct work_endpoint;

/** \brief Create and allocate a new work thread
 * Create, allocate and initialize a new work thread to be provided to a work pool.
 * \param thread Reference to a pointer to a new work thread structure
 * \param cpu_affinity CPU to pin the thread to
 * \return A_OK on success, an error code otherwise
 */
aresult_t work_thread_new(struct work_thread **thread,
                          unsigned int cpu_affinity);

/** \brief Signal that a work thread should shutdown
 * Update the internal thread signal semaphore to indicate that the given work thread
 * should shut down cleanly as soon as is possible.
 */
aresult_t work_thread_shutdown(struct work_thread *thread);

/** \brief Destroy a work thread
 * Destroy the state associated with a work thread. Fails if the work thread is still
 * active according to the threading subsystem.
 * \param thread Pointer to the work thread structure pointer.
 * \return A_OK on success, an error code otherwise. Returns error from thread context.
 */
aresult_t work_thread_destroy(struct work_thread **thread);

/** \brief Start the given work thread
 *
 */
aresult_t work_thread_start(struct work_thread *thread);

/** \brief Enqueue a work endpoint to be picked up by the thread
 *
 */
aresult_t work_thread_add_endpoint(struct work_thread *thread,
                                   struct work_endpoint *endpoint);

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_FOUNDATION_OFFLOAD_THREAD_H__ */
