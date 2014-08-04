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
#ifndef __INCLUDED_TSL_TIME_H__
#define __INCLUDED_TSL_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/result.h>
#include <stdint.h>

struct time_source;

/** \typedef get_timestamp_func_t
 * Function pointer for getting a timestamp from a given time source.
 */
typedef uint64_t (*get_timestamp_func_t)(struct time_source *source);

/** \typedef set_time_func_t
 * Function pointer for a function to set the timestamp for a given time source.
 */
typedef void (*set_time_func_t)(struct time_source *source, uint64_t ts);

/**
 *  Structure that defines how to get a timestamp. Allows overriding timing behavior.
 */
struct time_ops {
    get_timestamp_func_t get_timestamp;
    set_time_func_t set_time;
};

/**
 * Structure that defines a particular time source that can be used to get a timestamp.
 * This is typically used by the time_* APIs.
 */
struct time_source {
    struct time_ops *ops;
    const char *name;
    void *priv;
};

/**
 * Get the current system time from the default time source.
 */
uint64_t time_get_time(void);

/**
 * Specify a time source that is to be used as the default source.
 */
aresult_t time_set_default_source(struct time_source *source);

/**
 * Get the time from the provided time source
 */
aresult_t time_get_from_source(struct time_source *source, uint64_t *time);

/**
 * Get the currently active global time source
 */
aresult_t time_get_global_source(struct time_source **source);

/**
 * Get the system clock time source
 */
aresult_t time_get_system_source(struct time_source **source);

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_TSL_TIME_H__ */

