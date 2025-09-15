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
	ht_set(tbl, "bazz", "bazz");
	ht_set(tbl, "bob", "bob");
	ht_set(tbl, "buzz", "buzz");
	ht_set(tbl, "jane", "jane");
	ht_set(tbl, "x", "x");

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

	ht_set(tbl, "foo", "bar");
	ht_set(tbl, "bar", "foo");
	TEST_ASSERT_EQUAL_INT(2, tbl->length);

	TEST_ASSERT_NOT_NULL(ht_remove(tbl, "foo"));
	TEST_ASSERT_NULL(ht_remove(tbl, "foo"));
	TEST_ASSERT_EQUAL_INT(1, tbl->length);

	TEST_ASSERT_NOT_NULL(ht_remove(tbl, "bar"));
	TEST_ASSERT_NULL(ht_remove(tbl, "bar"));
	TEST_ASSERT_EQUAL_INT(0, tbl->length);

	ht_set(tbl, "bar", "foo");
	TEST_ASSERT_NOT_NULL(ht_remove(tbl, "bar"));
	TEST_ASSERT_NULL(ht_remove(tbl, "bar"));
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

	bool is_found[data_len] = {};
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
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_ht_set_get);
	RUN_TEST(test_ht_remove);
	RUN_TEST(test_ht_iterator);
	return UNITY_END();
}
