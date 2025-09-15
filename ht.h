#pragma once

#include <stddef.h>
#include <stdbool.h>

#define N 8

// [key] NULL if empty
typedef struct {
    const char *key;
    void *value;
} ht_entry;

// https://dave.cheney.net/2018/05/29/how-the-go-runtime-implements-maps-efficiently-without-generics
typedef struct ht_bucket {
    ht_entry entries[N]; // NULL-terminated
    struct ht_bucket *overflow;
} ht_bucket;

// returns index of bucket to store key in
typedef size_t hash_fn(const char *key, size_t num_buckets);

typedef struct {
    size_t length; // number of filled entries

    ht_bucket *buckets;
    size_t _buckets_length;

    hash_fn *_hash;
} ht;

// from: https://benhoyt.com/writings/hash-table-in-c/
// hash with FNV-1a algorithm. NOTE: its not secure, but it works.
//
// Returns 64-bit FNV-1a hash for key (NUL-terminated).
size_t hash_fnv1a(const char *key, size_t num_buckets);

// hash table with fnv1a hash function. returns NULL on err
ht *ht_create(void);

// hash table with custom hash function. returns NULL on err
ht *ht_create_with(hash_fn* hash);

// Free memory allocated for hash table and copied keys, NOTE: not values.
void ht_destroy(ht *table);

// Get item with given key from hash table.
// Return value, or NULL if key not found.
void *ht_get(ht *table, const char *key);

// Set item with given key (which must not be NULL) to value. If not already
// present in table, the key is copied and freed with [ht_destroy].
//
// Returns address of [key] on success.
// Returns NULL if [key] is NULL or out of memory.
const char *ht_set(ht *table, const char *key, void *value);

// Remove item with given key and if exists return pointer to its value or NULL
// if not found.
void *ht_remove(ht *table, const char *key);

void ht_print(ht *table);

typedef struct {
    ht_entry *current;

    // Don't use these fields directly.
    ht *_table;
    ht_bucket *_bucket;
    size_t _bucket_idx; // index into `_table.buckets`
    size_t _index;      // current index into `_bucket.entries`
} hti;

// Return new hash table iterator (for use with ht_next).
hti ht_iterator(ht *table);

// Move iterator to next item in hash table, update iterator's current
// [ht_entry] to current item, and return true. If there are no more items,
// return false. Do not mutate the table during iteration.
bool ht_next(hti *it);
