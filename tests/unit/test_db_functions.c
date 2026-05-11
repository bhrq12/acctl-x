/*
 * ============================================================================
 *
 *       Filename:  test_db_functions.c
 *
 *    Description:  单元测试 - 数据库函数
 *                  覆盖: db_ap_list, db_ap_get, db_update_resource
 *
 *        Version:  1.0
 *        Created:  2026-05-11
 *       Compiler:  gcc
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "db.h"

/* 测试辅助宏 */
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "[FAIL] %s: %s\n", __func__, msg); \
            return 1; \
        } \
    } while (0)

#define TEST_PASS() \
    do { \
        fprintf(stdout, "[PASS] %s\n", __func__); \
        return 0; \
    } while (0)

/* 测试前设置 */
static int setup_test_db(void)
{
    /* 创建测试数据库目录 */
    system("mkdir -p /tmp/acctl-test");

    /* 清理旧数据库 */
    unlink("/tmp/acctl-test/ac.json");
    unlink("/tmp/acctl-test/ac.json.bak");

    return 0;
}

/* 测试后清理 */
static void cleanup_test_db(void)
{
    unlink("/tmp/acctl-test/ac.json");
    unlink("/tmp/acctl-test/ac.json.bak");
    rmdir("/tmp/acctl-test");
}

/* ============================================================================
 * db_update_resource 测试
 * ============================================================================ */

static int test_db_update_resource_valid_key(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    /* 测试有效的 ip_start key */
    ret = db_update_resource("ip_start", "192.168.1.100");
    TEST_ASSERT(ret == 0, "db_update_resource ip_start failed");

    /* 测试有效的 ip_end key */
    ret = db_update_resource("ip_end", "192.168.1.200");
    TEST_ASSERT(ret == 0, "db_update_resource ip_end failed");

    /* 测试有效的 ip_mask key */
    ret = db_update_resource("ip_mask", "255.255.255.0");
    TEST_ASSERT(ret == 0, "db_update_resource ip_mask failed");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

static int test_db_update_resource_invalid_key(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    /* 测试无效的 key */
    ret = db_update_resource("invalid_key", "some_value");
    TEST_ASSERT(ret == -1, "db_update_resource should fail for invalid key");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

static int test_db_update_resource_null_params(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    /* 测试 NULL key */
    ret = db_update_resource(NULL, "value");
    TEST_ASSERT(ret == -1, "db_update_resource should fail with NULL key");

    /* 测试 NULL value（应该允许） */
    ret = db_update_resource("ip_start", NULL);
    TEST_ASSERT(ret == 0, "db_update_resource should accept NULL value");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

/* ============================================================================
 * db_ap_list 测试
 * ============================================================================ */

static int test_db_ap_list_empty(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    char json_buf[4096];
    ret = db_ap_list(json_buf, sizeof(json_buf));
    TEST_ASSERT(ret == 0, "db_ap_list failed");
    TEST_ASSERT(strlen(json_buf) > 0, "json_buf should not be empty");

    /* 空列表应该是 "[]" */
    TEST_ASSERT(strcmp(json_buf, "[]") == 0 ||
               strstr(json_buf, "[]") != NULL, "Empty list should be '[]'");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

static int test_db_ap_list_null_params(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    /* 测试 NULL json_buf */
    ret = db_ap_list(NULL, 4096);
    TEST_ASSERT(ret == -1, "db_ap_list should fail with NULL json_buf");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

/* ============================================================================
 * db_ap_get 测试
 * ============================================================================ */

static int test_db_ap_get_nonexistent(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    char json_buf[4096];
    ret = db_ap_get("00:11:22:33:44:55", json_buf, sizeof(json_buf));
    TEST_ASSERT(ret == -1, "db_ap_get should fail for nonexistent MAC");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

static int test_db_ap_get_null_params(void)
{
    setup_test_db();

    db_t *test_db = NULL;
    int ret = db_init(&test_db);
    TEST_ASSERT(ret == 0, "db_init failed");

    char json_buf[4096];

    /* 测试 NULL mac */
    ret = db_ap_get(NULL, json_buf, sizeof(json_buf));
    TEST_ASSERT(ret == -1, "db_ap_get should fail with NULL mac");

    /* 测试 NULL json_buf */
    ret = db_ap_get("00:11:22:33:44:55", NULL, sizeof(json_buf));
    TEST_ASSERT(ret == -1, "db_ap_get should fail with NULL json_buf");

    db_close(test_db);
    cleanup_test_db();
    TEST_PASS();
}

/* ============================================================================
 * 测试运行器
 * ============================================================================ */

typedef int (*test_func_t)(void);

static struct {
    const char *name;
    test_func_t func;
} tests[] = {
    {"db_update_resource_valid_key", test_db_update_resource_valid_key},
    {"db_update_resource_invalid_key", test_db_update_resource_invalid_key},
    {"db_update_resource_null_params", test_db_update_resource_null_params},
    {"db_ap_list_empty", test_db_ap_list_empty},
    {"db_ap_list_null_params", test_db_ap_list_null_params},
    {"db_ap_get_nonexistent", test_db_ap_get_nonexistent},
    {"db_ap_get_null_params", test_db_ap_get_null_params},
};

#define NUM_TESTS (sizeof(tests) / sizeof(tests[0]))

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    int passed = 0;
    int failed = 0;

    printf("========================================\n");
    printf("  acctl 单元测试 - 数据库函数\n");
    printf("========================================\n\n");

    for (size_t i = 0; i < NUM_TESTS; i++) {
        printf("运行测试 [%zu/%zu]: %s\n", i + 1, NUM_TESTS, tests[i].name);
        if (tests[i].func() == 0) {
            passed++;
        } else {
            failed++;
        }
        printf("\n");
    }

    printf("========================================\n");
    printf("  测试结果: %d 通过, %d 失败\n", passed, failed);
    printf("========================================\n");

    return failed > 0 ? 1 : 0;
}
