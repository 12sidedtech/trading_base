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
#ifndef __INCLUDED_LOGGER_LOGGER_H__
#define __INCLUDED_LOGGER_LOGGER_H__

#include <tsl/rbtree.h>
#include <tsl/list.h>

#include <stdint.h>

struct logger_endpoint;

/**
 * Function pointer for function to be called when the logger message is returned
 * to the initiating thread.
 */
typedef aresult_t (*logger_payload_release_func_t)(void *payload);

struct logger_message {
    /**
     * Message timestamp
     */
    uint64_t timestamp;
    /**
     * Function called to release the state associated with this message on return
     * to initiating thread.
     */
    logger_payload_release_func_t release;
    /**
     * The payload to be emitted.
     */
    void *payload;
    /**
     * The type of message
     */
    unsigned int message_type;
} CAL_CACHE_ALIGNED;

typedef aresult_t (*logger_message_emit_func_t)(void *state, void *payload);

struct logger_message_handler {
    unsigned int message_type;
    void *state;
    logger_message_emit_func_t dispatch;
    struct rb_tree_node tnode;
    struct list_entry lnode;
};

aresult_t logger_initialize(struct logger_endpoint **ep, unsigned int queue_depth);
aresult_t logger_add_message_handler(struct logger_endpoint *ep, unsigned int message_type, logger_message_emit_func_t func);

#endif /* __INCLUDED_LOGGER_LOGGER_H__ */

