#include "ht.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void test_ht_set_get(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    ht_set(tbl, "foo", "bar");
    ht_set(tbl, "bar", "foo");
    int num = 1;
    ht_set(tbl, "num", &num);
    assert(ht_key_is_str(tbl, "foo", "bar"));
    assert(ht_key_is_str(tbl, "bar", "foo"));
    assert(ht_key_is(tbl, "num", &num));

    ht_destroy(tbl);
}

void test_ht_remove(void) {
    ht* tbl = ht_create();
    if (tbl == NULL) exit_nomem();

    ht_set(tbl, "foo", "bar");
    ht_set(tbl, "bar", "foo");
    assert(ht_length(tbl) == 2);
    assert(ht_remove(tbl, "foo") != NULL);
    assert(ht_remove(tbl, "foo") == NULL);
    assert(ht_length(tbl) == 1);
    assert(ht_remove(tbl, "bar") != NULL);
    assert(ht_remove(tbl, "bar") == NULL);
    assert(ht_length(tbl) == 0);

    ht_set(tbl, "bar", "foo");
    assert(ht_remove(tbl, "bar") != NULL);
    assert(ht_remove(tbl, "bar") == NULL);
    assert(ht_length(tbl) == 0);

    ht_destroy(tbl);
}

int main(void) {
    // good idea?
    void (*tests[]) (void) = {
        test_ht_set_get,
        test_ht_remove,
    };
    size_t len = sizeof(tests) / sizeof(tests[0]);
    for (int i = 0; i < len; i++) {
        tests[i]();
    }
    printf("all tests passed\n");
}
