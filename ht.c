#include "ht.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

uint64_t hash_fnv1a(const char *key) {
    uint64_t hash = FNV_OFFSET;
    for (const char *p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

static inline uint64_t
hash_index(uint64_t hash, size_t num_buckets) {
    return (hash & (uint64_t)(num_buckets - 1));
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
bucket_get_value(ht_bucket *bucket, uint64_t hash) {
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
bucket_set(ht *table, ht_bucket *bucket, const char *key, uint64_t hash,
        void *value) {
    for (size_t i = 0; i < N; i++) {
        // hashes[i] is not guaranteed to not be 0 (NULL)
        if (bucket->entries[i].key == NULL) {
            bucket->entries[i].key = key;
            bucket->entries[i].value = value;
            bucket->hashes[i] = hash;
            table->length++;
            return value;

        } else if (bucket->hashes[i] == hash) {
            void *old_val = bucket->entries[i].value;
            bucket->entries[i].value = value;
            return old_val;
        }
    }

    if (bucket->overflow != NULL)
        return bucket_set(table, bucket->overflow, key, hash, value);
    else {
        ht_bucket *new_bucket = calloc(1, sizeof(ht_bucket));
        if (new_bucket == NULL) return NULL;
        new_bucket->entries[0].key = key;
        new_bucket->entries[0].value = value;
        new_bucket->hashes[0] = hash;
        table->length++;
        bucket->overflow = new_bucket;
        return value;
    }
}

void *
ht_get(ht *table, const char *key) {
    if (key == NULL) return NULL;
    uint64_t hash = hash_fnv1a(key);
    size_t index = hash_index(hash, table->_buckets_length);
    return bucket_get_value(table->buckets + index, hash);
}

void *
ht_get_hash(ht *table, uint64_t hash) {
    size_t index = hash_index(hash, table->_buckets_length);
    return bucket_get_value(table->buckets + index, hash);
}

// create new bucket list with double number of current buckets and
// NOTE: immediately move all elements to new buckets.
// Returns false if no memory or could not move.
static bool
ht_expand(ht *table) {
    size_t new_length = table->_buckets_length * 2;
    if (new_length < table->_buckets_length) return false; // overflow

    ht_bucket *old_buckets = table->buckets,
              *new_buckets = calloc(new_length, sizeof(ht_bucket));
    if (new_buckets == NULL) {
        table->buckets = old_buckets;
        return false;
    }

    // [bucket_set] auto increments
    int num_elements = table->length;
    table->length = 0;

    // TODO: expand not all at once?

    size_t index;
    uint64_t hash;
    void *err;

    hti it = ht_iterator(table);
    while (ht_next(&it)) {
        hash = it._bucket->hashes[it._index - 1]; // see [ht_next]
        index = hash_index(hash, new_length);
        err = bucket_set(table, new_buckets + index,
                it.current->key, hash, it.current->value);
        if (err == NULL) {
            for (size_t i = 0; i < new_length; i++) {
                destroy_overflows(new_buckets + i);
            }
            free(new_buckets);
            table->buckets = old_buckets;
            table->length = num_elements;
            return false;
        }
    }

    free(old_buckets);
    table->buckets = new_buckets;
    table->_buckets_length = new_length;
    return true;
}

void *
ht_set_hash(ht *table, const char *key, void *value, uint64_t hash) {
    if (key == NULL || value == NULL) return NULL;

    // NOTE: This calculation does not take into account bucket overflows.
    size_t half_full = table->length >= (table->_buckets_length * N) / 2;
    if (half_full) {
        if (!ht_expand(table)) {
            return NULL;
        }
    }

    size_t index = hash_index(hash, table->_buckets_length);
    return bucket_set(table, table->buckets + index, key, hash, value);
}

void *
ht_set(ht *table, const char *key, void *value) {
    if (key == NULL || value == NULL) return NULL;

    uint64_t hash = hash_fnv1a(key);
    return ht_set_hash(table, key, value, hash);
}

void *
ht_remove_hash(ht *table, const char *key, uint64_t hash) {
    if (key == NULL) return NULL;
    size_t index = hash_index(hash, table->_buckets_length);
    ht_bucket *bucket = table->buckets + index;

    // search current bucket and its overflows.
    while (bucket != NULL) {
        for (int i = 0; i < N; i++) {
            if (bucket->entries[i].key == NULL) break;
            if (bucket->hashes[i] == hash) {
                // find last not NULL key
                int last = i + 1;
                for (; last < N - 1; last++) {
                    if (!bucket->entries[last + 1].key) {
                        break;
                    }
                }

                void *val = bucket->entries[i].value;
                bucket->entries[i] = bucket->entries[last];
                bucket->entries[last].key = NULL;
                bucket->hashes[i] = bucket->hashes[last];
                table->length--;
                return val;
            }
        }
        bucket = bucket->overflow;
    }

    return NULL;
}

void *
ht_remove(ht *table, const char *key) {
    if (key == NULL) return NULL;
    uint64_t hash = hash_fnv1a(key);
    return ht_remove_hash(table, key, hash);
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
    it._tbl = table;
    it._bucket = table->buckets;
    it._bucket_idx = 0;
    it._index = 0;
    return it;
}

static bool
__next_bucket(hti *it) {
    it->_index = 0;
    if (it->_bucket_idx >= it->_tbl->_buckets_length) {
        return false;
    } else {
        it->_bucket = &it->_tbl->buckets[it->_bucket_idx++];
        return true;
    }
}

bool
ht_next(hti *it) {
    while (1) {
        if (it->_index > N) {
            if (it->_bucket->overflow) {
                it->_bucket = it->_bucket->overflow;
                it->_index = 0;

            } else if (!__next_bucket(it)) {
                return false;
            }
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
