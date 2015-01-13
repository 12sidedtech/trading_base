/*
  Copyright (c) 2014, 12Sided Technology, LLC
  Author: Phil Vachon <pvachon@12sidedtech.com>
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
#include <tsl/worker_thread.h>

#include <tsl/threading.h>
#include <tsl/diag.h>
#include <tsl/assert.h>
#include <tsl/errors.h>
#include <tsl/cpumask.h>
#include <tsl/threading.h>
#include <tsl/panic.h>

#include <pthread.h>
#include <string.h>

static
aresult_t __worker_thread_function(void *params)
{
    aresult_t ret = A_OK;
    struct worker_thread *wt = NULL;

    TSL_ASSERT_ARG(NULL != params);

    wt = params;

    if (false == ck_pr_cas_int(&wt->st, WORKER_THREAD_STATE_STARTING_UP, WORKER_THREAD_STATE_RUNNING)) {
        DIAG("Shutdown of work thread requested before it even started.");
        goto done;
    }

    ret = wt->work_func(params);

done:
    DIAG("Worker thread shutting down.");
    ck_pr_store_int(&wt->st, WORKER_THREAD_STATE_SHUTDOWN);
    return ret;
}

aresult_t worker_thread_new_mask(struct worker_thread *thr, worker_thread_work_func_t work_func, struct cpu_mask **pmsk)
{
    aresult_t ret = A_OK;

    struct cpu_mask *msk = NULL;

    TSL_ASSERT_ARG(NULL != thr);
    TSL_ASSERT_ARG(NULL != work_func);
    TSL_ASSERT_ARG(NULL != pmsk);
    TSL_ASSERT_ARG(NULL != *pmsk);

    msk = *pmsk;

    memset(thr, 0, sizeof(*thr));

    thr->work_func = work_func;

    thr->st = WORKER_THREAD_STATE_STARTING_UP;

    if (AFAILED(ret = thread_create(&thr->thr, __worker_thread_function, msk))) {
        DIAG("Failed to spin up a work thread for the command interface.");
        goto done;
    }

    if (AFAILED(ret = thread_start(thr->thr, thr))) {
        DIAG("Failed to start worker thread.");
        goto done;
    }

    *pmsk = NULL;

done:
    if (AFAILED(ret)) {
        if (NULL != thr->thr) {
            thread_destroy(&thr->thr);
        }
    }

    return ret;
}

aresult_t worker_thread_new(struct worker_thread *thr, worker_thread_work_func_t work_func, unsigned int cpu_core)
{
    aresult_t ret = A_OK;
    struct cpu_mask *msk = NULL;

    TSL_ASSERT_ARG(NULL != thr);
    TSL_ASSERT_ARG(NULL != work_func);

    if (AFAILED(ret = cpu_mask_create(&msk))) {
        goto done;
    }

    if (AFAILED(ret = cpu_mask_set(msk, cpu_core))) {
        goto done;
    }

    if (AFAILED(ret = worker_thread_new_mask(thr, work_func, &msk))) {
        goto done;
    }

done:
    if (AFAILED(ret)) {
        if (NULL != msk) {
            cpu_mask_destroy(&msk);
        }
    }

    return ret;
}

aresult_t worker_thread_request_shutdown(struct worker_thread *thr)
{
    aresult_t ret = A_OK;
    int state = 0;

    TSL_ASSERT_ARG(NULL != thr);

    state = ck_pr_load_int(&thr->st);

    if (WORKER_THREAD_STATE_RUNNING == state || WORKER_THREAD_STATE_STARTING_UP == state) {
        ck_pr_store_int(&thr->st, WORKER_THREAD_STATE_SHUTDOWN_REQUESTED);
    }

    return ret;
}

aresult_t worker_thread_delete(struct worker_thread *thr)
{
    aresult_t ret = A_OK;

    int state = 0;

    TSL_ASSERT_ARG(NULL != thr);

    state = ck_pr_load_int(&thr->st);

    if (!(WORKER_THREAD_STATE_SHUTDOWN == state || WORKER_THREAD_STATE_SHUTDOWN_REQUESTED == state) ) {
        ret = A_E_BUSY;
        goto done;
    }

    if (NULL != thr->thr) {
        aresult_t thr_ret = A_OK;

        if (AFAILED(ret = thread_join(thr->thr, &thr_ret))) {
            DIAG("Failed to join worker thread to parent thread.");
            goto done;
        }

        /* Now that the thread has been JOIN'd to the parent, destroy its resources */
        thread_destroy(&thr->thr);
    }

done:
    return ret;
}

