/*
 * ============================================================================
 *
 *       Filename:  acctl_ap_test.h
 *
 *    Description:  Common test utilities for acctl-ap
 *
 *        Version:  2.0
 *        Created:  2026-01-15
 *
 * ============================================================================
 */

#ifndef ACCTL_AP_TEST_H
#define ACCTL_AP_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define TEST_SUITE_NAME "acctl-ap"
#define TEST_PASSED 0
#define TEST_FAILED 1

typedef struct {
    const char *name;
    void (*test_func)(void);
} test_case_t;

typedef struct {
    const char *name;
    test_case_t *tests;
    int count;
    int passed;
    int failed;
} test_suite_t;

extern test_suite_t g_test_suite;

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  [FAIL] %s: %s\n", __func__, message); \
            return TEST_FAILED; \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            printf("  [FAIL] %s: Expected %ld, got %ld\n", __func__, (long)(expected), (long)(actual)); \
            return TEST_FAILED; \
        } \
    } while (0)

#define TEST_ASSERT_STRING_EQUAL(expected, actual) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("  [FAIL] %s: Expected '%s', got '%s'\n", __func__, (expected), (actual)); \
            return TEST_FAILED; \
        } \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("  [FAIL] %s: Pointer is NULL\n", __func__); \
            return TEST_FAILED; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        printf("  Running %s... ", #test_func); \
        fflush(stdout); \
        if (test_func() == TEST_PASSED) { \
            printf("[PASS]\n"); \
            g_test_suite.passed++; \
        } else { \
            printf("[FAIL]\n"); \
            g_test_suite.failed++; \
        } \
    } while (0)

#define ADD_TEST_CASE(test_func) \
    do { \
        g_test_suite.tests[g_test_suite.count].name = #test_func; \
        g_test_suite.tests[g_test_suite.count++].test_func = test_func; \
    } while (0)

void test_suite_init(const char *name, int max_tests);
int test_suite_run(void);
void test_suite_summary(void);

#endif /* ACCTL_AP_TEST_H */
