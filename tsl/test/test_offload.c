#include <tsl/test/helpers.h>
#include <tsl/offload/queue.h>
#include <tsl/offload/endpoint.h>
#include <tsl/offload/thread.h>
#include <tsl/offload/pool.h>

#include <limits.h>

TEST_DECL(test_queue)
{
    struct work_queue test_queue;

    TEST_ASSERT_EQUALS(work_queue_new(&test_queue, 32), A_OK);

    unsigned int size = 0;
    TEST_ASSERT_EQUALS(work_queue_size(&test_queue, &size), A_OK);
    TEST_ASSERT_EQUALS(size, 32);

    for (size_t i = 1; i < 32; ++i) {
        TEST_ASSERT_EQUALS(work_queue_push(&test_queue, (void *)i), A_OK);
        TEST_ASSERT_EQUALS(work_queue_fill(&test_queue, &size), A_OK);
        TEST_ASSERT_EQUALS(size, i);
    }

    TEST_ASSERT_NOT_EQUALS(work_queue_push(&test_queue, (void *)33), A_OK);

    for (size_t i = 1; i < 32; ++i) {
        size_t val = 33;
        TEST_ASSERT_EQUALS(work_queue_pop(&test_queue, (void **)&val), A_OK);
        TEST_ASSERT_EQUALS(val, i);
    }

    TEST_ASSERT_EQUALS(work_queue_fill(&test_queue, &size), A_OK);
    TEST_ASSERT_EQUALS(size, 0);

    TEST_ASSERT_EQUALS(work_queue_release(&test_queue), A_OK);

    return TEST_OK;
}

struct test_endpoint {
    struct work_endpoint ep;
    volatile int state CAL_ALIGN(64);
    volatile int shutdown CAL_ALIGN(64);
};

static
aresult_t test_ep_poll(struct work_endpoint *ep, unsigned int *wait)
{
    struct test_endpoint *tep = BL_CONTAINER_OF(ep, struct test_endpoint, ep);

    if (tep->state != INT_MAX) {
        tep->state++;
    }

    *wait = 0;

    return A_OK;
}

static
aresult_t test_ep_startup(struct work_endpoint *ep)
{
    struct test_endpoint *tep = BL_CONTAINER_OF(ep, struct test_endpoint, ep);

    tep->state = 0;

    return A_OK;
}

static
aresult_t test_ep_shutdown(struct work_endpoint *ep)
{
    struct test_endpoint *tep = BL_CONTAINER_OF(ep, struct test_endpoint, ep);

    tep->state = -1;
    tep->shutdown = 1;

    return A_OK;
}

static struct work_endpoint_ops test_ep_ops = {
    .poll = test_ep_poll,
    .startup = test_ep_startup,
    .shutdown = test_ep_shutdown,
};

TEST_DECL(test_work_endpoint)
{
    struct test_endpoint tep;
    tep.state = 42;
    tep.shutdown = 0;

    TEST_ASSERT_EQUALS(work_endpoint_init(&tep.ep, &test_ep_ops, &tep), A_OK);

    TEST_ASSERT_EQUALS(work_endpoint_startup(&tep.ep), A_OK);

    TEST_ASSERT_EQUALS(tep.state, 0);
    
    unsigned int wait_millis = 0;
    TEST_ASSERT_EQUALS(work_endpoint_poll(&tep.ep, &wait_millis), A_OK);
    TEST_ASSERT_EQUALS(tep.state, 1);
    TEST_ASSERT_EQUALS(work_endpoint_poll(&tep.ep, &wait_millis), A_OK);
    TEST_ASSERT_EQUALS(tep.state, 2);

    TEST_ASSERT_EQUALS(work_endpoint_shutdown(&tep.ep), A_OK);
    TEST_ASSERT_EQUALS(tep.state, -1);
    TEST_ASSERT_EQUALS(tep.shutdown, 1);
    return TEST_OK;
}

TEST_DECL(test_work_thread)
{
    struct test_endpoint tep;
    tep.state = 0;
    tep.shutdown = 0;

    TEST_ASSERT_EQUALS(work_endpoint_init(&tep.ep, &test_ep_ops, &tep), A_OK);

    struct work_thread *thread = NULL;
    TEST_ASSERT_EQUALS(work_thread_new(&thread, 1), A_OK);
    TEST_ASSERT_NOT_EQUALS(thread, NULL);

    TEST_ASSERT_EQUALS(work_thread_start(thread), A_OK);

    /* Add the endpoint */
    TEST_ASSERT_EQUALS(work_thread_add_endpoint(thread, &tep.ep), A_OK);

    volatile unsigned loop_ctr = 0;

    while (tep.state < 100 && loop_ctr < 10000000) { loop_ctr++; }

    TEST_ASSERT_NOT_EQUALS(tep.state, 0);

    /* Request the work thread shuts itself down */
    TEST_ASSERT_EQUALS(work_thread_shutdown(thread), A_OK);

    loop_ctr = 0;
    while (tep.shutdown != 1 && loop_ctr < 10000000) { loop_ctr++; }

    TEST_ASSERT_EQUALS(tep.shutdown, 1);

    /* Destroy the thread */
    loop_ctr = 0;
    while (thread != NULL && loop_ctr < 1000000) {
        TEST_ASSERT_EQUALS(work_thread_destroy(&thread), A_OK);
        loop_ctr++;
    }

    TEST_ASSERT_EQUALS(thread, NULL);

    return TEST_OK;
}

TEST_DECL(test_work_pool)
{
    struct work_pool *pool = NULL;

    TEST_ASSERT_EQUALS(work_pool_create(&pool), A_OK);
    TEST_ASSERT_NOT_EQUALS(pool, NULL);

    unsigned int tid = 0;
    TEST_ASSERT_EQUALS(work_pool_add_thread(pool, 1, &tid), A_OK);
    TEST_ASSERT_NOT_EQUALS(tid, 0);

    struct test_endpoint tep;
    tep.state = 0;
    tep.shutdown = 0;

    TEST_ASSERT_EQUALS(work_endpoint_init(&tep.ep, &test_ep_ops, &tep), A_OK);

    TEST_ASSERT_EQUALS(work_pool_add_endpoint(pool, tid, &tep.ep), A_OK);

    volatile unsigned loop_ctr = 0;
    while (tep.state < 100 && loop_ctr < 10000000) { loop_ctr++; }

    TEST_ASSERT_NOT_EQUALS(tep.state, 0);

    TEST_ASSERT_EQUALS(work_pool_shutdown(pool), A_OK);

    loop_ctr = 0;
    while (loop_ctr < 10000000) { loop_ctr++; }

    TEST_ASSERT_EQUALS(work_pool_destroy(&pool), A_OK);
    
    loop_ctr = 0;
    while (tep.shutdown !=  1 && loop_ctr < 10000000) { loop_ctr++; }

    TEST_ASSERT_EQUALS(tep.shutdown, 1);

    return TEST_OK;
}

