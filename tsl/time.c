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
#include <tsl/time.h>
#include <tsl/diag.h>
#include <tsl/errors.h>
#include <tsl/assert.h>

#include <stdint.h>

#include <time.h>

uint64_t system_get_timestamp(struct time_source *src)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (ts.tv_sec * 1000000000) + ts.tv_nsec;
}

void system_set_time(struct time_source *src, uint64_t ts)
{
    DIAG("System time source does not support setting of time.");
}

/* Definition of operations for working with the system time */
static struct time_ops system_time_ops = {
    .get_timestamp = system_get_timestamp,
    .set_time = system_set_time,
};

/* Declaration of the default system time source */
static struct time_source system_time = {
    .ops = &system_time_ops,
    .name = "System Clock",
    .priv = NULL,
};

static struct time_source *global_time_source = &system_time;

/* Public interface */

uint64_t time_get_time(void)
{
    return global_time_source->ops->get_timestamp(global_time_source);
}

aresult_t time_set_default_source(struct time_source *source)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(source != NULL);

    struct time_ops *ops = source->ops;

    if (ops == NULL) {
        ret = A_E_INVAL;
        goto done;
    }

    if (ops->get_timestamp == NULL || ops->set_time == NULL) {
        ret = A_E_INVAL;
        goto done;
    }

    global_time_source = source;

done:
    return ret;
}

aresult_t time_get_from_source(struct time_source *source, uint64_t *time)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(source != NULL);
    TSL_ASSERT_ARG(time != NULL);
    TSL_ASSERT_ARG(source->ops != NULL);

    struct time_ops *ops = source->ops;

    *time = ops->get_timestamp(source);

    return ret;
}

aresult_t time_get_global_source(struct time_source **source)
{
    TSL_ASSERT_ARG(source != NULL);
    *source = global_time_source;
    return A_OK;
}

aresult_t time_get_system_source(struct time_source **source)
{
    TSL_ASSERT_ARG(source != NULL);
    *source = &system_time;
    return A_OK;
}

