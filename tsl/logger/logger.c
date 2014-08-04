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
#include <tsl/cal.h>
#include <tsl/errors.h>
#include <tsl/assert.h>
#include <tsl/rbtree.h>
#include <tsl/offload/endpoint.h>
#include <tsl/offload/queue.h>
#include <tsl/offload/pool.h>
#include <tsl/logger/logger.h>

/* Forward declarations */
static struct work_endpoint_ops logger_endpoint_ops;

enum logger_state {
    /** Logger is in the setup phase */
    LOGGER_STATE_INACTIVE = 0,
    /** Logger has been told to assign itself to a thread */
    LOGGER_STATE_STARTING,
    /** Logger has been started and is running in another thread */
    LOGGER_STATE_RUNNING,
    /** Logger has been terminated, and cannot be used further */
    LOGGER_STATE_TERMINATED
};

/**
 * \brief Structure that defines and initializes a logger endpoint
 *
 * A logger is a work endpoint that is capable of taking a structure and a control code
 * and converting that to a pre-formatted message that is dumped to the specified logging
 * target, typically a stdio FILE.
 *
 * Multiple loggers can exist for a single application, but they should typically be
 * writing to different sinks for obvious reasons.
 */
struct logger_endpoint {
    /**
     * Allocator for logger messages
     * \note To keep this in the context of the initiating thread, the allocator
     *       lives in its own cache line.
     */
    struct allocator *message_alloc CAL_CACHE_ALIGNED;

    /**
     * Transmit from initiating thread to work thread
     * \note To avoid ping-ponging other data, kept on its own cache line(s)
     */
    struct work_queue out_queue CAL_CACHE_ALIGNED;

    /**
     * Receive from work thread to initiating queue
     * \note To avoid ping-ponging other data, kept on its own cache line(s)
     */
    struct work_queue in_queue CAL_CACHE_ALIGNED;

    /**
     * The actual work endpoint (always lives in work thread)
     */
    struct work_endpoint ep;

    /**
     * Tree of message handlers (always lives in work thread)
     */
    struct rb_tree message_handlers;

    /**
     * List of message handlers (lifecycle management)
     * \note Always lives in worker thread
     */
    struct list_entry message_handler_list;

    /**
     * State of the logger (shared state)
     */
    enum logger_state state CAL_CACHE_ALIGNED;
};

static
int __logger_cmp_message_handlers(void *lhs, void *rhs)
{
    int nlhs = (int)(uint64_t)lhs;
    int nrhs = (int)(uint64_t)rhs;

    return (int)(nlhs - nrhs);
}

/**
 * Initialize a new logger endpoint.
 * \param ep Pointer to receive a newly initialized endpoint
 * \param queue_depth Size of the message queue. Must be a power of 2.
 * \return A_OK on success, an error code otherwise.
 */
aresult_t logger_initialize(struct logger_endpoint **ep, unsigned int queue_depth)
{
    aresult_t ret = A_OK;
    struct logger_endpoint *nep = NULL;

    TSL_ASSERT_ARG(NULL != ep);

    *ep = NULL;

    /* Allocate */
    nep = calloc(1, sizeof(*nep));

    if (NULL == nep) {
        ret = A_E_NOMEM;
        goto done;
    }

    /* Initialize the work endpoint */
    if (AFAILED(ret = work_endpoint_init(&nep->ep, &logger_endpoint_ops, nep))) {
        DIAG("Failed to initialize work endpoint. Aborting.");
        goto done;
    }

    /* Initialize the work queues */
    if (AFAILED(ret = work_queue_new(&nep->out_queue, queue_depth))) {
        DIAG("Failed to initialize out queue for logger.");
        goto done;
    }
    if (AFAILED(ret = work_queue_new(&nep->in_queue, queue_depth))) {
        DIAG("Failed to initialize in queue for logger.");
        goto done;
    }

    list_init(&nep->message_handler_list);

    if (AFAILED(ret = rb_tree_new(&nep->message_handlers, __logger_cmp_message_handlers))) {
        DIAG("Failed to initialize RB-tree of message handlers");
        goto done;
    }

    *ep = nep;

done:
    if (AFAILED(ret)) {
        if (NULL != nep) {
            work_queue_release(&nep->in_queue);
            work_queue_release(&nep->out_queue);
            free(nep);
            nep = NULL;
        }
    }

    return ret;
}

/**
 * Add an event handler to the logger
 */
