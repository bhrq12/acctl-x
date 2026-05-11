/*
 * ============================================================================
 *
 *       Filename:  test_json.c
 *
 *    Description:  JSON database unit tests for acctl-ac
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
#include "../../src/include/json/mjson.h"
#include "acctl_ac_test.h"

test_suite_t g_test_suite;

void test_json_parse_basic(void)
{
    const char *json_str = "{\"name\":\"test\",\"value\":123}";
    struct json_object *obj = json_parse_string(json_str);

    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT(json_object_get_type(obj) == JSON_TYPE_OBJECT);

    json_object_put(obj);
}

void test_json_parse_array(void)
{
    const char *json_str = "{\"items\":[1,2,3]}";
    struct json_object *obj = json_parse_string(json_str);

    TEST_ASSERT_NOT_NULL(obj);

    struct json_object *arr;
    TEST_ASSERT(json_object_object_get(obj, "items", &arr));
    TEST_ASSERT(json_object_get_type(arr) == JSON_TYPE_ARRAY);
    TEST_ASSERT(json_object_array_length(arr) == 3);

    json_object_put(obj);
}

void test_json_get_string(void)
{
    const char *json_str = "{\"name\":\"OpenWrt-AC\"}";
    struct json_object *obj = json_parse_string(json_str);

    TEST_ASSERT_NOT_NULL(obj);

    struct json_object *name_obj;
    TEST_ASSERT(json_object_object_get(obj, "name", &name_obj));
    TEST_ASSERT(json_object_get_type(name_obj) == JSON_TYPE_STRING);

    const char *value = json_object_get_string(name_obj);
    TEST_ASSERT_STRING_EQUAL("OpenWrt-AC", value);

    json_object_put(obj);
}

void test_json_get_int(void)
{
    const char *json_str = "{\"port\":8080}";
    struct json_object *obj = json_parse_string(json_str);

    TEST_ASSERT_NOT_NULL(obj);

    struct json_object *port_obj;
    TEST_ASSERT(json_object_object_get(obj, "port", &port_obj));
    TEST_ASSERT(json_object_get_type(port_obj) == JSON_TYPE_INT);

    int value = json_object_get_int(port_obj);
    TEST_ASSERT_EQUAL(8080, value);

    json_object_put(obj);
}

void test_json_create_object(void)
{
    struct json_object *obj = json_object_new_object();
    TEST_ASSERT_NOT_NULL(obj);

    json_object_object_add_string(obj, "name", "test-ac");
    json_object_object_add_int(obj, "port", 8080);

    struct json_object *name_obj;
    TEST_ASSERT(json_object_object_get(obj, "name", &name_obj));
    const char *name = json_object_get_string(name_obj);
    TEST_ASSERT_STRING_EQUAL("test-ac", name);

    json_object_put(obj);
}

void test_json_create_array(void)
{
    struct json_object *arr = json_object_new_array();
    TEST_ASSERT_NOT_NULL(arr);

    json_object_array_add(arr, json_object_new_int(1));
    json_object_array_add(arr, json_object_new_int(2));
    json_object_array_add(arr, json_object_new_int(3));

    TEST_ASSERT_EQUAL(3, json_object_array_length(arr));

    json_object_put(arr);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    test_suite_init("JSON Database Tests", 20);

    ADD_TEST_CASE(test_json_parse_basic);
    ADD_TEST_CASE(test_json_parse_array);
    ADD_TEST_CASE(test_json_get_string);
    ADD_TEST_CASE(test_json_get_int);
    ADD_TEST_CASE(test_json_create_object);
    ADD_TEST_CASE(test_json_create_array);

    test_suite_run();
    test_suite_summary();

    return g_test_suite.failed > 0 ? 1 : 0;
}
