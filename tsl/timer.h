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
#ifndef __INCLUDED_TSL_TIMER_H__
#define __INCLUDED_TSL_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/cal.h>
#include <tsl/list.h>
#include <tsl/errors.h>
#include <tsl/fixed_heap.h>
#include <tsl/alloc.h>
#include <tsl/time.h>
#include <tsl/diag.h>

#include <stdint.h>

/* Forward declarations */
struct timer;

/**
 * A timer manager. Instantiate a timer manager with a maximum number of timers
 * to be able to use the deadline timing infrastructure. The timer manager
 * will manage all armed timers associated with it, and provides constant time
 * allocation facilities for all timers.
 */
struct timer_manager {
    /** Min-heap of timers, sorted by time */
    struct fixed_heap armed_timers;
    /** Linked list of all live (allocated) timers */
    struct list_entry live_timers;
};

/**
 * Create and initialize a new timer manager, to be used to track the lifecycle of
 * a timer.
 * \param pmgr New manager instance, returned by value
 * \param max_timers The maximum number of timers that can be armed at once
 * \return A_OK on success, an error code otherwise
 */
aresult_t timer_manager_new(struct timer_manager **pmgr, size_t max_timers);

/**
 * Delete a timer manager instance.
 * \param pmgr The timer manager to delete. Set to NULL on success.
 * \return A_OK on success, an error code otherwise
 */
aresult_t timer_manager_delete(struct timer_manager **pmgr);

/**
 * Fire the first pending timer that is ready to fire, if applicable. Return time
 * until the next timer might be ready to fire, in nanoseconds, by reference.
 * \param mgr The timer manager to inspect and manipulate
 * \param pnext When the next timer is to fire (nanoseconds since epoch)
 * \return A_OK on success, an error code otherwise.
 */
static inline
aresult_t timer_manager_fire_next(struct timer_manager *mgr, uint64_t *pnext);

/**
 * Function pointer for a timer firing event
 */
typedef aresult_t (*timer_fire_func_t)(struct timer *);

#define TIMER_STATE_DISARMED ((uint64_t)0)

/**
 * A structure describing a single timer
 */
struct timer {
    /** Function to be fired when the timer goes live */
    timer_fire_func_t fire;

    /***************************
     * Private data
     */

    /** Time the function is next to fire (nanoseconds since epoch) */
    uint64_t next_firing;
    /** Linked list entry for all timers */
    struct list_entry tnode;
    /** The timer manager we hail from */
    struct timer_manager *mgr;
} CAL_CACHE_ALIGNED;

/**
 * Arm the given timer, using the given timer_manager to track the timer's
 * lifecycle. Timer fires at now + usecs.
 * \param tmr The timer to arm
 * \param mgr The manager to associate the timer with
 * \param usecs The time from now to fire the timer at
 * \return A_OK on success, an error code otherwise
 */
aresult_t timer_arm(struct timer *tmr, struct timer_manager *mgr, uint64_t usecs);

/**
 * Arm the given timer, using the given timer_manager to track the timer's
 * lifecycle, specifically firing the timer at `when` nanoseconds from the epoch.
 * \param tmr The timer to arm
 * \param mgr The manager to associate the timer with
 * \param when The number of nanoseconds since the epoch to fire the timer at.
 * \return A_OK on success, an error code otherwise
 */
aresult_t timer_arm_at(struct timer *tmr, struct timer_manager *mgr, uint64_t when);

/**
 * Disarm the specified timer, preventing it from firing.
 * \param tmr The timer to disarm.
 * \return A_OK on success, an error code otherwise
 */
aresult_t timer_disarm(struct timer *tmr);

/* Inlined function implementation */

static inline
aresult_t timer_manager_fire_next(struct timer_manager *mgr, uint64_t *pnext)
{
    aresult_t ret = A_OK;
    struct timer *tmr = NULL;

    TSL_ASSERT_ARG_DEBUG(NULL != mgr);
    TSL_ASSERT_ARG_DEBUG(NULL != pnext);

    *pnext = 0;

    if (AFAILED(ret = fixed_heap_peek_head(&mgr->armed_timers, (void **)&tmr))) {
        if (ret == A_E_EMPTY) {
            ret = A_OK;
        }
        goto done;
    }

    if (time_get_time() >= tmr->next_firing) {
        /* Pop from the top of the heap */
        if (AFAILED(ret = fixed_heap_pop(&mgr->armed_timers, (void **)&tmr))) {
            DIAG("Failed to pop first item from armed timers list");
            goto done;
        }

        /* Remove from the active list */
        list_del(&tmr->tnode);

        /* Fire! */
        tmr->fire(tmr);

        /* Get the next timer to fire, if any */
        if (AFAILED(ret = fixed_heap_peek_head(&mgr->armed_timers, (void **)&tmr))) {
            goto done;
        }
    }

    *pnext = tmr->next_firing;

done:
    return ret;
}

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_TSL_TIMER_H__ */

