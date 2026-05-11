/*
 * ============================================================================
 *
 *       Filename:  acctl_ap_test.c
 *
 *    Description:  Test framework implementation for acctl-ap
 *
 *        Version:  2.0
 *        Created:  2026-01-15
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "acctl_ap_test.h"

test_suite_t g_test_suite;

void test_suite_init(const char *name, int max_tests)
{
    memset(&g_test_suite, 0, sizeof(g_test_suite));
    g_test_suite.name = name;
    g_test_suite.tests = (test_case_t *)malloc(sizeof(test_case_t) * max_tests);
    g_test_suite.count = 0;
    g_test_suite.passed = 0;
    g_test_suite.failed = 0;
}

int test_suite_run(void)
{
    printf("\n=== %s ===\n", g_test_suite.name);

    for (int i = 0; i < g_test_suite.count; i++) {
        RUN_TEST(g_test_suite.tests[i].test_func);
    }

    return g_test_suite.failed > 0 ? 1 : 0;
}

void test_suite_summary(void)
{
    printf("\n=== Test Summary ===\n");
    printf("Total:  %d\n", g_test_suite.count);
    printf("Passed: %d\n", g_test_suite.passed);
    printf("Failed: %d\n", g_test_suite.failed);

    if (g_test_suite.failed > 0) {
        printf("\n!!! %d test(s) failed !!!\n", g_test_suite.failed);
    } else {
        printf("\n*** All tests passed ***\n");
    }
}