aresult_t logger_add_message_handler(struct logger_endpoint *ep, unsigned int message_type, logger_message_emit_func_t func)
{
    aresult_t ret = A_OK;
    struct logger_message_handler *nhdlr = NULL;

    TSL_ASSERT_ARG(NULL != ep);
    TSL_ASSERT_ARG(0 < message_type);
    TSL_ASSERT_ARG(NULL != func);
    TSL_ASSERT_ARG(LOGGER_STATE_INACTIVE == ep->state);

    /* Check if the message_type is already registered */
    struct rb_tree_node *temp = NULL;
    if (!AFAILED(rb_tree_find(&ep->message_handlers, (void *)(uint64_t)message_type, &temp))) {
        ret = A_E_BUSY;
        DIAG("Already have a handler registered for message type %u\n", message_type);
        goto done;
    }

    nhdlr = calloc(1, sizeof(*nhdlr));

    if (NULL == nhdlr) {
        ret = A_E_NOMEM;
        goto done;
    }

    nhdlr->message_type = message_type;
    nhdlr->state = ep;
    nhdlr->dispatch = func;

    list_init(&nhdlr->lnode);

    if (AFAILED(ret = rb_tree_insert(&ep->message_handlers, (void *)(uint64_t)message_type, &nhdlr->tnode))) {
        DIAG("Failed to insert into RB-tree of message handlers");
        goto done;
    }

    list_append(&ep->message_handler_list, &nhdlr->lnode);

done:
    if (AFAILED(ret)) {
        if (NULL != nhdlr) {
            free(nhdlr);
        }

    }
    return ret;
}

/**
 * Attach a logger to the specified thread in the given work pool
 * \param ep The endpoint to attach
 * \param pool The work pool containing the thread the logger is to be associated to
 * \param thread_id The ID of the thread in the work pool
 * \return A_OK on success, an error code otherwise.
 */
aresult_t logger_start(struct logger_endpoint *ep, struct work_pool *pool, unsigned int thread_id)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(NULL != ep);
    TSL_ASSERT_ARG(NULL != pool);
    TSL_ASSERT_ARG(LOGGER_STATE_INACTIVE == ep->state);

    ep->state = LOGGER_STATE_STARTING;
    if (AFAILED(ret = work_pool_add_endpoint(pool, thread_id, &ep->ep))) {
        DIAG("Failed to add logger endpoint to specified thread %u", thread_id);
        goto done;
    }

done:
    if (AFAILED(ret)) {
        ep->state = LOGGER_STATE_TERMINATED;
    }
    return ret;
}

/*--------------------------------------------------------*/
/* All code beyond this point runs in the thread context  */
/*--------------------------------------------------------*/

static inline
struct logger_message_handler *__logger_get_handler(struct logger_endpoint *lep, unsigned int message_id)
{
    struct rb_tree_node *handler = NULL;
    if (AFAILED(rb_tree_find(&lep->message_handlers, (void *)(uint64_t)message_id, &handler))) {
        return NULL;
    }

    return BL_CONTAINER_OF(handler, struct logger_message_handler, tnode);
}

/**
 * poll operation for the logger endpoint
 */
static
aresult_t logger_endpoint_poll_func(struct work_endpoint *ep, unsigned int *wait)
{
    aresult_t ret = A_OK;

    struct logger_endpoint *lep = (struct logger_endpoint *)ep->priv;

    /* Check the receive queue in the work endpoint */
    unsigned int fill = 0;

    if (AFAILED_UNLIKELY(work_queue_fill(&lep->out_queue, &fill))) {
        DIAG("ERROR: checking logger queue fill failed.");
        ret = A_E_INVAL;
        goto done;
    }

    if (fill == 0) {
        goto done;
    }

    /* Process top of the queue */
    struct logger_message *lmsg = NULL;
    if (AFAILED_UNLIKELY(work_queue_pop(&lep->out_queue, (void **)&lmsg))) {
        DIAG("ERROR: unexpected failure in work queue management");
        goto done;
    }

    /* Execute the logger message handler */
    struct logger_message_handler *hdlr = __logger_get_handler(lep, lmsg->message_type);

    if (NULL == hdlr) {
        DIAG("ERROR: could not find message handler for message type %u\n", lmsg->message_type);
        goto done;
    }

    hdlr->dispatch(hdlr->state, lmsg->payload);

done:
    if (NULL != lmsg) {
        /* Return the message to the calling thread */
        if (AFAILED_UNLIKELY(work_queue_push(&lep->in_queue, (void *)lmsg))) {
            DIAG("ERROR: message delayed in returning -- something is really wrong with app config");
            /* TODO: push the message into a waiting queue */
        }
    }

    /* TODO: check the waiting queue and push those messages into the return queue */

    return ret;
}

/**
 * startup function called in the thread context
 */
static
aresult_t logger_endpoint_startup(struct work_endpoint *ep)
{
    aresult_t ret = A_OK;

    struct logger_endpoint *lep = (struct logger_endpoint *)ep->priv;
    lep->state = LOGGER_STATE_RUNNING;

    return ret;
}

/**
 * Shutdown function called in thread context
 */
static
aresult_t logger_endpoint_shutdown(struct work_endpoint *ep)
{
    aresult_t ret = A_OK;

    struct logger_endpoint *lep = (struct logger_endpoint *)ep->priv;
    lep->state = LOGGER_STATE_TERMINATED;

    return ret;
}

static
struct work_endpoint_ops logger_endpoint_ops = {
    .poll = logger_endpoint_poll_func,
    .startup = logger_endpoint_startup,
    .shutdown = logger_endpoint_shutdown
};

