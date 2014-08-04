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
#ifndef __TSL_ERRORS_H__
#define __TSL_ERRORS_H__

#include <tsl/version.h>
#include <tsl/result.h>

/**
 * \group aresult_errors aresult_t Return Codes
 * All valid error codes for generic system facilities (such as the TSL and low-level
 * handler code).
 * @{
 */
#define A_OK            ARESULT_CODE(0, 0, FACIL_SYSTEM, 0)         /** Everything is OK */
#define A_E_NOMEM       ARESULT_ERROR(FACIL_SYSTEM, 1)              /** Out of memory */
#define A_E_BADARGS     ARESULT_ERROR(FACIL_SYSTEM, 2)              /** Bad arguments */
#define A_E_NOTFOUND    ARESULT_ERROR(FACIL_SYSTEM, 3)              /** Not found */
#define A_E_BUSY        ARESULT_ERROR(FACIL_SYSTEM, 4)              /** Busy/In Use */
#define A_E_INVAL       ARESULT_ERROR(FACIL_SYSTEM, 5)              /** Invalid reference */
#define A_E_NOTHREAD    ARESULT_ERROR(FACIL_SYSTEM, 6)              /** Thread not found */
#define A_E_EMPTY       ARESULT_ERROR(FACIL_SYSTEM, 7)              /** Target is empty */
#define A_E_NO_SOCKET   ARESULT_ERROR(FACIL_SYSTEM, 8)              /** Invalid reference */
#define A_E_NOENT       ARESULT_ERROR(FACIL_SYSTEM, 9)              /** Invalid entity */
#define A_E_INV_DATE    ARESULT_ERROR(FACIL_SYSTEM, 10)             /** Invalid date */
#define A_E_NOSPC       ARESULT_ERROR(FACIL_SYSTEM, 11)             /** No space remains */
#define A_E_EXIST       ARESULT_ERROR(FACIL_SYSTEM, 12)             /** Item already exists */
/** Unknown message or data type */
#define A_E_UNKNOWN     ARESULT_ERROR(FACIL_SYSTEM, 13)
/** Process is finished */
#define A_E_DONE        ARESULT_ERROR(FACIL_SYSTEM, 14)
/*@}*/
#endif /* __TSL_ERRORS_H__ */

