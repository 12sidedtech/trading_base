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

