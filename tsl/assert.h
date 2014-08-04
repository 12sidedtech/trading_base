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
#ifndef __TSL_ASSERT_H__
#define __TSL_ASSERT_H__

#include <tsl/version.h>
#include <tsl/errors.h>
#include <tsl/cal.h>

#include <assert.h>

#ifdef _TSL_DEBUG
#include <stdio.h>
#endif /* _TSL_DEBUG */

#ifndef _TSL_DEBUG
/** \brief Assert state about an argument
 * Macro to check if a given assertion about an argument is true
 * \param x Assertion about an argument
 * \note Function must return an aresult_t
 */
#define TSL_ASSERT_ARG(x) \
    do {                                \
        if (CAL_UNLIKELY(!(x))) {       \
            return A_E_BADARGS;         \
        }                               \
    } while (0)

/**
 * \brief Assert state about an argument only in debug builds
 * Macro to check if a given assertion is true, but only applicable
 * in debug builds. When building a performance build, the TSL_ASSERT_DEBUG
 * macro becomes a no-op
 */
#define TSL_ASSERT_ARG_DEBUG(...)

#else /* defined(_TSL_DEBUG) */

/** \brief Assert state about an argument
 * Macro to check if a given assertion about an argument is true
 * \param x Assertion about an argument
 * \note Function must return an aresult_t
 */
#define TSL_ASSERT_ARG(x) \
    do {                                \
        if (CAL_UNLIKELY(!(x))) {       \
            printf("Assertion failed! %s:%d (function %s): " #x " == FALSE\n", \
                    __FILE__, __LINE__, __FUNCTION__); \
            return A_E_BADARGS;         \
        }                               \
    } while (0)

/**
 * \brief Assert state about an argument only in debug builds
 * Macro to check if a given assertion is true, but only applicable
 * in debug builds. When building a performance build, the TSL_ASSERT_DEBUG
 * macro becomes a no-op
 */
#define TSL_ASSERT_ARG_DEBUG(x) TSL_ASSERT_ARG((x))

#endif /* defined(_TSL_DEBUG) */

#endif /* __TSL_ASSERT_H__ */

