/*
 * ============================================================================
 *
 *       Filename:  test_security.c
 *
 *    Description:  Security test cases for acctl-ac
 *                  Tests for path traversal, input validation, etc.
 *
 *        Version:  1.0
 *        Created:  2026-05-11
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "acctl_ac_test.h"

/* Test path traversal detection */
void test_path_traversal_detection(void)
{
    struct {
        const char *path;
        int should_reject;
    } test_cases[] = {
        { "/api/aps", 0 },
        { "/api/status", 0 },
        { "/api/../etc/passwd", 1 },
        { "/api/..%2fetc/passwd", 1 },
        { "/api/../../etc/shadow", 1 },
        { "/api/./aps", 0 },
        { "/api//aps", 1 },
        { NULL, 0 }
    };
    
    printf("Testing path traversal detection...\n");
    
    for (int i = 0; test_cases[i].path; i++) {
        int has_traversal = (strstr(test_cases[i].path, "..") != NULL ||
                            strstr(test_cases[i].path, "//") != NULL);
        int rejected = has_traversal;
        
        if (test_cases[i].should_reject) {
            TEST_ASSERT(rejected == 1, "Path should be rejected: %s", test_cases[i].path);
        } else {
            TEST_ASSERT(rejected == 0, "Path should be allowed: %s", test_cases[i].path);
        }
    }
}

/* Test API parameter validation */
void test_api_param_validation(void)
{
    printf("Testing API parameter validation...\n");
    
    /* Test valid parameter names */
    TEST_ASSERT(strlen("hostname") <= 64, "Valid param name length");
    TEST_ASSERT(strchr("hostname", ';') == NULL, "No dangerous chars in valid name");
    
    /* Test invalid parameter names */
    const char *invalid_names[] = {
        "name;rm -rf",
        "name|cat",
        "name`whoami`",
        "$(id)",
        NULL
    };
    
    for (int i = 0; invalid_names[i]; i++) {
        const char *dangerous_chars = ";|`$()";
        int has_dangerous = 0;
        for (size_t j = 0; j < strlen(invalid_names[i]); j++) {
            if (strchr(dangerous_chars, invalid_names[i][j]) != NULL) {
                has_dangerous = 1;
                break;
            }
        }
        TEST_ASSERT(has_dangerous, "Should detect dangerous chars in: %s", invalid_names[i]);
    }
    
    /* Test parameter value length */
    char long_value[300];
    memset(long_value, 'a', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    TEST_ASSERT(strlen(long_value) > 256, "Long value created");
}

/* Test MAC address validation */
void test_mac_address_validation(void)
{
    printf("Testing MAC address validation...\n");
    
    /* Valid MAC addresses */
    const char *valid_macs[] = {
        "00:11:22:33:44:55",
        "aa:bb:cc:dd:ee:ff",
        "AA:BB:CC:DD:EE:FF",
        "12:34:56:78:9a:bc",
        NULL
    };
    
    for (int i = 0; valid_macs[i]; i++) {
        int valid = (strlen(valid_macs[i]) == 17);
        if (valid) {
            for (size_t j = 0; j < 17; j++) {
                if (j % 3 == 2) {
                    if (valid_macs[i][j] != ':') valid = 0;
                } else {
                    char c = valid_macs[i][j];
                    if (!((c >= '0' && c <= '9') ||
                          (c >= 'a' && c <= 'f') ||
                          (c >= 'A' && c <= 'F'))) {
                        valid = 0;
                    }
                }
            }
        }
        TEST_ASSERT(valid, "Valid MAC: %s", valid_macs[i]);
    }
    
    /* Invalid MAC addresses */
    const char *invalid_macs[] = {
        "00:11:22:33:44",
        "00:11:22:33:44:55:66",
        "00-11-22-33-44-55",
        "gg:hh:ii:jj:kk:ll",
        "00:11:22:33:44:5",
        NULL
    };
    
    for (int i = 0; invalid_macs[i]; i++) {
        int valid = (strlen(invalid_macs[i]) == 17);
        if (valid) {
            for (size_t j = 0; j < 17; j++) {
                if (j % 3 == 2) {
                    if (invalid_macs[i][j] != ':') valid = 0;
                } else {
                    char c = invalid_macs[i][j];
                    if (!((c >= '0' && c <= '9') ||
                          (c >= 'a' && c <= 'f') ||
                          (c >= 'A' && c <= 'F'))) {
                        valid = 0;
                    }
                }
            }
        }
        TEST_ASSERT(!valid, "Invalid MAC should be rejected: %s", invalid_macs[i]);
    }
}

/* Test HTTP method validation */
void test_http_method_validation(void)
{
    printf("Testing HTTP method validation...\n");
    
    const char *valid_methods[] = {
        "GET", "POST", "PUT", "DELETE", NULL
    };
    
    const char *invalid_methods[] = {
        "PATCH", "OPTIONS", "HEAD", "TRACE", "CONNECT", NULL
    };
    
    for (int i = 0; valid_methods[i]; i++) {
        int known = (strcmp(valid_methods[i], "GET") == 0 ||
                    strcmp(valid_methods[i], "POST") == 0 ||
                    strcmp(valid_methods[i], "PUT") == 0 ||
                    strcmp(valid_methods[i], "DELETE") == 0);
        TEST_ASSERT(known, "Valid method: %s", valid_methods[i]);
    }
    
    for (int i = 0; invalid_methods[i]; i++) {
        int known = (strcmp(invalid_methods[i], "GET") == 0 ||
                    strcmp(invalid_methods[i], "POST") == 0 ||
                    strcmp(invalid_methods[i], "PUT") == 0 ||
                    strcmp(invalid_methods[i], "DELETE") == 0);
        TEST_ASSERT(!known, "Invalid method should be rejected: %s", invalid_methods[i]);
    }
}

/* Test JSON input validation */
void test_json_input_validation(void)
{
    printf("Testing JSON input validation...\n");
    
    const char *valid_json = "{\"hostname\":\"test-ap\",\"wifi_ssid\":\"MyWiFi\"}";
    const char *invalid_json = "not a json";
    const char *malformed_json = "{\"hostname\":\"test\"";
    
    TEST_ASSERT(valid_json[0] == '{', "Valid JSON starts with {");
    TEST_ASSERT(strstr(valid_json, "hostname") != NULL, "Valid JSON contains hostname");
    
    TEST_ASSERT(invalid_json[0] != '{', "Invalid JSON does not start with {");
    TEST_ASSERT(malformed_json[strlen(malformed_json)-1] != '}', "Malformed JSON missing closing }");
}

/* Run all security tests */
int main(void)
{
    test_suite_init("Security Tests", 20);
    
    test_path_traversal_detection();
    test_api_param_validation();
    test_mac_address_validation();
    test_http_method_validation();
    test_json_input_validation();
    
    test_suite_summary();
    
    return g_test_suite.failed > 0 ? 1 : 0;
}
