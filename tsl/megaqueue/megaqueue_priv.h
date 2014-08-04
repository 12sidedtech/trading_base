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
#ifndef __INCLUDED_MEGAQUEUE_MEGAQUEUE_PRIV_H__
#define __INCLUDED_MEGAQUEUE_MEGAQUEUE_PRIV_H__

#include <tsl/assert.h>
#include <tsl/errors.h>

#include <ck_pr.h>

static inline
int __megaqueue_is_full(struct megaqueue *queue)
{
    struct megaqueue_header *hdr = queue->hdr;

    size_t head_offset = ck_pr_load_64(&hdr->head);
    size_t tail_offset = ck_pr_load_64(&hdr->tail);

    return tail_offset == ((head_offset + 1) % queue->object_count);
}

static inline
int __megaqueue_is_empty(struct megaqueue *queue)
{
    struct megaqueue_header *hdr = queue->hdr;

    size_t head_offset = ck_pr_load_64(&hdr->head);
    size_t tail_offset = ck_pr_load_64(&hdr->tail);

    return tail_offset == head_offset;
}

static inline
int __megaqueue_waiting(struct megaqueue *queue)
{
    return !(__megaqueue_is_full(queue) || __megaqueue_is_empty(queue));
}

/**
 * Get the next producer slot in the megaqueue, by reference
 */
static inline
aresult_t megaqueue_next_slot(struct megaqueue *queue, void **slot)
{
    aresult_t ret = A_OK;
    size_t head_offset = 0;
    struct megaqueue_header *hdr = NULL;
    void *cur_pos = NULL;

    TSL_ASSERT_ARG_DEBUG(NULL != queue);
    TSL_ASSERT_ARG_DEBUG(NULL != slot);

    hdr = queue->hdr;
    head_offset = ck_pr_load_64(&hdr->head);

    /* Always leave one free entry */
    if ( __megaqueue_is_full(queue) ) {
        ret = A_E_NOSPC;
        goto done;
    }

    cur_pos = queue->writable_start + (queue->object_size * head_offset);

    *slot = cur_pos;

done:
    return ret;
}

/**
 * Ratchet the producer counter forward to the next megaqueue slot.
 */
static inline
aresult_t megaqueue_advance(struct megaqueue *queue)
{
    aresult_t ret = A_OK;
    struct megaqueue_header *hdr = NULL;
    size_t offset = 0;

    TSL_ASSERT_ARG_DEBUG(NULL != queue);

    hdr = queue->hdr;

    if (!__megaqueue_is_full(queue)) {
        offset = ck_pr_load_64(&hdr->head);
        offset = (offset + 1) % queue->object_count;

        ck_pr_store_64(&hdr->head, offset);
    } else {
        ret = A_E_NOSPC;
    }

    return ret;
}

static inline
aresult_t megaqueue_read_next_slot(struct megaqueue *queue, void **slot)
{
    aresult_t ret = A_OK;
    struct megaqueue_header *hdr = NULL;
    void *cur_pos = NULL;
    size_t tail_offset = 0;

    TSL_ASSERT_ARG_DEBUG(NULL != queue);
    TSL_ASSERT_ARG_DEBUG(NULL != slot);

    hdr = queue->hdr;
    tail_offset = ck_pr_load_64(&hdr->tail);

    if (!__megaqueue_is_empty(queue)) {
        cur_pos = queue->writable_start + (queue->object_size * tail_offset);
        *slot = cur_pos;
    } else {
        *slot = NULL;
        ret = A_E_EMPTY;
    }

    return ret;
}

static inline
aresult_t megaqueue_read_advance(struct megaqueue *queue)
{
    aresult_t ret = A_OK;
    struct megaqueue_header *hdr = NULL;
    size_t offset = 0;

    TSL_ASSERT_ARG_DEBUG(NULL != queue);

    hdr = queue->hdr;

    if (!__megaqueue_is_empty(queue)) {
        offset = ck_pr_load_64(&hdr->tail);
        offset = (offset + 1) % queue->object_count;

        ck_pr_store_64(&hdr->tail, offset);
    } else {
        ret = A_E_EMPTY;
    }

    return ret;

}

#endif /* __INCLUDED_MEGAQUEUE_MEGAQUEUE_PRIV_H__ */

