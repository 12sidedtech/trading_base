#include <tsl/test/helpers.h>

#include <tsl/time.h>
#include <tsl/basic.h>
#include <tsl/errors.h>

struct mock_time {
    struct time_source ts;
    uint64_t mock_clock;
};

uint64_t mock_time_get_timestamp(struct time_source *src)
{
    struct mock_time *tm = BL_CONTAINER_OF(src, struct mock_time, ts);

    return tm->mock_clock;
}

void mock_time_set_time(struct time_source *src, uint64_t ts)
{
    struct mock_time *tm = BL_CONTAINER_OF(src, struct mock_time, ts);
    tm->mock_clock = ts;
}


struct time_ops mock_time_ops = {
    .get_timestamp = mock_time_get_timestamp,
    .set_time = mock_time_set_time,
};


TEST_DECL(test_time)
{
    uint64_t test_ts = 0;

    struct mock_time test_time = {
        .ts = { .ops = &mock_time_ops,
                .name = "Mock Time", },
        .mock_clock = 0,
    };

    test_ts = time_get_time();

    TEST_ASSERT_NOT_EQUALS(test_ts, 0);

    uint64_t time_out = 1;
    TEST_ASSERT_EQUALS(time_get_from_source(&test_time.ts, &time_out), A_OK);
    TEST_ASSERT_EQUALS(time_out, 0);

    test_time.mock_clock = 420;
    TEST_ASSERT_EQUALS(time_get_from_source(&test_time.ts, &time_out), A_OK);
    TEST_ASSERT_EQUALS(time_out, 420);

    return TEST_OK;
}

