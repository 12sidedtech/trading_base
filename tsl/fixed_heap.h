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
#ifndef __INCLUDED_TSL_FIXED_HEAP_H__
#define __INCLUDED_TSL_FIXED_HEAP_H__

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#include <tsl/errors.h>
#include <tsl/cal.h>
#include <tsl/assert.h>

/**
 * Checks the heap property for the given two items.
 *
 * Logically, the fixed_heap assumes it is a max heap, but by reversing
 * this predicate's return values, you can make the heap behave like a
 * min heap.
 *
 * For example:
 *  - if lhs < rhs return negative;
 *  - if lhs == rhs return 0;
 *  - if lhs > rhs return positive;
 *
 * If lhs and rhs are signed, it is reasonable to return lhs - rhs to
 * create a max heap. Reverse the arguments to create a min heap.
 */
typedef int (*fixed_heap_compare_func_t)(void *lhs, void *rhs);

/**
 * \brief Structure representing a fixed-size heap.
 *
 * A fixed-size heap is a heap represented by an array that cannot be implicitly
 * grown. In the event a user tries to insert too many items into the heap, an
 * error is generated.
 */
struct fixed_heap {
    void **heap;                        /** Array representing the heap */
    size_t capacity;                    /** Capacity of the heap */
    size_t entries;                     /** Number of entries in the heap */
    fixed_heap_compare_func_t compare;  /** Comparison function for heapifying */
};

/**
 * \brief Create a new fixed heap
 * Creates a new, empty fixed heap of the specified size that can contain max_items items.
 * \param heap Pointer to heap structure.
 * \param max_items Maximum number of items a heap can contain.
 * \param compare Function to perform the heapify comparisons.
 * \return A_OK on success, an error code otherwise
 */
aresult_t fixed_heap_new(struct fixed_heap *heap, size_t max_items,
                         fixed_heap_compare_func_t compare);

/**
 * \brief Resizes a fixed heap
 * Resizes a fixed heap, where `new_max_items >= heap->entries`.
 * \param heap The heap to be resized
 * \param new_max_items The number of items the heap should contain.
 * \return A_OK on success, an error code otherwise
 * \note If the memory allocation fails, the heap remains untouched.
 */
aresult_t fixed_heap_resize(struct fixed_heap *heap, size_t new_max_items);

/**
 * \brief Delete a fixed heap
 * Deletes all state associated with a fixed heap and cleans up resources.
 * \param heap The heap to delete.
 * \return A_OK on success, an error code otherwise
 */
aresult_t fixed_heap_delete(struct fixed_heap *heap);

#define FIXED_HEAP_PARENT(idx)  (((idx) - 1) >> 1)
#define FIXED_HEAP_LEFT(idx)    (((idx) << 1) + 1)
#define FIXED_HEAP_RIGHT(idx)   (((idx) << 1) + 2)

static inline
void __fixed_heap_swap(struct fixed_heap *heap, size_t a, size_t b)
{
    void *temp = heap->heap[a];
    heap->heap[a] = heap->heap[b];
    heap->heap[b] = temp;
}

/**
 * Insert an item into the heap, maintaining the heap property enforced by
 * the predicate heap->compare
 * \param heap The heap to insert the item into
 * \param key The key to insert
 * \param idx The index of the insertion
 * \note This is a private function. Do not call directly.
 */
static inline
void __fixed_heap_insert(struct fixed_heap *heap, void *key, size_t idx)
{
    heap->heap[idx] = key;

    size_t i = idx;
    while (i > 0 && (heap->compare(heap->heap[FIXED_HEAP_PARENT(i)],
                                   heap->heap[i]) < 0))
    {
        /* Swap current for parent */
        __fixed_heap_swap(heap, i, FIXED_HEAP_PARENT(i));
        i = FIXED_HEAP_PARENT(i);
    }
}

/**
 * Re-heapify the heap, after removing an item typically.
 * \param heap The heap to maintain the heap property of
 * \param idx The index from which to start updating the heap
 * \note This is a private function. Do not call directly.
 */
static inline
void __fixed_heap_heapify(struct fixed_heap *heap, size_t idx)
{
    size_t largest = (size_t)(-1);
    size_t i = idx;

    do {
        size_t l = FIXED_HEAP_LEFT(i);
        size_t r = FIXED_HEAP_RIGHT(i);

        largest = i;

        if (l < heap->entries && (heap->compare(heap->heap[l], heap->heap[i]) > 0)) {
            largest = l;
        }

        if (r < heap->entries && (heap->compare(heap->heap[r], heap->heap[largest]) > 0)) {
            largest = r;
        } 

        if (largest != i) {
            __fixed_heap_swap(heap, i, largest);
            i = largest;
            largest = (size_t)(-1);
        }
    } while (i != largest);
}

/**
 * Insert an item into the heap.
 * \param heap The heap to insert the item into
 * \param item The item to insert into the heap
 * \return A_OK on success, an error code on failure
 */
static inline
aresult_t fixed_heap_push(struct fixed_heap *heap, void *item)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(heap != NULL);
    TSL_ASSERT_ARG_DEBUG(heap->heap != NULL);

    if (CAL_UNLIKELY(heap->capacity <= heap->entries)) {
        ret = A_E_NOSPC;
        goto done;
    }

    __fixed_heap_insert(heap, item, heap->entries++);

done:
    return ret;
}

/**
 * Remove the item at the head of the fixed heap.
 */
static inline
aresult_t fixed_heap_pop(struct fixed_heap *heap, void **item)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(heap != NULL);
    TSL_ASSERT_ARG_DEBUG(item != NULL);
    TSL_ASSERT_ARG_DEBUG(heap->heap != NULL);

    if (CAL_UNLIKELY(heap->entries == 0)) {
        ret = A_E_EMPTY;
        goto done;
    }

    *item = heap->heap[0];
    heap->entries--;
    heap->heap[0] = heap->heap[heap->entries];
    heap->heap[heap->entries] = NULL;
    __fixed_heap_heapify(heap, 0);

done:
    return ret;
}

/**
 * Peek at the item at the head of the fixed heap.
 */
static inline
aresult_t fixed_heap_peek_head(struct fixed_heap *heap, void **item)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(heap != NULL);
    TSL_ASSERT_ARG_DEBUG(item != NULL);
    TSL_ASSERT_ARG_DEBUG(heap->heap != NULL);

    if (CAL_UNLIKELY(heap->entries == 0)) {
        ret = A_E_EMPTY;
        goto done;
    }

    *item = heap->heap[0];
done:
    return ret;
}

/**
 * Remove the specified item (found by searching) from the heap.
 */
static inline
aresult_t fixed_heap_remove_item(struct fixed_heap *heap, void *item)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG_DEBUG(heap != NULL);

    size_t item_offs = (size_t)-1;
    for (size_t i = 0; i < heap->entries; i++) {
        if (heap->heap[i] == item) {
            item_offs = i;
            break;
        }
    }

    if (CAL_UNLIKELY(item_offs == (size_t)-1)) {
        ret = A_E_NOTFOUND;
        goto done;
    }

    /* If this is the last item on the heap, just decrement
     * entries. Otherwise, take the last item, replace the current
     * item with it and sift it appropriately.
     */
    if (CAL_LIKELY(item_offs < (heap->entries--))) {
        heap->heap[item_offs] = heap->heap[heap->entries];
        __fixed_heap_heapify(heap, item_offs);
    }

done:
    return ret;
}

#ifdef __cplusplus
} // extern "C"
#endif /* defined(__cplusplus) */

#endif /* __INCLUDED_TSL_FIXED_HEAP_H__ */

