#include <tsl/test/helpers.h>

#include <tsl/logalloc.h>
#include <tsl/alloc/logalloc_priv.h>

TEST_DECL(test_logalloc_basic)
{
    struct logalloc *talloc = NULL;

    void *prev = NULL, *cur = NULL;

    void *alloc[4] = { NULL, NULL, NULL, NULL };

    TEST_ASSERT_OK(logalloc_new(&talloc, 126, 128, NULL));
    TEST_ASSERT_NOT_EQUALS(talloc, NULL);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_OK(logalloc_alloc(talloc, 32 * 128, &alloc[i]));
    }

    TEST_ASSERT_EQUALS(logalloc_alloc(talloc, 128, &cur), A_E_NOMEM);

    TEST_ASSERT_EQUALS(logalloc_alloc(talloc, 128 * 128, &cur), A_E_NOMEM);

    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_OK(logalloc_free(&alloc[i]));
    }

    TEST_ASSERT_OK(logalloc_alloc(talloc, 128 * 128, &cur));
    TEST_ASSERT_OK(logalloc_free(&cur));

    for (int i = 0; i < 512; i++) {

        TEST_ASSERT_OK(logalloc_alloc(talloc, 120, &cur));

        if (NULL != prev) {
            TEST_ASSERT_OK(logalloc_free(&prev));
        }

        prev = cur;
    }

    TEST_ASSERT_OK(logalloc_free(&cur));

    for (int i = 0; i < 128; i++) {
        TEST_ASSERT_OK(logalloc_alloc(talloc, 120, &cur));
    }

    TEST_ASSERT_EQUALS(logalloc_alloc(talloc, 120, &cur), A_E_NOMEM);

    TEST_ASSERT_OK(logalloc_free(&prev));

    TEST_ASSERT_OK(logalloc_delete(&talloc));
    TEST_ASSERT_EQUALS(talloc, NULL);

    return TEST_OK;
}

TEST_DECL(test_logalloc_fill_in)
{
    struct logalloc *talloc = NULL;

    void *prev = NULL, *cur = NULL, *gap = NULL;
    size_t max_len = 0;

    TEST_ASSERT_OK(logalloc_new(&talloc, 126, 128, NULL));
    TEST_ASSERT_NOT_EQUALS(talloc, NULL);

    TEST_ASSERT_OK(logalloc_prepare_region(talloc, 16 * 128 - 10, &cur, &max_len));
    TEST_ASSERT_NOT_EQUALS(cur, NULL);
    TEST_ASSERT_EQUALS(max_len, 16 * 128);

    TEST_ASSERT_OK(logalloc_finalize_region(talloc, 14 * 128));

    prev = cur;

    TEST_ASSERT_OK(logalloc_alloc(talloc, (114 * 128 - 10), &cur));

    TEST_ASSERT_OK(logalloc_free(&prev));

    TEST_ASSERT_OK(logalloc_alloc(talloc, 14 * 128 - 10, &prev));

    TEST_ASSERT_OK(logalloc_free(&cur));
    TEST_ASSERT_OK(logalloc_free(&prev));

    TEST_ASSERT_OK(logalloc_alloc(talloc, 4 * 128 - 2, &prev));
    TEST_ASSERT_EQUALS(logalloc_alloc(talloc, 127 * 128, &cur), A_E_NOMEM);

    TEST_ASSERT_OK(logalloc_prepare_region(talloc, 110 * 128, &cur, &max_len));
    TEST_ASSERT_EQUALS(max_len, 110 * 128);
    TEST_ASSERT_OK(logalloc_finalize_region(talloc, 110 * 128 - 16));

    TEST_ASSERT_OK(logalloc_alloc(talloc, 14 * 128, &gap));

    TEST_ASSERT_OK(logalloc_delete(&talloc));
    TEST_ASSERT_EQUALS(talloc, NULL);

    return TEST_OK;
}

