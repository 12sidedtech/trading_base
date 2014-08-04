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
#include <tsl/megaqueue/megaqueue.h>
#include <tsl/errors.h>

#include <fcntl.h>
#include <sys/mman.h>

#include <stdint.h>

TEST_DECL(test_megaqueue)
{
    struct megaqueue mq;

    shm_unlink("megaqueue_mqtestseg");

    TEST_ASSERT_EQUALS(megaqueue_open(&mq, O_RDWR | O_CREAT, "mqtestseg", 128, 16 * 1024), A_OK);

    void *empty_slot = NULL;
    TEST_ASSERT_EQUALS(megaqueue_read_next_slot(&mq, &empty_slot), A_E_EMPTY);
    TEST_ASSERT_EQUALS(empty_slot, NULL);

    void *first_slot = NULL;
    TEST_ASSERT_EQUALS(megaqueue_next_slot(&mq, &first_slot), A_OK);
    TEST_ASSERT_NOT_EQUALS(first_slot, NULL);

    uint64_t *fsp = first_slot;

    fsp[0] = 0xdeadbeefcafebabe;
    fsp[1] = 0xbebafecaefbeadde;

    void *second_slot = NULL;
    TEST_ASSERT_EQUALS(megaqueue_advance(&mq), A_OK);
    TEST_ASSERT_EQUALS(megaqueue_next_slot(&mq, &second_slot), A_OK);
    TEST_ASSERT_NOT_EQUALS(second_slot, NULL);
    TEST_ASSERT_NOT_EQUALS(first_slot, second_slot);

    TEST_ASSERT_EQUALS((uint64_t)second_slot - (uint64_t)first_slot, 128);

    uint64_t *ssp = second_slot;
    ssp[0] = 0xefbeaddebebafeca;
    ssp[1] = 0xcafebabedeadbeef;

    void *first_slot_read = NULL;
    TEST_ASSERT_EQUALS(megaqueue_read_next_slot(&mq, &first_slot_read), A_OK);
    TEST_ASSERT_NOT_EQUALS(first_slot_read, NULL);

    uint64_t *fsrp = first_slot_read;
    TEST_ASSERT_EQUALS(fsrp[0], 0xdeadbeefcafebabe);
    TEST_ASSERT_EQUALS(fsrp[1], 0xbebafecaefbeadde);

    /* Advance the write pointer */
    TEST_ASSERT_EQUALS(megaqueue_advance(&mq), A_OK);

    /* Advance the read pointer */
    TEST_ASSERT_EQUALS(megaqueue_read_advance(&mq), A_OK);

    void *second_slot_read = NULL;
    TEST_ASSERT_EQUALS(megaqueue_read_next_slot(&mq, &second_slot_read), A_OK);
    TEST_ASSERT_NOT_EQUALS(second_slot_read, NULL);

    uint64_t *ssrp = second_slot_read;
    TEST_ASSERT_EQUALS(ssrp[0], 0xefbeaddebebafeca);
    TEST_ASSERT_EQUALS(ssrp[1], 0xcafebabedeadbeef);

    TEST_ASSERT_EQUALS(megaqueue_close(&mq, 1), A_OK);

    return TEST_OK;
}

