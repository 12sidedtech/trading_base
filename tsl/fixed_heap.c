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
#include <tsl/fixed_heap.h>
#include <tsl/errors.h>
#include <tsl/assert.h>

#include <stdlib.h>
#include <string.h>

aresult_t fixed_heap_new(struct fixed_heap *heap, size_t max_items,
                         fixed_heap_compare_func_t compare)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(heap != NULL);
    TSL_ASSERT_ARG(max_items > 0);

    memset(heap, 0, sizeof(*heap));

    heap->heap = (void **)calloc(max_items, sizeof(void *));

    if (heap->heap == NULL) {
        ret = A_E_NOMEM;
        goto done;
    }

    heap->capacity = max_items;
    heap->entries = 0;
    heap->compare = compare;

done:
    return ret;
}

aresult_t fixed_heap_resize(struct fixed_heap *heap, size_t new_max_items)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(heap != NULL);
    TSL_ASSERT_ARG(new_max_items >= heap->entries);
    TSL_ASSERT_ARG(heap->heap != NULL);

    void *new_heap = realloc(heap->heap, sizeof(void *) * new_max_items);

    if (new_heap == NULL) {
        ret = A_E_NOMEM;
        goto done;
    }

    heap->heap = new_heap;
    heap->capacity = new_max_items;

done:
    return ret;
}

aresult_t fixed_heap_delete(struct fixed_heap *heap)
{
    aresult_t ret = A_OK;

    TSL_ASSERT_ARG(heap != NULL);

    if (heap->heap) {
        free(heap->heap);
        heap->heap = NULL;
    }

    memset(heap, 0, sizeof(*heap));

    return ret;
}

