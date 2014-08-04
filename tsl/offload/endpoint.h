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
#ifndef __INCLUDED_FOUNDATION_OFFLOAD_ENDPOINT_H__
#define __INCLUDED_FOUNDATION_OFFLOAD_ENDPOINT_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/errors.h>
#include <tsl/list.h>

#include <stdint.h>

/* Forward declaration */
struct work_endpoint;

/* Typedefs for functions used */

/** \typedef Type declaration for an endpoint polling function pointer
 * Basic type used for the work endpoint polling function.
 */
typedef aresult_t (*work_endpoint_poll_func_t)(struct work_endpoint *ep,
                                               unsigned int *wait);

/** \typedef Type declaration for and endpoint startup function pointer
 * Basic type used for declaring a pointer to a work endpoint's startup function,
 * called when the work endpoint first is initialized in the work_thread.
 */
typedef aresult_t (*work_endpoint_startup_func_t)(struct work_endpoint *ep);

/** \typedef Type declaration for an endpoint shutdown function pointer
 * Basic type used when declaring a pointer to a work endpoint's shutdown function,
 * used when a shutdown has been signalled to a given work endpoint by the work
 * pool.
 */
typedef aresult_t (*work_endpoint_shutdown_func_t)(struct work_endpoint *ep);

/* Declaration of work endpoint operations */
struct work_endpoint_ops {
    work_endpoint_poll_func_t       poll;
    work_endpoint_startup_func_t    startup;
    work_endpoint_shutdown_func_t   shutdown;
};

/**
 * Structure containing the state for a work endpoint.
 * Do not initialize directly -- call work_endpoint_init.
 */
struct work_endpoint {
    /** Specified work endpoint lifecycle ops */
    struct work_endpoint_ops *ops;
    /** Private state for the work endpoint */
    void *priv;
    /** Work endpoint list node */
    struct list_entry wnode;
    /** Next poll timestamp for this EP */
    uint64_t next_poll;
    /** A flag used to control a work endpoint */
    unsigned int mode CAL_ALIGN(64);
};


/** \brief Initialize a work endpoint in-place
 * Given a work_endpoint, initialize it to empty state in-place.
 * \param ep Pointer to the endpoint to initialize
 */
aresult_t work_endpoint_init(struct work_endpoint *ep,
                             struct work_endpoint_ops *ops,
                             void *state);

/* Functions called from the context of a Work Thread */

/** \brief Call startup function for a work endpoint
 * On a given work endpoint, call the startup function.
 * \param ep The endpoint in question
 */
aresult_t work_endpoint_startup(struct work_endpoint *ep);

/** \brief Call poll function for endpoint
 * On a given work endpoint, call its poll function. Does basic sanity checking.
 * \param ep The work endpoint in question
 * \param wait_millis The number of millis to wait until the next poll
 */
aresult_t work_endpoint_poll(struct work_endpoint *ep,
                             unsigned int *wait_millis);

/** \brief Call shutdown function for endpoint
 * On a given work endpoint, call its shutdown function. Does basic sanity checking.
 * \param ep The work endpoint in question
 */
aresult_t work_endpoint_shutdown(struct work_endpoint *ep);

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_FOUNDATION_OFFLOAD_ENDPOINT_H__ */

