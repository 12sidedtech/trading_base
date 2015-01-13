#include <tsl/test/helpers.h>
#include <tsl/config/engine.h>

#include <tsl/errors.h>

#include <string.h>

TEST_DECL(test_config)
{
    const char *test_file = "sample/config.json";
    struct config *cfg CAL_CLEANUP(config_delete) = NULL;
    char *path = NULL;
    int cpucore = -1;

    TEST_ASSERT_EQUALS(config_new(&cfg), A_OK);
    TEST_ASSERT_NOT_EQUALS(NULL, cfg);

    TEST_ASSERT_EQUALS(config_add(cfg, test_file), A_OK);

    TEST_ASSERT_EQUALS(config_get_string(cfg, &path, "cmdIf.unixSocket.path"), A_OK);
    TEST_ASSERT_NOT_EQUALS(path, NULL);
    TEST_ASSERT_EQUALS(strcmp("/tmp/RANDOMPRODID_CMD", path), 0);
    TEST_ASSERT_EQUALS(config_get_integer(cfg, &cpucore, "cmdIf.cpuCore"), A_OK);
    TEST_ASSERT_EQUALS(cpucore, 1);



    return TEST_OK;
}

