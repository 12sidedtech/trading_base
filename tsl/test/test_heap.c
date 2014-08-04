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
#include <tsl/test/helpers.h>

//#include <tsl/heap.h>
#include <tsl/fixed_heap.h>

#include <tsl/basic.h>
#include <tsl/cal.h>

static
int test_min_heap_compare_func(void *lhs, void *rhs)
{
    int64_t l = (int64_t)lhs;
    int64_t r = (int64_t)rhs;

    int64_t delta = r - l;  /* Min heap */

    return (delta > 0) ? 1 : ((delta < 0) ? -1 : 0);
}

static
int test_max_heap_compare_func(void *lhs, void *rhs)
{
    int64_t l = (int64_t)lhs;
    int64_t r = (int64_t)rhs;

    int64_t delta = l - r;  /* Max heap */

    return (delta > 0) ? 1 : ((delta < 0) ? -1 : 0);
}

static
int test_heap_assert(struct fixed_heap *heap, fixed_heap_compare_func_t cmp)
{
    for (size_t i = 0; i < heap->entries; i++) {
        if (FIXED_HEAP_LEFT(i) < heap->entries) {
            TEST_ASSERT(cmp(heap->heap[i], heap->heap[FIXED_HEAP_LEFT(i)]) >= 0);
        }

        if (FIXED_HEAP_RIGHT(i) < heap->entries) {
            TEST_ASSERT(cmp(heap->heap[i], heap->heap[FIXED_HEAP_RIGHT(i)]) >= 0);
        }
    }

    return TEST_OK;
}

static CAL_UNUSED
void test_dump_heap(struct fixed_heap *heap)
{
    printf("{ ");
    for (size_t i = 0; i < heap->entries; i++) {
        printf("%ld, ", (int64_t)heap->heap[i]);
    }
    printf("}\n");
}

TEST_DECL(test_fixed_heap)
{
    struct fixed_heap heap;

    /* Create and destroy a small min-heap */
    TEST_ASSERT_EQUALS(fixed_heap_new(&heap, 7, test_min_heap_compare_func), A_OK);

    for (int64_t i = 7; i > 0; i--) {
        TEST_ASSERT_EQUALS(fixed_heap_push(&heap, (void *)i), A_OK);
    }
    
    int64_t head_peek = -1;
    TEST_ASSERT_EQUALS(fixed_heap_peek_head(&heap, (void **)&head_peek), A_OK);
    TEST_ASSERT_EQUALS(head_peek, 1);

    TEST_ASSERT_EQUALS(test_heap_assert(&heap, test_min_heap_compare_func), TEST_OK);

    for (int64_t i = 1; i < 8; i++) {
        int64_t head = -1;
        TEST_ASSERT_EQUALS(fixed_heap_pop(&heap, (void **)&head), A_OK);
        TEST_ASSERT_EQUALS(test_heap_assert(&heap, test_min_heap_compare_func), TEST_OK);
        TEST_ASSERT_EQUALS(head, i);
    }

    TEST_ASSERT_EQUALS(fixed_heap_delete(&heap), A_OK);
    TEST_ASSERT_EQUALS(heap.heap, NULL);

    /* Create and destroy a min-heap */
    TEST_ASSERT_EQUALS(fixed_heap_new(&heap, 120, test_min_heap_compare_func), A_OK);

    for (int64_t i = 120; i > 0; i--) {
        TEST_ASSERT_EQUALS(fixed_heap_push(&heap, (void *)i), A_OK);
    }
    
    TEST_ASSERT_EQUALS(fixed_heap_peek_head(&heap, (void **)&head_peek), A_OK);
    TEST_ASSERT_EQUALS(head_peek, 1);
    TEST_ASSERT_EQUALS(test_heap_assert(&heap, test_min_heap_compare_func), TEST_OK);

    for (int64_t i = 1; i < 120; i++) {
        int64_t head = -1;
        TEST_ASSERT_EQUALS(fixed_heap_pop(&heap, (void **)&head), A_OK);
        TEST_ASSERT_EQUALS(test_heap_assert(&heap, test_min_heap_compare_func), TEST_OK);
        TEST_ASSERT_EQUALS(head, i);
    }

    TEST_ASSERT_EQUALS(fixed_heap_delete(&heap), A_OK);
    TEST_ASSERT_EQUALS(heap.heap, NULL);

    /* Create and destroy a max-heap */
    TEST_ASSERT_EQUALS(fixed_heap_new(&heap, 120, test_max_heap_compare_func), A_OK);

    for (int64_t i = 1; i < 120; i++) {
        TEST_ASSERT_EQUALS(fixed_heap_push(&heap, (void *)i), A_OK);
    }
    
    TEST_ASSERT_EQUALS(fixed_heap_peek_head(&heap, (void **)&head_peek), A_OK);
    TEST_ASSERT_EQUALS(head_peek, 119);

    for (int64_t i = 1; i < 120; i++) {
        int64_t head = -1;
        TEST_ASSERT_EQUALS(fixed_heap_pop(&heap, (void **)&head), A_OK);
        TEST_ASSERT_EQUALS(test_heap_assert(&heap, test_max_heap_compare_func), TEST_OK);
        TEST_ASSERT_EQUALS(head, 120-i);
    }

    TEST_ASSERT_EQUALS(fixed_heap_delete(&heap), A_OK);
    TEST_ASSERT_EQUALS(heap.heap, NULL);

    return TEST_OK;
}

#if 0
struct test_heap_entry {
    int64_t value;
    struct heap_node node;
};

static CAL_UNUSED
int test_heap_compare(struct heap_node *lhs, struct heap_node *rhs)
{
    struct test_heap_entry *_lhs = BL_CONTAINER_OF(lhs, struct test_heap_entry, node);
    struct test_heap_entry *_rhs = BL_CONTAINER_OF(rhs, struct test_heap_entry, node);

    return (int)_lhs->value - (int)_rhs->value;
}

TEST_DECL(test_heap)
{
    struct heap heap;

    struct test_heap_entry entries[42];

    /*
     * Create and delete a tree (smoke test)
     */
    TEST_ASSERT_EQUALS(heap_new(&heap, test_heap_compare), A_OK);
    TEST_ASSERT_EQUALS(heap_delete(&heap), A_OK);

    /*
     * Create a new heap and insert items
     */
    TEST_ASSERT_EQUALS(heap_new(&heap, test_heap_compare), A_OK);

    for (size_t i = 0; i < BL_ARRAY_ENTRIES(entries); i++) {
        struct test_heap_entry *entry = &entries[i];

        entry->value = (int64_t)i + (i % 2 ? -42 : 42);

        TEST_ASSERT_EQUALS(heap_push(&heap, &entry->node), A_OK);
    }

    TEST_ASSERT_EQUALS(heap_delete(&heap), A_OK);
    TEST_ASSERT_EQUALS(heap.root, NULL);

    return TEST_OK;
}

#endif

