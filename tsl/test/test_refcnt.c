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
#include <tsl/refcnt.h>
#include <tsl/basic.h>

#define TEST_MAGIC  0xbebafeca

struct test_refcnt {
    uint32_t magic;
    unsigned int id;
    struct refcnt rc;
};

/* Stupid mechanism to get a value out of the destructor */
static
int test_failed = 1;

void test_refcnt_destructor(struct refcnt *rc)
{
    struct test_refcnt *tr = BL_CONTAINER_OF(rc, struct test_refcnt, rc);

    if (tr->id != 42 || tr->magic != TEST_MAGIC) {
        test_failed = 1;
    }

    test_failed = 0;
}

TEST_DECL(test_refcnt_basic)
{
    struct test_refcnt first;

    first.magic = TEST_MAGIC;
    first.id = 42;

    /* Test 1: Initialize reference counting */
    TEST_ASSERT_EQUALS(refcnt_init(&first.rc, test_refcnt_destructor), A_OK);
    
    TEST_ASSERT(first.rc.refcnt == 1);

    /* Test 2: Check that refcnt_check does the right thing for a valid object */
    int valid = 0;
    TEST_ASSERT_EQUALS(refcnt_check(&first.rc, &valid), A_OK);
    TEST_ASSERT_NOT_EQUALS(valid, 0);

    /* Test 3: Get a new reference */
    TEST_ASSERT_EQUALS(refcnt_get(&first.rc), A_OK);
    TEST_ASSERT(first.rc.refcnt == 2);

    /* Test 4: Release a reference */
    TEST_ASSERT_EQUALS(refcnt_release(&first.rc), A_OK);
    TEST_ASSERT(first.rc.refcnt == 1);

    /* Test 5: Release final reference */
    TEST_ASSERT_EQUALS(refcnt_release(&first.rc), A_OK);
    TEST_ASSERT(first.rc.refcnt == 0);
    TEST_ASSERT(test_failed == 0);

    return TEST_OK;
}
