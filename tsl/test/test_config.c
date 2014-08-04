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
#include <tsl/config/engine.h>

#include <tsl/errors.h>

#include <string.h>

TEST_DECL(test_config)
{
    const char *test_file = "sample/config.json";
    struct config *cfg = NULL;
    char *path = NULL;
    int cpucore = -1;

    TEST_ASSERT_EQUALS(config_new(&cfg), A_OK);
    TEST_ASSERT_NOT_EQUALS(NULL, cfg);

    TEST_ASSERT_EQUALS(config_add(cfg, test_file), A_OK);

    TEST_ASSERT_EQUALS(config_get_string(cfg, &path, "cmdIf.unixSocket.path"), A_OK);
    TEST_ASSERT_NOT_EQUALS(path, NULL);
    TEST_ASSERT_EQUALS(strcmp("/tmp/RANDOMPRODID_CMD", path), 0);
    TEST_ASSERT_EQUALS(config_get_integer(cfg, &cpucore, "cmdIf.cpuCore"), A_OK);
    TEST_ASSERT_EQUALS(cpucore, 1);



    return TEST_OK;
}

