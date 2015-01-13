#ifndef __INCLUDED_MEGAQUEUE_MEGAQUEUE_PRIV_H__
#define __INCLUDED_MEGAQUEUE_MEGAQUEUE_PRIV_H__

#include <tsl/assert.h>
#include <tsl/errors.h>

#include <ck_pr.h>

static inline
bool __megaqueue_is_full(struct megaqueue *queue)
{
    struct megaqueue_header *hdr = queue->hdr;

    size_t head_offset = ck_pr_load_64(&hdr->head);
    size_t delete_offset = ck_pr_load_64(&hdr->_delete);

    return delete_offset == ((head_offset + 1) % queue->object_count);
}

static inline
bool __megaqueue_is_empty(struct megaqueue *queue)
{
    struct megaqueue_header *hdr = queue->hdr;

    size_t head_offset = ck_pr_load_64(&hdr->head);
    size_t tail_offset = ck_pr_load_64(&hdr->tail);

    return tail_offset == head_offset;
}

static inline
bool __megaqueue_can_delete(struct megaqueue *queue)
{
    struct megaqueue_header *hdr = queue->hdr;

    size_t tail_offset = ck_pr_load_64(&hdr->tail);
    size_t delete_offset = ck_pr_load_64(&hdr->_delete);

    return tail_offset == delete_offset;
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

/**
 * Legacy -- advance both the read and delete pointers in lockstep
 */
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
        ck_pr_store_64(&hdr->_delete, offset);
    } else {
        ret = A_E_EMPTY;
    }

    return ret;
}

/**
 * Advance the read pointer - does not muck with the delete pointer
 */
static inline
aresult_t megaqueue_read_only_advance(struct megaqueue *queue)
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

/**
 * Advance the deletion pointer - stops advancing if the read pointer == the delete pointer
 */
static inline
aresult_t megaqueue_delete_advance(struct megaqueue *queue)
{
    aresult_t ret = A_OK;
    struct megaqueue_header *hdr = NULL;
    size_t offset = 0;

    TSL_ASSERT_ARG_DEBUG(NULL != queue);

    hdr = queue->hdr;

    if (!__megaqueue_can_delete(queue)) {
        offset = ck_pr_load_64(&hdr->_delete);
        offset = (offset + 1) % queue->object_count;

        ck_pr_store_64(&hdr->_delete, offset);
    } else {
        ret = A_E_EMPTY;
    }

    return ret;
}

#endif /* __INCLUDED_MEGAQUEUE_MEGAQUEUE_PRIV_H__ */

