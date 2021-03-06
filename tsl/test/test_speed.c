#include <tsl/test/helpers.h>
#include <tsl/speed.h>

#include <stdlib.h>
#include <string.h>
#include <malloc.h>

void cleanup_char(char **ptr)
{
    if (NULL != *ptr) {
        free(*ptr);
    }
}

TEST_DECL(test_speed)
{
    char *bufptr CAL_CLEANUP(cleanup_char) = memalign(64, 128);
    char *outbuf CAL_CLEANUP(cleanup_char) = memalign(64, 128);

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

    return TEST_OK;
}

