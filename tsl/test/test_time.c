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

#include <tsl/time.h>
#include <tsl/basic.h>
#include <tsl/errors.h>

struct mock_time {
    struct time_source ts;
    uint64_t mock_clock;
};

uint64_t mock_time_get_timestamp(struct time_source *src)
{
    struct mock_time *tm = BL_CONTAINER_OF(src, struct mock_time, ts);

    return tm->mock_clock;
}

void mock_time_set_time(struct time_source *src, uint64_t ts)
{
    struct mock_time *tm = BL_CONTAINER_OF(src, struct mock_time, ts);
    tm->mock_clock = ts;
}


struct time_ops mock_time_ops = {
    .get_timestamp = mock_time_get_timestamp,
    .set_time = mock_time_set_time,
};


TEST_DECL(test_time)
{
    uint64_t test_ts = 0;

    struct mock_time test_time = {
        .ts = { .ops = &mock_time_ops,
                .name = "Mock Time", },
        .mock_clock = 0,
    };

    test_ts = time_get_time();

    TEST_ASSERT_NOT_EQUALS(test_ts, 0);

    uint64_t time_out = 1;
    TEST_ASSERT_EQUALS(time_get_from_source(&test_time.ts, &time_out), A_OK);
    TEST_ASSERT_EQUALS(time_out, 0);

    test_time.mock_clock = 420;
    TEST_ASSERT_EQUALS(time_get_from_source(&test_time.ts, &time_out), A_OK);
    TEST_ASSERT_EQUALS(time_out, 420);

    return TEST_OK;
}

