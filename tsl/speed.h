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
#ifndef __INCLUDED_TSL_SPEED_H__
#define __INCLUDED_TSL_SPEED_H__

/** \file speed.h
 * Really low-level primitives for building up code that uses AVX/SSE for
 * vector operations
 */

#ifdef __AVX__
#define TSL_SSE_PREPARE() \
    ({ __asm__ __volatile__ ( "    vzeroupper\n" ); })
#else
#define TSL_SSE_PREPARE()
#endif

#define TSL_LOAD_ALIGNED_128(__src, __reg)              \
        ({                                              \
            __asm__ __volatile__ (                      \
                    "    movdqa %[src], %%" #__reg "\n"  \
                    :                                   \
                    : [src]"m"(*(__src))                \
                    : #__reg                            \
                );                                      \
        })

#define TSL_LOAD_ALIGNED_256(__src, __reg, __xmm)       \
        ({                                              \
            __asm__ __volatile__ (                      \
                    "    vmovdqa %[src], %%" #__reg "\n" \
                    :                                   \
                    : [src]"m"(*(__src))                \
                    : #__xmm                            \
                );                                      \
        })

#define TSL_STORE_ALIGNED_128(__src, __reg)             \
        ({                                              \
            __asm__ __volatile__ (                      \
                    "    movntdq %%" #__reg ", %[src]\n"   \
                    :                                   \
                    : [src]"m"(*(__src))                \
                    : #__reg                            \
                );                                      \
        })

#define TSL_STORE_ALIGNED_256(__src, __reg, __xmm)      \
        ({                                              \
            __asm__ __volatile__ (                      \
                    "    vmovdqa %%" #__reg ", %[src]\n" \
                    :                                   \
                    : [src]"m"i(*(__src))               \
                    : #__xmm                            \
                );                                      \
        })

#endif /* __INCLUDED_TSL_SPEED_H__ */

