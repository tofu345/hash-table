#include "unity/unity.h"

#include "ht.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// print hash-table
__attribute__ ((unused))
static void ht_print(ht *table) {
    for (size_t i = 0; i < table->_buckets_length; i++) {
        printf("bucket %zu\n", i);
        ht_bucket *bucket = &table->buckets[i];
        while (bucket != NULL) {
            for (int i = 0; i < N; i++) {
                ht_entry *entry = &bucket->entries[i];
                if (!bucket->filled[i]) {
                    break;
                }
                printf("> %ld\n", (long) entry->key);
            }
            bucket = bucket->overflow;
        }
        puts("");
    }
}


ht *tbl;

void setUp(void) {
    tbl = ht_create();
    if (tbl == NULL) {
        fprintf(stderr, "out of memory\n");
        TEST_IGNORE();
    }
}

void tearDown(void) {
    ht_destroy(tbl);
}

void *res;

// check if ht_set(key, val) == val
#define ASSERT_SET(key, val) \
    res = ht_set(tbl, key, val); \
    TEST_ASSERT(errno != ENOMEM); \
    TEST_ASSERT_EQUAL_STRING(res, val);

// check if ht_set_hash(key, val, hash) == val
#define ASSERT_SET_HASH(key, val, hash) \
    res = ht_set_hash(tbl, key, val, hash); \
    TEST_ASSERT_EQUAL_PTR(res, val); \
    TEST_ASSERT(errno != ENOMEM);

// check if ht_get_hash(key, val) == val
#define ASSERT_GET(key, val) \
    res = ht_get(tbl, key); \
    TEST_ASSERT_EQUAL_STRING(res, val); \
    TEST_ASSERT(errno != ENOKEY);

// check if ht_get_hash(hash, val) == val
#define ASSERT_GET_HASH(hash, val) \
    res = ht_get_hash(tbl, hash); \
    TEST_ASSERT_EQUAL_PTR(res, val); \
    TEST_ASSERT(errno != ENOKEY);

static void
test_ht_set_get(void) {
    ASSERT_SET("foo", "bar");
    ASSERT_SET("bar", "foo");
    ASSERT_SET("bazz", "bazz");
    ASSERT_SET("bob", "bob");
    ASSERT_SET("buzz", "buzz");
    ASSERT_SET("jane", "jane");
    ASSERT_SET("x", "x");

    size_t i, num = 7;
    for (i = 0; i < num; ++i) {
        ASSERT_SET_HASH((void *) i, (void *) i, i);
    }

    int expected_len = 7 + num;
    TEST_ASSERT_EQUAL_INT(expected_len, tbl->length);

    ASSERT_GET("foo", "bar");
    ASSERT_GET("bar", "foo");
    ASSERT_GET("bazz", "bazz");
    ASSERT_GET("bob", "bob");
    ASSERT_GET("buzz", "buzz");
    ASSERT_GET("jane", "jane");
    ASSERT_GET("x", "x");

    for (i = 0; i < 5; ++i) {
        ASSERT_GET_HASH(i, i);
    }

    TEST_ASSERT_EQUAL_INT(expected_len, tbl->length);
}

static void
test_ht_remove(void) {
    ASSERT_SET("foo", "foo");
    ASSERT_SET("x", "x");   // collides with "foo" with FNV-a, will be in same bucket.
    ASSERT_SET("bar", "bar");
    TEST_ASSERT_EQUAL_INT(3, tbl->length);

    res = ht_remove(tbl, "foo");
    TEST_ASSERT(errno != ENOKEY && strcmp(res, "foo") == 0);
    TEST_ASSERT_EQUAL_INT(2, tbl->length);

    ht_remove(tbl, "foo");
    TEST_ASSERT(errno == ENOKEY);
    TEST_ASSERT_EQUAL_INT(2, tbl->length);

    ht_get(tbl, "foo");
    TEST_ASSERT(errno == ENOKEY);
    TEST_ASSERT_EQUAL_INT(2, tbl->length);

    TEST_ASSERT_EQUAL_STRING("bar", ht_get(tbl, "bar"));
    TEST_ASSERT_EQUAL_STRING("x", ht_get(tbl, "x"));

    res = ht_remove(tbl, "bar");
    TEST_ASSERT(errno != ENOKEY && strcmp(res, "bar") == 0);
    TEST_ASSERT_EQUAL_INT(1, tbl->length);

    ht_remove(tbl, "bar");
    TEST_ASSERT(errno == ENOKEY);
    TEST_ASSERT_EQUAL_INT(1, tbl->length);

    TEST_ASSERT_EQUAL_STRING("x", ht_get(tbl, "x"));

    ASSERT_SET("bar", "foo");
    TEST_ASSERT_EQUAL_INT(2, tbl->length);

    res = ht_remove(tbl, "bar");
    TEST_ASSERT(errno != ENOKEY && strcmp(res, "foo") == 0);
    TEST_ASSERT_EQUAL_INT(1, tbl->length);

    ht_remove(tbl, "bar");
    TEST_ASSERT(errno == ENOKEY);
    TEST_ASSERT_EQUAL_INT(1, tbl->length);

    TEST_ASSERT_EQUAL_STRING("x", ht_get(tbl, "x"));

    res = ht_remove(tbl, "x");
    TEST_ASSERT(errno != ENOKEY && strcmp(res, "x") == 0);
    TEST_ASSERT_EQUAL_INT(0, tbl->length);
}

static void
test_ht_iterator(void) {
    char *data[] = { "foo", "bar", "baz", "jane" };
    int data_len = sizeof(data) / sizeof(data[0]);
    for (int i = 0; i < data_len; i++) {
        ASSERT_SET_HASH(data[i], (void *)(size_t) i, hash_fnv1a(data[i]));
    }

    bool is_found[data_len];
    memset(is_found, 0, data_len);

    hti it = ht_iterator(tbl);
    while (ht_next(&it)) {
        is_found[(long) it.current->value] = true;
    }

    bool all_found = true;
    for (int i = 0; i < data_len; i++) {
        if (!is_found[i]) {
            all_found = false;
            printf("could not find %s\n", data[i]);
        }
    }
    TEST_ASSERT(all_found);
}

static void
test_ht_expand(void) {
    size_t num = 150; // with N starting at 8, should cause ht_expand() 3 times

    size_t i;
    for (i = 0; i < num; ++i) {
        ASSERT_SET_HASH((void *) i, (void *) i, i);
    }

    TEST_ASSERT_EQUAL_INT(num, tbl->length);

    bool is_found[num];
    memset(is_found, 0, num);

    hti it = ht_iterator(tbl);
    size_t idx;
    while (ht_next(&it)) {
        idx = (size_t) it.current->value;
        is_found[idx] = true;
    }

    bool all_found = true;
    for (i = 0; i < num; i++) {
        if (!is_found[i]) {
            all_found = false;
            printf("could not find %zu\n", i);
        }
    }
    TEST_ASSERT(all_found);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ht_set_get);
    RUN_TEST(test_ht_remove);
    RUN_TEST(test_ht_iterator);
    RUN_TEST(test_ht_expand);
    return UNITY_END();
}
