/*
 * Test for AP hash table implementation
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "aphash.h"

#define TEST_MAC1 "00:11:22:33:44:55"
#define TEST_MAC2 "AA:BB:CC:DD:EE:FF"
#define TEST_MAC3 "11:22:33:44:55:66"

static void mac_str_to_bytes(const char *mac_str, unsigned char *mac_bytes)
{
    sscanf(mac_str, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
           &mac_bytes[0], &mac_bytes[1], &mac_bytes[2],
           &mac_bytes[3], &mac_bytes[4], &mac_bytes[5]);
}

static void test_hash_init(void)
{
    printf("=== Testing hash_init ===\n");
    hash_init();
    printf("hash_init: OK\n");
}

static void test_hash_ap_add(void)
{
    printf("\n=== Testing hash_ap_add ===\n");
    
    unsigned char mac1[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC1, mac1);
    
    struct ap_hash_t *aphash1 = hash_ap_add(mac1);
    if (aphash1) {
        printf("hash_ap_add (MAC1): OK\n");
    } else {
        printf("hash_ap_add (MAC1): FAILED\n");
    }
    
    unsigned char mac2[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC2, mac2);
    
    struct ap_hash_t *aphash2 = hash_ap_add(mac2);
    if (aphash2) {
        printf("hash_ap_add (MAC2): OK\n");
    } else {
        printf("hash_ap_add (MAC2): FAILED\n");
    }
    
    // Test duplicate add
    struct ap_hash_t *aphash1_dup = hash_ap_add(mac1);
    if (aphash1_dup == aphash1) {
        printf("hash_ap_add (duplicate): OK\n");
    } else {
        printf("hash_ap_add (duplicate): FAILED\n");
    }
}

static void test_hash_ap(void)
{
    printf("\n=== Testing hash_ap ===\n");
    
    unsigned char mac1[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC1, mac1);
    
    struct ap_hash_t *aphash1 = hash_ap(mac1);
    if (aphash1) {
        printf("hash_ap (MAC1): OK\n");
    } else {
        printf("hash_ap (MAC1): FAILED\n");
    }
    
    unsigned char mac2[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC2, mac2);
    
    struct ap_hash_t *aphash2 = hash_ap(mac2);
    if (aphash2) {
        printf("hash_ap (MAC2): OK\n");
    } else {
        printf("hash_ap (MAC2): FAILED\n");
    }
    
    // Test non-existent MAC
    unsigned char mac3[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC3, mac3);
    
    struct ap_hash_t *aphash3 = hash_ap(mac3);
    if (!aphash3) {
        printf("hash_ap (non-existent): OK\n");
    } else {
        printf("hash_ap (non-existent): FAILED\n");
    }
}

static void test_hash_ap_count(void)
{
    printf("\n=== Testing hash_ap_count ===\n");
    
    int count = hash_ap_count();
    printf("hash_ap_count: %d (expected: 2)\n", count);
    if (count == 2) {
        printf("hash_ap_count: OK\n");
    } else {
        printf("hash_ap_count: FAILED\n");
    }
}

static void test_hash_ap_del(void)
{
    printf("\n=== Testing hash_ap_del ===\n");
    
    unsigned char mac1[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC1, mac1);
    
    hash_ap_del((char *)mac1);
    
    int count = hash_ap_count();
    printf("hash_ap_count after delete: %d (expected: 1)\n", count);
    if (count == 1) {
        printf("hash_ap_del: OK\n");
    } else {
        printf("hash_ap_del: FAILED\n");
    }
    
    // Test deleting non-existent MAC
    unsigned char mac3[ETH_ALEN];
    mac_str_to_bytes(TEST_MAC3, mac3);
    
    hash_ap_del((char *)mac3);
    count = hash_ap_count();
    if (count == 1) {
        printf("hash_ap_del (non-existent): OK\n");
    } else {
        printf("hash_ap_del (non-existent): FAILED\n");
    }
}

static void test_hash_ap_list_json(void)
{
    printf("\n=== Testing hash_ap_list_json ===\n");
    
    char buf[1024];
    int ret = hash_ap_list_json(buf, sizeof(buf));
    if (ret > 0) {
        printf("hash_ap_list_json: OK\n");
        printf("JSON output: %s\n", buf);
    } else {
        printf("hash_ap_list_json: FAILED\n");
    }
}

static void test_hash_cleanup(void)
{
    printf("\n=== Testing hash_cleanup ===\n");
    
    hash_cleanup();
    int count = hash_ap_count();
    printf("hash_ap_count after cleanup: %d (expected: 0)\n", count);
    if (count == 0) {
        printf("hash_cleanup: OK\n");
    } else {
        printf("hash_cleanup: FAILED\n");
    }
}

int main(void)
{
    printf("AC Controller AP Hash Table Tests\n");
    printf("==================================\n");
    
    test_hash_init();
    test_hash_ap_add();
    test_hash_ap();
    test_hash_ap_count();
    test_hash_ap_del();
    test_hash_ap_list_json();
    test_hash_cleanup();
    
    printf("\nAll tests completed!\n");
    return 0;
}
