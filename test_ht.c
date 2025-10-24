#include "unity/unity.h"

#include "ht.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void exit_nomem(void) {
    fprintf(stderr, "out of memory\n");
}

static void
test_ht_set_get(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    TEST_ASSERT_NOT_NULL(ht_set(tbl, "foo", "bar"));
    TEST_ASSERT_NOT_NULL(ht_set(tbl, "bar", "foo"));
    TEST_ASSERT_NOT_NULL(ht_set(tbl, "bazz", "bazz"));
    TEST_ASSERT_NOT_NULL(ht_set(tbl, "bob", "bob"));
    TEST_ASSERT_NOT_NULL(ht_set(tbl, "buzz", "buzz"));
    TEST_ASSERT_NOT_NULL(ht_set(tbl, "jane", "jane"));
    TEST_ASSERT_NOT_NULL(ht_set(tbl, "x", "x"));
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, tbl->length, "wrong num_elements");

    TEST_ASSERT_EQUAL_STRING("bar", ht_get(tbl, "foo"));
    TEST_ASSERT_EQUAL_STRING("foo", ht_get(tbl, "bar"));
    TEST_ASSERT_EQUAL_STRING("bazz", ht_get(tbl, "bazz"));
    TEST_ASSERT_EQUAL_STRING("bob", ht_get(tbl, "bob"));
    TEST_ASSERT_EQUAL_STRING("buzz", ht_get(tbl, "buzz"));
    TEST_ASSERT_EQUAL_STRING("jane", ht_get(tbl, "jane"));
    TEST_ASSERT_EQUAL_STRING("x", ht_get(tbl, "x"));
    TEST_ASSERT_EQUAL_INT_MESSAGE(7, tbl->length, "wrong num_elements");

    ht_destroy(tbl);
}

static void
test_ht_remove(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    ht_set(tbl, "foo", "foo");
    ht_set(tbl, "x", "x"); // collides with "foo" with FNV-1a
    ht_set(tbl, "bar", "bar");
    TEST_ASSERT_EQUAL_INT(3, tbl->length);

    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "foo"));
    TEST_ASSERT_EQUAL_INT(2, tbl->length);
    TEST_ASSERT_NULL(ht_remove(tbl, "foo"));
    TEST_ASSERT_EQUAL_INT(2, tbl->length);

    TEST_ASSERT_EQUAL_STRING("x", ht_get(tbl, "x"));

    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(1, tbl->length);
    TEST_ASSERT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(1, tbl->length);

    ht_set(tbl, "bar", "foo");
    TEST_ASSERT_EQUAL_INT(2, tbl->length);

    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(1, tbl->length);
    TEST_ASSERT_NULL(ht_remove(tbl, "bar"));
    TEST_ASSERT_EQUAL_INT(1, tbl->length);

    TEST_ASSERT_NOT_NULL(ht_remove(tbl, "x"));
    TEST_ASSERT_EQUAL_INT(0, tbl->length);

    ht_destroy(tbl);
}

static int
__str_array_idx(char* arr[], int len, const char* val) {
    for (int i = 0; i < len; i++) {
        if (!strcmp(arr[i], val)) return i;
    }
    return -1;
}

static void
test_ht_iterator(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    char *data[] = { "foo", "bar", "baz", "jane" };
    int data_len = sizeof(data) / sizeof(data[0]);
    for (int i = 0; i < data_len; i++) {
        ht_set(tbl, data[i], data[i]);
    }

    bool is_found[data_len];
    memset(is_found, 0, data_len);

    hti it = ht_iterator(tbl);
    while (ht_next(&it)) {
        int idx = __str_array_idx(data, data_len, it.current->key);
        if (idx == -1) continue;
        is_found[idx] = true;
    }

    bool all_found = true;
    for (int i = 0; i < data_len; i++) {
        if (!is_found[i]) {
            all_found = false;
            printf("could not find %s\n", data[i]);
        }
    }
    TEST_ASSERT(all_found);

    ht_destroy(tbl);
}

static void
test_ht_expand(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    // eh.
    const char *data[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13",
        "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25",
        "26", "27", "28", "29", "30", "31", "32", "33", "34", "35",
    };
    const int data_len = sizeof(data) / sizeof(data[0]);

    for (int i = 0; i < data_len; i++) {
        TEST_ASSERT_NOT_NULL(ht_set(tbl, data[i], "x"));
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(data_len, tbl->length, "wrong num_elements");

    ht_destroy(tbl);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ht_set_get);
    RUN_TEST(test_ht_remove);
    RUN_TEST(test_ht_iterator);
    RUN_TEST(test_ht_expand);
    return UNITY_END();
}
