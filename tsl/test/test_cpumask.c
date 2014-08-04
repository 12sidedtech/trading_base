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
#include <tsl/cpumask.h>

TEST_DECL(test_cpu_mask)
{
    struct cpu_mask *msk = NULL;

    TEST_ASSERT_EQUALS(cpu_mask_create(&msk), A_OK);
    TEST_ASSERT_NOT_EQUALS(msk, NULL);

    TEST_ASSERT_EQUALS(cpu_mask_set(msk, 1), A_OK);
    int val = 0;
    TEST_ASSERT_EQUALS(cpu_mask_test(msk, 1, &val), A_OK);
    TEST_ASSERT(val == 1);

    TEST_ASSERT_EQUALS(cpu_mask_clear(msk, 1), A_OK);
    TEST_ASSERT_EQUALS(cpu_mask_test(msk, 1, &val), A_OK);
    TEST_ASSERT(val == 0);

    TEST_ASSERT_EQUALS(cpu_mask_set(msk, 2), A_OK);
    TEST_ASSERT_EQUALS(cpu_mask_test(msk, 2, &val), A_OK);
    TEST_ASSERT(val == 1);

    TEST_ASSERT_EQUALS(cpu_mask_clear_all(msk), A_OK);
    TEST_ASSERT_EQUALS(cpu_mask_test(msk, 2, &val), A_OK);
    TEST_ASSERT(val == 0);

    return TEST_OK;
}

