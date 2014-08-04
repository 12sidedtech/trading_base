/*
  Copyright (c) 2014, 12Sided Technology LLC.
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
#include <tsl/timer.h>
#include <tsl/errors.h>
#include <tsl/time.h>
#include <tsl/alloc.h>
#include <tsl/diag.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static
int __timer_manager_sort_timers(void *lhs, void *rhs)
{
    struct timer *tlhs = (struct timer *)lhs;
    struct timer *trhs = (struct timer *)rhs;

    return (int64_t)trhs->next_firing - (int64_t)tlhs->next_firing;
}

aresult_t timer_manager_new(struct timer_manager **pmgr, size_t max_timers)
{
    aresult_t ret = A_OK;
    struct timer_manager *mgr = NULL;

    TSL_ASSERT_ARG(NULL != pmgr);
    TSL_ASSERT_ARG(0 < max_timers);

    *pmgr = NULL;

    mgr = calloc(1, sizeof(*mgr));
    if (NULL == mgr) {
        ret = A_E_NOMEM;
        goto done;
    }

    if (AFAILED(fixed_heap_new(&mgr->armed_timers, max_timers, __timer_manager_sort_timers))) {
        DIAG("Failed to allocate a heap for the live timers.");
        ret = A_E_NOMEM;
        goto done;
    }

    list_init(&mgr->live_timers);

    *pmgr = mgr;

done:
    if (AFAILED(ret)) {
        if (NULL != mgr) {
            fixed_heap_delete(&mgr->armed_timers);
            free(mgr);
            mgr = NULL;
        }
    }
    return ret;
}

aresult_t timer_manager_delete(struct timer_manager **pmgr)
{
    aresult_t ret = A_OK;
    struct timer_manager *mgr = NULL;

    TSL_ASSERT_ARG(NULL != pmgr);
    TSL_ASSERT_ARG(NULL != *pmgr);

    mgr = *pmgr;

    fixed_heap_delete(&mgr->armed_timers);
    memset(mgr, 0, sizeof(*mgr));
    free(mgr);

    *pmgr = NULL;

    return ret;
}

static
aresult_t __timer_manager_add_timer(struct timer *tmr, struct timer_manager *mgr)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(NULL != tmr);
    TSL_ASSERT_ARG_DEBUG(NULL != mgr);

    tmr->mgr = mgr;
    list_append(&mgr->live_timers, &tmr->tnode);

    if (AFAILED_UNLIKELY(fixed_heap_push(&mgr->armed_timers, tmr))) {
        ret = A_E_INVAL;
        goto done;
    }

done:
    return ret;
}

aresult_t timer_arm(struct timer *tmr, struct timer_manager *mgr, uint64_t usecs)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(NULL != tmr);
    TSL_ASSERT_ARG_DEBUG(NULL != mgr);
    /* Must be some time in the future */
    TSL_ASSERT_ARG_DEBUG(0 < usecs);

    tmr->next_firing = time_get_time() + (usecs * 1000);

    ret = __timer_manager_add_timer(tmr, mgr);

    return ret;
}

aresult_t timer_arm_at(struct timer *tmr, struct timer_manager *mgr, uint64_t when)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(NULL != tmr);
    TSL_ASSERT_ARG_DEBUG(NULL != mgr);

    tmr->next_firing = when;

    ret = __timer_manager_add_timer(tmr, mgr);

    return ret;
}

aresult_t timer_disarm(struct timer *tmr)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(NULL != tmr);

    tmr->next_firing = 0;

    return ret;
}

