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
#include <tsl/offload/thread.h>
#include <tsl/offload/thread_priv.h>
#include <tsl/offload/endpoint.h>
#include <tsl/offload/queue.h>

#include <tsl/threading.h>
#include <tsl/cpumask.h>
#include <tsl/cal.h>
#include <tsl/list.h>
#include <tsl/assert.h>
#include <tsl/time.h>

#include <ck_pr.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>


/** \brief Signal shutdown to work thread
 * Signal to a running work thread that it should shut down, using a thread-safe
 * semaphore mechanism.
 * \param thread The work thread to signal shutdown to
 * \return A_OK on success, an error otherwise
 */
static
void __work_thread_signal_to_die(struct work_thread *thread)
{
    ck_pr_store_32(&thread->signal_death, WORK_THREAD_DEAD);    
}

/** \brief Signal fast shutdown to work thread
 * Signal to a running work thread that it should shut down immediately,
 * using a thread-safe semaphore mechanism.
 * \param thread The work thread to signal shutdown to
 * \return A_OK on success, an error otherwise
 */
CAL_UNUSED static
void __work_thread_signal_to_die_fast(struct work_thread *thread)
{
    ck_pr_store_32(&thread->signal_death, WORK_THREAD_FAST_SHUTDOWN);    
}

/**
 * Private thread helper function for checking for new work endpoints
 */
static
void __work_thread_check_new_endpoints(struct work_thread *thread)
{
    struct work_endpoint *ep = NULL;
    int i = WORK_THREAD_MAX_QUEUED_ENDPOINTS/2;

    while (work_queue_pop(&thread->ep_queue, (void **)&ep), (ep != NULL && (--i > 0))) {
        /* Start the work endpoint */
        work_endpoint_startup(ep);
        list_append(&thread->work_endpoints, &ep->wnode);
    }
}

/* Private thread worker function, used for each work thread */
static
aresult_t __work_thread_worker_func(void *params)
{
    aresult_t ret = A_OK;
    struct work_thread *thread = (struct work_thread *)params;


    while(ck_pr_load_32(&thread->signal_death) == WORK_THREAD_ALIVE) {
        __work_thread_check_new_endpoints(thread);

        /* Poll our endpoints */
        struct work_endpoint *ep, *temp;
        unsigned int min_wait = 5;
        list_for_each_type_safe(ep, temp, &thread->work_endpoints, wnode) {
            /* Check if the time to poll this endpoint has come */
            if (ep->next_poll > time_get_time()) {
                continue;
            }

            unsigned int millis = 0;
            aresult_t poll_ret = work_endpoint_poll(ep, &millis);

            if (millis < min_wait) {
                min_wait = millis;
            }

            /* TODO: check if the work endpoint requested a shutdown */
            if (CAL_UNLIKELY(AFAILED(poll_ret))) {
                DIAG("Endpoint signalled failure during poll. Shutting down...");
                work_endpoint_shutdown(ep);
                /* Jettison the work endpoint */
                list_del(&ep->wnode);
            }
        }

        if (min_wait > 0) {
            usleep(min_wait * 1000);
        }
    }

    DIAG("Shutting down worker thread.");

    /* Signal to all the endpoints they should shutdown */
    struct work_endpoint *ep, *temp;
    list_for_each_type_safe(ep, temp, &thread->work_endpoints, wnode) {
        DIAG("Shutting down endpoint...");
        work_endpoint_shutdown(ep);
        list_del(&ep->wnode);
    }

    return ret;
}

aresult_t work_thread_new(struct work_thread **thread,
                          unsigned int cpu_affinity)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(thread);

    struct work_thread *wt = NULL;

    wt = (struct work_thread *)calloc(1, sizeof(*wt));

    if (wt == NULL) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Create the affinity CPU mask */
    struct cpu_mask *msk = NULL;
    ret = cpu_mask_create(&msk);

    if (AFAILED(ret)) {
        goto done_fail;
    }

    ret = cpu_mask_set(msk, cpu_affinity);

    if (AFAILED(ret)) {
        goto done_fail;
    }

    /* Allocate the actual thread */
    ret = thread_create(&wt->thread, __work_thread_worker_func, msk);
    if (AFAILED(ret)) {
        goto done_fail;
    }
    msk = NULL; /* We handed the mask off to the thread struct */

    /* Finally, initialize the work queue */
    ret = work_queue_new(&wt->ep_queue, WORK_THREAD_MAX_QUEUED_ENDPOINTS);
    if (AFAILED(ret)) {
        goto done_fail;
    }

    /* Signal that the thread is alive */
    wt->signal_death = WORK_THREAD_ALIVE;

    list_init(&wt->work_endpoints);

    *thread = wt;

done_fail:
    /* Clean up after ourselves */
    if (AFAILED(ret)) {
        if (wt) {
            if (wt->thread) {
                thread_destroy(&wt->thread);
            }
            
            if (msk) {
                /* If the thread failed to be created, destroy the mask */
                cpu_mask_destroy(&msk);
            }

            /* We can safely do this no matter what if the queue was initialized */
            work_queue_release(&wt->ep_queue);

            memset(wt, 0, sizeof(*wt));
            free(wt);
            wt = NULL;
        }
    }

done:
    return ret;
}

aresult_t work_thread_shutdown(struct work_thread *thread)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(thread);

    int val = 0;

    ret = thread_running(thread->thread, &val);

    if (val == 0 || AFAILED(ret)) {
        DIAG("Thread state is broken or thread is dead, not signalling shutdown");
        goto done;
    }

    /* Signal to the thread that it should die */
    __work_thread_signal_to_die(thread);

done:
    return ret;
}

aresult_t work_thread_destroy(struct work_thread **thread)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(thread != NULL);
    TSL_ASSERT_ARG(*thread != NULL);

    struct work_thread *pthread = *thread;

    int val = 0;

    ret = thread_running(pthread->thread, &val);

    if (AFAILED(ret) || val != 0) {
        DIAG("Attempted to destroy a work thread while it was still running.");
        goto done;
    }

    /* Join the thread */
    aresult_t thread_ret = A_OK;
    ret = thread_join(pthread->thread, &thread_ret);

    /* Release the thread resources */
    thread_destroy(&pthread->thread);

    /* Release the work queue */
    work_queue_release(&pthread->ep_queue);

    /* Drop all the work endpoints */
    struct work_endpoint *ep, *temp;
    list_for_each_type_safe(ep, temp, &pthread->work_endpoints, wnode) {
        list_del(&ep->wnode);
    }

    /* Now release our memory */
    free(pthread);

    ret = thread_ret;
    *thread = NULL;
done:
    return ret;
}

aresult_t work_thread_start(struct work_thread *thread)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(thread != NULL);

    ret = thread_start(thread->thread, thread);

    return ret;
}

aresult_t work_thread_add_endpoint(struct work_thread *thread,
                                   struct work_endpoint *endpoint)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(thread != NULL);
    TSL_ASSERT_ARG(endpoint != NULL);

    ret = work_queue_push(&thread->ep_queue, endpoint);

    return ret;
}

