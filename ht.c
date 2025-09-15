#include "ht.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

size_t
hash_fnv1a(const char *key, size_t num_buckets) {
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++) {
	hash ^= (uint64_t)(unsigned char)(*p);
	hash *= FNV_PRIME;
    }
    return (size_t)(hash & (uint64_t)(num_buckets - 1));
}

bool
str_equal(const char *key1, const char *key2) {
    return strcmp(key1, key2) == 0;
}

ht *
ht_create_with(hash_fn* hash) {
    ht *table = malloc(sizeof(ht));
    if (table == NULL) return NULL;
    table->_hash = hash;
    table->length = 0;
    table->_buckets_length = N;
    table->buckets = calloc(N, sizeof(ht_bucket));
    if (table->buckets == NULL) {
	free(table);
	return NULL;
    }
    return table;
}

ht *
ht_create(void) {
    return ht_create_with(&hash_fnv1a);
}

static void
bucket_destroy(ht_bucket *bucket) {
    for (size_t j = 0; j < N; j++) {
	ht_entry *entry = &bucket->entries[j];
	if (entry->key == NULL) break;
	free((void *)entry->key);
    }
    if (bucket->overflow != NULL)
	bucket_destroy(bucket->overflow);
}

void
ht_destroy(ht *table) {
    for (size_t i = 0; i < table->_buckets_length; i++) {
	bucket_destroy(&table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}

// return [value] of [key] in [bucket] or its [overflows].
static void *
bucket_get_value(ht_bucket *bucket, const char *key) {
    while (bucket != NULL) {
	for (size_t i = 0; i < N; i++) {
	    if (bucket->entries[i].key == NULL)
		return NULL;
	    if (str_equal(key, bucket->entries[i].key))
		return bucket->entries[i].value;
	}
	bucket = bucket->overflow;
    }
    return NULL;
}

// return [ht_bucket] that contains of [key], setting [idx] to the index of
// [key] in bucket.
// returns NULL if not found.
static ht_bucket *
bucket_get(ht *table, const char *key, size_t *idx) {
    size_t bucket_idx = table->_hash(key, table->_buckets_length);
    ht_bucket *bucket = table->buckets + bucket_idx;
    while (bucket != NULL) {
	for (*idx = 0; *idx < N; (*idx)++) {
	    if (bucket->entries[*idx].key == NULL)
		return NULL;
	    if (str_equal(key, bucket->entries[*idx].key))
		return bucket;
	}
	bucket = bucket->overflow;
    }
    return NULL;
}

static void *
bucket_set(ht *table, ht_bucket *bucket, const char *key, void *value,
	bool copy_key) {
    for (size_t i = 0; i < N; i++) {
	if (bucket->entries[i].key == NULL) {
	    if (copy_key) {
		key = strdup(key);
		if (key == NULL) return NULL;
	    }
	    bucket->entries[i].key = key;
	    bucket->entries[i].value = value;
	    table->length++;
	    return value;

	} else if (str_equal(key, bucket->entries[i].key)) {
	    bucket->entries[i].value = value;
	    return value;
	}
    }

    if (bucket->overflow != NULL)
	return bucket_set(table, bucket->overflow, key, value, copy_key);
    else {
	ht_bucket *new_bucket = calloc(1, sizeof(ht_bucket));
	if (new_bucket == NULL) return NULL;
	new_bucket->entries[0].key = key;
	new_bucket->entries[0].value = value;
	table->length += 1;
	bucket->overflow = new_bucket;
	return value;
    }
}

// returns index of last not-null key in [bucket]
int
bucket_last(ht_bucket *bucket) {
    for (int i = N - 1; i > 0; i--) {
	if (bucket->entries[i].key != NULL) {
	    return i + 1;
	}
    }
    return 0;
}

void *
ht_get(ht *table, const char *key) {
    if (key == NULL) return NULL;
    size_t index = table->_hash(key, table->_buckets_length);
    return bucket_get_value(table->buckets + index, key);
}

static const char *
ht_set_entry(ht *table, const char *key, void *value, bool copy_key) {
    size_t index = table->_hash(key, table->_buckets_length);
    ht_bucket *bucket = table->buckets + index;
    bucket_set(table, bucket, key, value, copy_key);
    return key;
}

// create new bucket list with double number of current buckets and
// NOTE: immediately move all elements to new bucket list.
// Returns false if no memory or could not move all elements.
static bool
ht_expand(ht *table) {
    size_t new_length = table->length * 2;
    if (new_length < table->length) return false; // overflow

    ht_bucket *old_buckets = table->buckets,
	      *new_buckets = calloc(new_length, sizeof(ht_bucket));
    if (new_buckets == NULL) return false;
    table->buckets = new_buckets;

    // TODO: expand on a per bucket basis?
    for (size_t i = 0; i < table->length; i++) {
	ht_entry *p = table->buckets[i].entries;
	for (; p->key; p++) {
	    if (ht_set_entry(table, p->key, p->value, false) == NULL) {
		table->buckets = old_buckets;
		free(new_buckets);
		return false;
	    }
	}
    }

    free(old_buckets);
    return true;
}

const char*
ht_set(ht *table, const char *key, void *value) {
    if (key == NULL) return NULL;

    size_t half_full = table->length >= (table->_buckets_length * N) / 2;
    if (half_full) {
	if (!ht_expand(table)) {
	    return NULL;
	}
    }

    if (ht_set_entry(table, key, value, true) == NULL)
	return NULL;
    return key;
}

void *
ht_remove(ht *table, const char *key) {
    if (key == NULL) return NULL;
    size_t i;
    ht_bucket *bucket = bucket_get(table, key, &i);
    if (bucket == NULL) return NULL;

    void* value = bucket->entries[i].value;
    int last = bucket_last(bucket);
    bucket->entries[i] = bucket->entries[last];
    bucket->entries[last].key = NULL;
    table->length--;
    return value;
}

void
ht_print(ht *table) {
    for (size_t i = 0; i < table->_buckets_length; i++) {
	printf("bucket %zu\n", i);
	ht_bucket *bucket = &table->buckets[i];
	while (bucket != NULL) {
	    for (int i = 0; i < N; i++) {
		ht_entry *entry = &bucket->entries[i];
		if (entry->key == NULL) break;
		printf("> %s\n", entry->key);
	    }
	    bucket = bucket->overflow;
	}
	puts("");
    }
}

hti
ht_iterator(ht *table) {
    hti it;
    it._table = table;
    it._bucket = &table->buckets[0];
    it._bucket_idx = 1;
    it._index = 0;
    return it;
}

static bool
__next_bucket(hti *it) {
    it->_index = 0;
    if (it->_bucket_idx >= N) {
	return false;
    } else {
	it->_bucket = &it->_table->buckets[it->_bucket_idx++];
	return true;
    }
}

bool
ht_next(hti *it) {
    size_t bucket_len = it->_table->_buckets_length;
    // better than recursion?
    while (1) {
	ht_bucket *bucket = it->_bucket;
	ht_entry *entry = &bucket->entries[it->_index];
	if (entry->key == NULL) {
	    if (!__next_bucket(it))
		return false;
	    continue;
	}

	it->current = entry;
	it->_index++;
	if (it->_index >= bucket_len) {
	    if (!__next_bucket(it))
		return false;
	    continue;
	}
	return true;
    }
}
