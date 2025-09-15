#include "ht.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

size_t hash_fnv1a(const char *key) {
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++) {
	hash ^= (uint64_t)(unsigned char)(*p);
	hash *= FNV_PRIME;
    }
    return hash;
}

static inline size_t
hash_index(size_t hash, size_t num_buckets) {
    return (size_t)(hash & (uint64_t)(num_buckets - 1));
}

ht *
ht_create(void) {
    ht *table = malloc(sizeof(ht));
    if (table == NULL) return NULL;
    table->length = 0;
    table->_buckets_length = N;
    table->buckets = calloc(N, sizeof(ht_bucket));
    if (table->buckets == NULL) {
	free(table);
	return NULL;
    }
    return table;
}

static void
destroy_overflows(ht_bucket *bucket) {
    if (bucket && bucket->overflow) {
	destroy_overflows(bucket->overflow);
	free(bucket->overflow);
    }
}

void
ht_destroy(ht *table) {
    for (size_t i = 0; i < table->_buckets_length; i++) {
	destroy_overflows(&table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}

// return [value] of [key] in [bucket] or its [overflows].
static void *
bucket_get_value(ht_bucket *bucket, size_t hash) {
    while (bucket != NULL) {
	for (size_t i = 0; i < N; i++) {
	    if (bucket->entries[i].key == NULL) return NULL;
	    if (bucket->hashes[i] == hash)
		return bucket->entries[i].value;
	}
	bucket = bucket->overflow;
    }
    return NULL;
}

static void *
bucket_set(ht *table, ht_bucket *bucket, const char *key, size_t hash,
	void *value) {
    for (size_t i = 0; i < N; i++) {
	if (bucket->entries[i].key == NULL) {
	    bucket->entries[i].key = key;
	    bucket->entries[i].value = value;
	    bucket->hashes[i] = hash;
	    table->length++;
	    return (void*)key;

	} else if (bucket->hashes[i] == hash) {
	    bucket->entries[i].value = value;
	    return (void*)key;
	}
    }

    if (bucket->overflow != NULL)
	return bucket_set(table, bucket->overflow, key, hash, value);
    else {
	ht_bucket *new_bucket = calloc(1, sizeof(ht_bucket));
	if (new_bucket == NULL) return NULL;
	new_bucket->entries[0].key = key;
	new_bucket->entries[0].value = value;
	table->length++;
	bucket->overflow = new_bucket;
	return (void*)key;
    }
}

void *
ht_get(ht *table, const char *key) {
    if (key == NULL) return NULL;
    size_t hash = hash_fnv1a(key);
    size_t index = hash_index(hash, table->_buckets_length);
    return bucket_get_value(table->buckets + index, hash);
}

// create new bucket list with double number of current buckets and
// NOTE: immediately move all elements to new bucket list.
// Returns false if no memory or could not move all elements.
static bool
ht_expand(ht *table) {
    size_t new_length = table->_buckets_length * 2;
    if (new_length < table->_buckets_length) return false; // overflow

    ht_bucket *old_buckets = table->buckets,
	      *new_buckets = calloc(new_length, sizeof(ht_bucket));
    if (new_buckets == NULL) return false;
    table->length = 0;

    // TODO: expand on a per bucket basis? not all at once?

    size_t index, hash;
    void *err;

    hti it = ht_iterator(table);
    while (ht_next(&it)) {
	hash = it._bucket->hashes[it._index - 1];
	index = hash_index(hash, new_length);
	err = bucket_set(table, new_buckets + index,
		it.current->key, hash, it.current->value);
	if (err == NULL) {
	    for (size_t i = 0; i < new_length; i++) {
		destroy_overflows(new_buckets + i);
	    }
	    free(new_buckets);
	    table->buckets = old_buckets;
	    return false;
	}
    }

    free(old_buckets);
    table->buckets = new_buckets;
    table->_buckets_length = new_length;
    return true;
}

void*
ht_set(ht *table, const char *key, void *value) {
    if (key == NULL) return NULL;

    size_t half_full = table->length >= (table->_buckets_length * N) / 2;
    if (half_full) {
	if (!ht_expand(table)) {
	    return NULL;
	}
    }

    size_t hash = hash_fnv1a(key);
    size_t index = hash_index(hash, table->_buckets_length);
    void *err = bucket_set(table, table->buckets + index, key, hash, value);
    if (err == NULL) return NULL;
    return (void*)key;
}

// returns index of last not-null key in [bucket]
int
bucket_last(ht_bucket *bucket) {
    for (int i = 0; i < N - 1; i++) {
	if (bucket->hashes[i]) {
	    return i + 1;
	}
    }
    return N - 1;
}

void *
ht_remove(ht *table, const char *key) {
    if (key == NULL) return NULL;
    size_t hash = hash_fnv1a(key);
    size_t index = hash_index(hash, table->_buckets_length);
    ht_bucket *bucket = table->buckets + index;

    while (bucket != NULL) {
	for (int i = 0; i < N; i++) {
	    if (bucket->entries[i].key == NULL) break;
	    if (bucket->hashes[i] == hash) {
		int last = bucket_last(bucket);
		bucket->entries[i] = bucket->entries[last];
		bucket->entries[last].key = NULL;
		bucket->hashes[i] = bucket->hashes[last];
		table->length--;
		return (void*)key;
	    }
	}
	bucket = bucket->overflow;
    }

    return NULL;
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
    // better than recursion?
    while (1) {
	if (it->_index > N) {
	    if (!__next_bucket(it))
		return false;
	    continue;
	}

	ht_entry *entry = &it->_bucket->entries[it->_index++];
	if (entry->key == NULL) {
	    if (!__next_bucket(it))
		return false;
	    continue;
	}

	it->current = entry;
	return true;
    }
}
