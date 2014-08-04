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
#include <tsl/panic.h>
#include <tsl/app.h>
#include <tsl/basic.h>

#include <stdlib.h>
#include <stdio.h>

TEST_DECL(test_basic) {
    TEST_ASSERT_EQUALS( BL_MIN3(1, 2, 3), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(2, 1, 3), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(3, 2, 1), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(3, 3, 1), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(1, 3, 3), 1 );
    return TEST_OK;
}

int main(int argc, char *argv[])
{
    if (AFAILED(app_init("tsl_tests"))) {
        PANIC("Failed to perform basic application initialization process.");
    }

    TEST_START(tsl);
    TEST_CASE(test_basic);
    TEST_CASE(test_alloc_basic);
    TEST_CASE(test_refcnt_basic);
    TEST_CASE(test_rbtree_lifecycle);
    TEST_CASE(test_rbtree_corner_cases);
    TEST_CASE(test_cpu_mask);
    TEST_CASE(test_time);
    TEST_CASE(test_speed);
    TEST_CASE(test_fixed_heap);
    TEST_CASE(test_queue);
    TEST_CASE(test_work_endpoint);
    TEST_CASE(test_work_thread);
    TEST_CASE(test_work_pool);
    TEST_CASE(test_megaqueue);
    TEST_CASE(test_config);
    TEST_FINISH(tsl);
    return EXIT_SUCCESS;
}

