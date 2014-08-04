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
#ifndef __INCLUDED_MEGAQUEUE_MEGAQUEUE_H__
#define __INCLUDED_MEGAQUEUE_MEGAQUEUE_H__

#include <tsl/cal.h>
#include <tsl/result.h>
#include <stdint.h>

/**
 * A megaqueue filename is of the form "megaqueue_[qdesc]"
 */
#define MEGAQUEUE_NAME_PREFIX       "megaqueue_"
/**
 * A megaqueue consumer filename is "mqconsumer_[qdesc]_[consumername]"
 */
#define MEGAQUEUE_CONSUMER_PREFIX   "mqconsumer_"

/**
 * Contents of the header page of the Megaqueue
 */
struct megaqueue_header {
    /** Head of the Megaqueue, current position */
    uint64_t head CAL_CACHE_ALIGNED;
    /** Tail of the Megaqueue for the sensitive listener */
    uint64_t tail CAL_CACHE_ALIGNED;
    /* PID of the process that is the producer */
    uint64_t producer_pid CAL_CACHE_ALIGNED;
    /* Size of an object in the Megaqueue */
    uint64_t object_size;
    /* Maximum count of objects in the Megaqueue */
    uint64_t object_count;
} CAL_CACHE_ALIGNED;

enum megaqueue_consumer_state {
    CONSUMER_STATE_ACTIVE,
    CONSUMER_STATE_SHUTDOWN
};

/**
 * The current state of the Megaqueue Consumer in question
 */
struct megaqueue_consumer_state_block {
    /** Where this consumer currently is in the megaqueue */
    uint64_t tail CAL_CACHE_ALIGNED;
    /** The current state of this megaqueue listener 
     * \see enum megaqueue_consumer_state
     * */
    int state CAL_CACHE_ALIGNED;

    /* ----- Less Frequently Accessed Items ----- */
    /** PID of this consumer process */
    uint32_t pid;
    /** Whether or not this process is critical (alert on failure) or passive */
    int critical;
};

struct megaqueue {
    /** Size of an object in the megaqueue, in bytes */
    uint32_t object_size;
    /** Count of objects in the megaqueue */
    uint32_t object_count;
    /** Start of the writable megaqueue region */
    void *writable_start;
    /** The entire shared memory region */
    void *region;
    /** The size of the full mapped region */
    size_t region_size;
    /** Shared memory region name */
    char *rgn_name;

    /* The megaqueue header, used in initialization of a consumer */
    struct megaqueue_header *hdr;

    /** The file descriptor used to map the region */
    int fd;
};

struct megaqueue_consumer {
    /** Current read head of the megaqueue */
    void *read_head;

    /** The position of the current read location */
    size_t read_pos;

    struct megaqueue mq;
};

struct megaqueue_producer {
    /** Current write head of the megaqueue */
    void *write_head;

    struct megaqueue mq;
};

aresult_t megaqueue_open(struct megaqueue *queue,
                         int mode,
                         const char *queue_name,
                         size_t obj_size,
                         size_t obj_count);

aresult_t megaqueue_close(struct megaqueue *queue, int unlink);

aresult_t megaqueue_consumer_open(struct megaqueue_consumer **queue,
                                  const char *queue_name,
                                  size_t obj_size,
                                  size_t obj_count);

/* Internal functions for managing the megaqueue */
static inline
aresult_t megaqueue_advance(struct megaqueue *queue);

static inline
aresult_t megaqueue_next_slot(struct megaqueue *queue, void **slot);

static inline
aresult_t megaqueue_read_next_slot(struct megaqueue *queue, void **slot);

static inline
aresult_t megaqueue_read_advance(struct megaqueue *queue);

#include <tsl/megaqueue/megaqueue_priv.h>

#endif /* __INCLUDED_MEGAQUEUE_MEGAQUEUE_H__ */

