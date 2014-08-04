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
#include <tsl/speed.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

TEST_DECL(test_speed)
{
    char *bufptr = memalign(64, 128);
    char *outbuf = memalign(64, 128);

    TEST_ASSERT_NOT_EQUALS(bufptr, NULL);
    TEST_ASSERT_NOT_EQUALS(outbuf, NULL);

    TSL_SSE_PREPARE();

    memset(outbuf, 0, 128);

    for (int i = 0; i < 128; ++i) {
        bufptr[i] = 127 - i;
    }

    TSL_LOAD_ALIGNED_128(bufptr, xmm0);
    TSL_STORE_ALIGNED_128(outbuf, xmm0);

    for (int i = 0; i < 16; ++i) {
        TEST_ASSERT_EQUALS(outbuf[i], 127-i);
    }

    TEST_ASSERT_EQUALS(outbuf[16], 0);
    TEST_ASSERT_EQUALS(outbuf[17], 0);

    free(bufptr);
    return TEST_OK;
}

