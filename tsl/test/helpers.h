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
#ifndef __INCLUDED_TSL_TEST_HELPERS_H__
#define __INCLUDED_TSL_TEST_HELPERS_H__

#include <stdio.h>
#include <stdlib.h>

#define TEST_DECL(x) int x(void)

#define TEST_OK                 0
#define TEST_FAILED             1

#define TEST_ASSERT(x) \
    do {                            \
        if (!(x)) {                 \
            fprintf(stderr, "%s:%d - Assertion failure: " #x " == FALSE\n", __FILE__, __LINE__); \
            return TEST_FAILED;     \
        }                           \
    } while (0)

#define TEST_ASSERT_EQUALS(x, y) \
    do {                            \
        if (!((x) == (y))) {        \
            fprintf(stderr, "%s:%d - Equality assertion failed: " #x " != " #y "\n", __FILE__, __LINE__); \
            return TEST_FAILED;     \
        }                           \
    } while (0)

#define TEST_ASSERT_NOT_EQUALS(x, y) \
    do {                            \
        if (!((x) != (y))) {        \
            fprintf(stderr, "%s:%d - Equality assertion failed: " #x " == " #y "\n", __FILE__, __LINE__); \
            return TEST_FAILED;     \
        }                           \
    } while (0)

#define TEST_START(name) \
    unsigned int __failures = 0; \
    fprintf(stderr, "Starting test suite " #name "\n");


#define TEST_CASE(func) \
    do {                                            \
        extern int func(void);                      \
        fprintf(stderr, "Running test " #func "\n"); \
        if ((func()) != 0) {                        \
            fprintf(stderr, "Failure in " #func " (#%d)\n", __COUNTER__); \
            __failures++;                           \
        }                                           \
        fflush(stderr);                             \
    } while (0)

#define TEST_FINISH(name) \
    do {                                            \
        fprintf(stderr, "Test complete. %u failures occurred in %u tests.\n", __failures, __COUNTER__); \
        exit(__failures != 0 ? EXIT_FAILURE : EXIT_SUCCESS); \
    } while (0)

#endif /* __INCLUDED_TSL_TEST_HELPERS_H__ */

