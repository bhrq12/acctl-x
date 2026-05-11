/*
 * ============================================================================
 *
 *       Filename:  test_capwap.c
 *
 *    Description:  CAPWAP protocol unit tests for acctl-ac
 *
 *        Version:  2.0
 *        Created:  2026-01-15
 *
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../src/include/capwap/capwap.h"
#include "acctl_ac_test.h"

test_suite_t g_test_suite;

void test_capwap_header_create(void)
{
    struct capwap_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.version = CAPWAP_VERSION;
    hdr.type = CAPWAP_MSG_DISCOVERY_REQUEST;
    hdr.length = sizeof(hdr);
    hdr.session_id_lo = 0x12345678;
    hdr.session_id_hi = 0xABCDEF00;
    hdr.sequence = 1;

    TEST_ASSERT(hdr.version == CAPWAP_VERSION, "Version mismatch");
    TEST_ASSERT(hdr.type == CAPWAP_MSG_DISCOVERY_REQUEST, "Type mismatch");
    TEST_ASSERT(hdr.length == sizeof(struct capwap_header), "Length mismatch");
}

void test_capwap_state_transitions(void)
{
    TEST_ASSERT(CAPWAP_IS_REQUEST(CAPWAP_MSG_DISCOVERY_REQUEST), "Discovery request check failed");
    TEST_ASSERT(CAPWAP_IS_RESPONSE(CAPWAP_MSG_DISCOVERY_RESPONSE), "Discovery response check failed");
    TEST_ASSERT(!CAPWAP_IS_REQUEST(CAPWAP_MSG_DISCOVERY_RESPONSE), "Response should not be request");
}

void test_capwap_msg_category(void)
{
    TEST_ASSERT_EQUAL(0, CAPWAP_GET_CATEGORY(CAPWAP_MSG_DISCOVERY_REQUEST));
    TEST_ASSERT_EQUAL(1, CAPWAP_GET_CATEGORY(CAPWAP_MSG_JOIN_REQUEST));
    TEST_ASSERT_EQUAL(2, CAPWAP_GET_CATEGORY(CAPWAP_MSG_CONFIG_REQUEST));
    TEST_ASSERT_EQUAL(3, CAPWAP_GET_CATEGORY(CAPWAP_MSG_ECHO_REQUEST));
    TEST_ASSERT_EQUAL(4, CAPWAP_GET_CATEGORY(CAPWAP_MSG_FIRMWARE_INFO_REQ));
    TEST_ASSERT_EQUAL(5, CAPWAP_GET_CATEGORY(CAPWAP_MSG_ALARM_REPORT));
}

void test_capwap_session_id(void)
{
    struct capwap_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    uint32_t lo = 0x12345678;
    uint32_t hi = 0xABCDEF00;

    CAPWAP_SET_SESSION(&hdr, lo, hi);

    uint64_t session = CAPWAP_GET_SESSION(&hdr);
    TEST_ASSERT_EQUAL(lo, (uint32_t)(session & 0xFFFFFFFF));
    TEST_ASSERT_EQUAL(hi, (uint32_t)(session >> 32));
}

void test_capwap_connection_init(void)
{
    capwap_conn_t conn;
    memset(&conn, 0, sizeof(conn));

    int ret = capwap_init(&conn);
    TEST_ASSERT(ret == 0, "Connection init failed");
    TEST_ASSERT(conn.state == CAPWAP_STATE_IDLE, "Initial state should be IDLE");
}

void test_capwap_message_types(void)
{
    const char *type_str;

    type_str = capwap_msg_type_to_string(CAPWAP_MSG_DISCOVERY_REQUEST);
    TEST_ASSERT_NOT_NULL((void*)type_str);

    type_str = capwap_msg_type_to_string(CAPWAP_MSG_JOIN_REQUEST);
    TEST_ASSERT_NOT_NULL((void*)type_str);

    type_str = capwap_msg_type_to_string(CAPWAP_MSG_CONFIG_RESPONSE);
    TEST_ASSERT_NOT_NULL((void*)type_str);
}

void test_capwap_state_string(void)
{
    const char *state_str;

    state_str = capwap_state_to_string(CAPWAP_STATE_IDLE);
    TEST_ASSERT_STRING_EQUAL("IDLE", state_str);

    state_str = capwap_state_to_string(CAPWAP_STATE_DISCOVERY);
    TEST_ASSERT_STRING_EQUAL("DISCOVERY", state_str);

    state_str = capwap_state_to_string(CAPWAP_STATE_RUN);
    TEST_ASSERT_STRING_EQUAL("RUN", state_str);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    test_suite_init("CAPWAP Protocol Tests", 20);

    ADD_TEST_CASE(test_capwap_header_create);
    ADD_TEST_CASE(test_capwap_state_transitions);
    ADD_TEST_CASE(test_capwap_msg_category);
    ADD_TEST_CASE(test_capwap_session_id);
    ADD_TEST_CASE(test_capwap_connection_init);
    ADD_TEST_CASE(test_capwap_message_types);
    ADD_TEST_CASE(test_capwap_state_string);

    test_suite_run();
    test_suite_summary();

    return g_test_suite.failed > 0 ? 1 : 0;
}
