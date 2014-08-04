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
#include <tsl/offload/pool.h>
#include <tsl/offload/thread.h>

#include <tsl/offload/thread_priv.h>

#include <tsl/list.h>
#include <tsl/result.h>
#include <tsl/assert.h>
#include <tsl/diag.h>


struct work_pool {
    struct list_entry work_threads;
    unsigned int last_thread_id;
};

aresult_t work_pool_create(struct work_pool **pool)
{
    struct work_pool *new_pool = NULL;
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(pool != NULL);

    *pool = NULL;

    new_pool = (struct work_pool *)calloc(1, sizeof(*new_pool));

    if (new_pool == NULL) {
        ret = A_E_NOMEM;
        goto done;
    }

    list_init(&new_pool->work_threads);

    new_pool->last_thread_id = 1;

    *pool = new_pool;

done:
    return ret;
}

aresult_t work_pool_add_thread(struct work_pool *pool,
                               unsigned int core_num,
                               unsigned int *thread_id)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(pool != NULL);
    TSL_ASSERT_ARG(thread_id != NULL);

    *thread_id = 0;

    struct work_thread *new_thread = NULL;

    /* Create a new work thread */
    ret = work_thread_new(&new_thread, core_num);

    if (AFAILED(ret)) {
        DIAG("Failed to create new worker thread.");
        goto done;
    }

    /* Spin up the new thread */
    ret = work_thread_start(new_thread);

    if (AFAILED(ret)) {
        DIAG("Failed to start worker thread!");
        goto done;
    }

    /* Set the thread ID and add to list of worker threads */
    new_thread->thread_id = pool->last_thread_id++;
    list_append(&pool->work_threads, &new_thread->tnode);

    *thread_id = new_thread->thread_id;

done:
    if (AFAILED(ret)) {
        if (new_thread) {
            work_thread_destroy(&new_thread);
        }
    }
    return ret;
}

static
struct work_thread *__work_pool_find_thread_by_id(struct work_pool *pool,
                                                  unsigned int thread_id)
{
    struct work_thread *thread = NULL;

    list_for_each_type(thread, &pool->work_threads, tnode) {
        if (thread->thread_id == thread_id) {
            return thread;
        }
    }

    return NULL;
}

aresult_t work_pool_add_endpoint(struct work_pool *pool,
                                 unsigned int thread_id,
                                 struct work_endpoint *endpoint)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(pool != NULL);
    TSL_ASSERT_ARG(endpoint != NULL);
    TSL_ASSERT_ARG(thread_id < pool->last_thread_id);

    struct work_thread *thr = __work_pool_find_thread_by_id(pool, thread_id);

    if (thr == NULL) {
        ret = A_E_NOTFOUND;
        goto done;
    }

    ret = work_thread_add_endpoint(thr, endpoint);

done:
    return ret;
}

aresult_t work_pool_destroy(struct work_pool **pool)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(pool != NULL);
    TSL_ASSERT_ARG(*pool != NULL);

    struct work_thread *thread = NULL;
    struct work_pool *ppool = *pool;

    /* Reap each thread */
    struct work_thread *temp = NULL;
    list_for_each_type_safe(thread, temp, &ppool->work_threads, tnode) {
        int success = 0;
        for (int i = 0; i < 100000; ++i) {
            list_del(&thread->tnode);
            if (!AFAILED(work_thread_destroy(&thread))) {
                success = 1;
                break;
            }
        }

        if (!success) {
            /* TODO: signal a forced shutdown to the thread */
            DIAG("Failed to shut down worker thread %u", thread->thread_id);
        }
    }

    memset(ppool, 0, sizeof(*ppool));
    free(ppool);

    *pool = NULL;

    return ret;
}

aresult_t work_pool_shutdown(struct work_pool *pool)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(pool != NULL);

    struct work_thread *thread = NULL;

    /* Signal to each thread it should shut down */
    list_for_each_type(thread, &pool->work_threads, tnode) {
        if (AFAILED(ret = work_thread_shutdown(thread))) {
            DIAG("Failed to shut down worker thread...");
        }
    }

    return ret;
}

