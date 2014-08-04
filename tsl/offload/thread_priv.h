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
#ifndef __INCLUDED_FOUNDATION_OFFLOAD_THREAD_PRIV_H__
#define __INCLUDED_FOUNDATION_OFFLOAD_THREAD_PRIV_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/list.h>

#include <tsl/offload/queue.h>

struct thread;

#define WORK_THREAD_MAX_QUEUED_ENDPOINTS            32

/* Flags used for indicating the state of the thread */
#define WORK_THREAD_ALIVE                           0
#define WORK_THREAD_DEAD                            1
#define WORK_THREAD_FAST_SHUTDOWN                   2

/** \brief Work thread state
 * Structure containing work thread state, associated queues and
 * and list of work endpoints. 
 */
struct work_thread {
    struct thread *thread;                          /** Actual thread state */
    unsigned int thread_id;                         /** Thread ID used by offload */
    struct work_queue ep_queue;                     /** Queue for passing new work endpoints */

    /* These fields can be shared between threads, so align to a cache line */
    struct list_entry work_endpoints CAL_ALIGN(64); /** List of work endpoints */
    uint32_t signal_death CAL_ALIGN(64);            /** Death semaphore */

    struct list_entry tnode CAL_ALIGN(64);          /** Used for managing lists of work threads */
};

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_FOUNDATION_OFFLOAD_THREAD_PRIV_H__ */

