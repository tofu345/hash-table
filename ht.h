#pragma once

// Generic Hash-table
//
// Initially according to https://benhoyt.com/writings/hash-table-in-c/. Then
// later https://dave.cheney.net/2018/05/29/how-the-go-runtime-implements-maps-efficiently-without-generics
// to imitate the golang map implentation.

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Number of elements in a bucket and initial number of buckets.
#define N 8

typedef struct {
    void *key;
    void *value;
} ht_entry;

typedef struct ht_bucket {
    bool filled[N]; // false-terminated list of filled entries
    uint64_t hashes[N]; // for faster comparisons.
    ht_entry entries[N];
    struct ht_bucket *overflow;
} ht_bucket;

typedef struct {
    size_t length; // number of filled entries

    ht_bucket *buckets;
    size_t _buckets_length;
} ht;

// Returns 64-bit FNV-1a hash for [key] (NUL-terminated).
uint64_t hash_fnv1a(const char *key);

// Returns 64-bit FNV-1a hash for [key] of [len].
uint64_t hash_fnv1a_(const char *key, int len);

// Create new hash-table, returns NULL if out of memory.
ht *ht_create(void);

// Free hash-table.
void ht_destroy(ht *table);

// Hash [key] with hash_fnv1a() and return value with [key], or set `errno` to
// ENOKEY if not found.
void *ht_get(ht *table, const char *key);

// Return value with [hash], or or set `errno` to ENOKEY if not found.
void *ht_get_hash(ht *table, uint64_t hash);

// Compute hash of [key] with hash_fnv1a() and store [value].
//
// If [key] had a previous value return it, otherwise return [value].
//
// Return NULL and set `errno` to ENOMEM if out of memory.
void *ht_set(ht *table, const char *key, void *value);

// ht_set() with precomputed hash.
void *ht_set_hash(ht *table, void *key, void *value, uint64_t hash);

// Remove item with [key] and return its value or set `errno` to ENOKEY if not
// found.
void *ht_remove(ht *table, const char *key);

// ht_remove() with precomputed hash.
void *ht_remove_hash(ht *table, uint64_t hash);

typedef struct {
    ht_entry *current;

    // Don't use these fields directly.
    ht *_tbl;
    ht_bucket *_bucket; // current bucket under inspection
    size_t _bucket_idx; // index of current bucket into `_buckets`
    size_t _index;      // index of current entry into `_bucket.entries`
} hti;

// Return new hash table iterator (for use with ht_next).
hti ht_iterator(ht *table);

// Move iterator to next item in hash table, update iterator's current
// [ht_entry] to current item, and return true.
//
// If there are no more items, return false. Do not mutate the table during
// iteration.
bool ht_next(hti *it);
