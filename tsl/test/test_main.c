#include <tsl/test/helpers.h>
#include <tsl/panic.h>
#include <tsl/app.h>
#include <tsl/basic.h>

#include <stdlib.h>
#include <stdio.h>

TEST_DECL(test_basic) {
    TEST_ASSERT_EQUALS( BL_MIN3(1, 2, 3), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(2, 1, 3), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(3, 2, 1), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(3, 3, 1), 1 );
    TEST_ASSERT_EQUALS( BL_MIN3(1, 3, 3), 1 );
    return TEST_OK;
}

int main(int argc, char *argv[])
{
    if (AFAILED(app_init("tsl_tests"))) {
        PANIC("Failed to perform basic application initialization process.");
    }

    TEST_START(tsl);
    TEST_CASE(test_basic);
    TEST_CASE(test_alloc_basic);
    TEST_CASE(test_logalloc_basic);
    TEST_CASE(test_logalloc_fill_in);
    TEST_CASE(test_hash_table_basic);
    TEST_CASE(test_refcnt_basic);
    TEST_CASE(test_rbtree_lifecycle);
    TEST_CASE(test_rbtree_corner_cases);
    TEST_CASE(test_cpu_mask);
    TEST_CASE(test_time);
    TEST_CASE(test_speed);
    TEST_CASE(test_fixed_heap);
    TEST_CASE(test_queue);
    TEST_CASE(test_work_endpoint);
    TEST_CASE(test_work_thread);
    TEST_CASE(test_work_pool);
    TEST_CASE(test_megaqueue);
    TEST_CASE(test_config);
    TEST_FINISH(tsl);
    return EXIT_SUCCESS;
}

