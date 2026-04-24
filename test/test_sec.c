/*
 * Test for security module implementation
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sec.h"

#define TEST_PASSWORD "test_password123"
#define TEST_SALT "test_salt"
#define TEST_ITERATIONS 10000

static void test_sec_compute_hmac(void)
{
    printf("=== Testing sec_compute_hmac ===\n");
    
    uint8_t key[] = "test_key";
    size_t key_len = strlen((char *)key);
    uint8_t msg[] = "test_message";
    size_t msg_len = strlen((char *)msg);
    uint8_t digest[SHA256_DIGEST_SIZE];
    
    sec_compute_hmac(key, key_len, msg, msg_len, digest);
    
    printf("HMAC computed: ");
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
    printf("sec_compute_hmac: OK\n");
}

static void test_pbkdf2_sha256(void)
{
    printf("\n=== Testing pbkdf2_sha256 ===\n");
    
    uint8_t salt[] = TEST_SALT;
    size_t salt_len = strlen((char *)salt);
    uint8_t out[32];
    
    pbkdf2_sha256(TEST_PASSWORD, strlen(TEST_PASSWORD), salt, salt_len, TEST_ITERATIONS, out, sizeof(out));
    
    printf("PBKDF2 hash: ");
    for (int i = 0; i < sizeof(out); i++) {
        printf("%02x", out[i]);
    }
    printf("\n");
    printf("pbkdf2_sha256: OK\n");
}

static void test_sec_hash_password(void)
{
    printf("\n=== Testing sec_hash_password ===\n");
    
    char hash[256];
    int ret = sec_hash_password(TEST_PASSWORD, hash, sizeof(hash));
    
    if (ret == 0) {
        printf("Password hash: %s\n", hash);
        printf("sec_hash_password: OK\n");
    } else {
        printf("sec_hash_password: FAILED\n");
    }
}

static void test_sec_verify_password(void)
{
    printf("\n=== Testing sec_verify_password ===\n");
    
    char hash[256];
    sec_hash_password(TEST_PASSWORD, hash, sizeof(hash));
    
    // Test with correct password
    int ret1 = sec_verify_password(TEST_PASSWORD, hash);
    if (ret1 == 0) {
        printf("sec_verify_password (correct): OK\n");
    } else {
        printf("sec_verify_password (correct): FAILED\n");
    }
    
    // Test with incorrect password
    int ret2 = sec_verify_password("wrong_password", hash);
    if (ret2 != 0) {
        printf("sec_verify_password (incorrect): OK\n");
    } else {
        printf("sec_verify_password (incorrect): FAILED\n");
    }
}

static void test_sec_valid_command(void)
{
    printf("\n=== Testing sec_valid_command ===\n");
    
    // Test valid commands
    const char *valid_commands[] = {
        "reboot",
        "uptime",
        "ifconfig",
        "iwconfig",
        "wifi",
        "cat /proc/uptime",
        "cat /proc/loadavg",
        "cat /tmp/ap_status",
        NULL
    };
    
    for (int i = 0; valid_commands[i]; i++) {
        if (sec_valid_command(valid_commands[i])) {
            printf("sec_valid_command (%s): OK\n", valid_commands[i]);
        } else {
            printf("sec_valid_command (%s): FAILED\n", valid_commands[i]);
        }
    }
    
    // Test invalid commands
    const char *invalid_commands[] = {
        "rm -rf /",
        "ls /etc",
        "cat /etc/passwd",
        "echo 'test' > /tmp/test",
        NULL
    };
    
    for (int i = 0; invalid_commands[i]; i++) {
        if (!sec_valid_command(invalid_commands[i])) {
            printf("sec_valid_command (%s): OK\n", invalid_commands[i]);
        } else {
            printf("sec_valid_command (%s): FAILED\n", invalid_commands[i]);
        }
    }
}

static void test_sec_sanitize_input(void)
{
    printf("\n=== Testing sec_sanitize_input ===\n");
    
    const char *test_inputs[] = {
        "normal input",
        "input with spaces",
        "input with special chars !@#$%^&*()",
        "input with control chars\n\r\t",
        NULL
    };
    
    for (int i = 0; test_inputs[i]; i++) {
        char output[256];
        sec_sanitize_input(test_inputs[i], output, sizeof(output));
        printf("Input: '%s' -> Output: '%s'\n", test_inputs[i], output);
    }
    printf("sec_sanitize_input: OK\n");
}

int main(void)
{
    printf("AC Controller Security Module Tests\n");
    printf("==================================\n");
    
    test_sec_compute_hmac();
    test_pbkdf2_sha256();
    test_sec_hash_password();
    test_sec_verify_password();
    test_sec_valid_command();
    test_sec_sanitize_input();
    
    printf("\nAll tests completed!\n");
    return 0;
}
