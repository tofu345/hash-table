#include "unity/unity.h"
#include "ht.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void exit_nomem(void) {
    fprintf(stderr, "out of memory\n");
}

bool ht_key_is(ht* tbl, const char* key, void* expected_value) {
    char* val = ht_get(tbl, key);
    if (val == NULL) return false;
    return val == expected_value;
}

bool ht_key_is_str(ht* tbl, const char* key, const char* expected_value) {
    char* val = ht_get(tbl, key);
    if (val == NULL) return false;
    return strcmp(val, expected_value) == 0;
}

static void
test_ht_set_get(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    ht_set(tbl, "foo", "bar");
    ht_set(tbl, "bar", "foo");
    int num = 1;
    ht_set(tbl, "num", &num);

    TEST_ASSERT_EQUAL_STRING("bar", ht_get(tbl, "foo"));
    TEST_ASSERT_EQUAL_STRING("foo", ht_get(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(1, *(int*)ht_get(tbl, "num"));

    ht_destroy(tbl);
}

static void
test_ht_remove(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    ht_set(tbl, "foo", "bar");
    ht_set(tbl, "bar", "foo");
    TEST_ASSERT_EQUAL_INT(2, ht_length(tbl));

    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "foo"));
    TEST_ASSERT_NULL(ht_remove(tbl, "foo"));
    TEST_ASSERT_EQUAL_INT(1, ht_length(tbl));

    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(0, ht_length(tbl));

    ht_set(tbl, "bar", "foo");
    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(0, ht_length(tbl));

    ht_destroy(tbl);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ht_set_get);
    RUN_TEST(test_ht_remove);
    return UNITY_END();
}
