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
#include <tsl/offload/endpoint.h>

#include <tsl/errors.h>
#include <tsl/list.h>
#include <tsl/assert.h>
#include <tsl/diag.h>
#include <tsl/time.h>

#include <string.h>

aresult_t work_endpoint_init(struct work_endpoint *ep,
                             struct work_endpoint_ops *ops,
                             void *state)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(ep != NULL);
    TSL_ASSERT_ARG(ops != NULL);
    TSL_ASSERT_ARG(state != NULL);

    /* At least one function in the ops structure must be defined */
    TSL_ASSERT_ARG(ops->poll != NULL || ops->startup != NULL || ops->shutdown != NULL);

    memset(ep, 0, sizeof(*ep));

    /* Initialize the endpoint */
    ep->ops = ops;
    ep->priv = state;

    list_init(&ep->wnode);

    return ret;
}

aresult_t work_endpoint_startup(struct work_endpoint *ep)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(ep != NULL);
    TSL_ASSERT_ARG(ep->ops != NULL);

    struct work_endpoint_ops *ops = ep->ops;

    if (ops->startup != NULL) {
        ret = ops->startup(ep);
    }

    return ret;
}

aresult_t work_endpoint_poll(struct work_endpoint *ep,
                             unsigned int *wait)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(ep != NULL);
    TSL_ASSERT_ARG(wait != NULL);
    TSL_ASSERT_ARG(ep->ops != NULL);

    struct work_endpoint_ops *ops = ep->ops;

    if (ops->poll != NULL) {
        ret = ops->poll(ep, wait);
        ep->next_poll = time_get_time() + ((*wait) * 1000000);
    }

    return ret;
}

aresult_t work_endpoint_shutdown(struct work_endpoint *ep)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(ep != NULL);
    TSL_ASSERT_ARG(ep->ops != NULL);

    struct work_endpoint_ops *ops = ep->ops;

    if (ops->shutdown != NULL) {
        ret = ops->shutdown(ep);
    }
    
    return ret;
}

