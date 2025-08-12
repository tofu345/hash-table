#include "ht.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    const char* key; // key is NULL if empty
    void* value;
} ht_entry;

struct ht {
    ht_entry* entries;
    size_t capacity;
    size_t length;
};

#define INITIAL_CAPACITY 16

ht* ht_create(void) {
    ht* table = malloc(sizeof(ht));
    if (table == NULL) {
        return NULL;
    }
    table->length = 0;
    table->capacity = INITIAL_CAPACITY;

    table->entries = calloc(table->capacity, sizeof(ht_entry));
    if (table->entries == NULL) {
        free(table); // error, free table before return
        return NULL;
    }
    return table;
}

void ht_destroy(ht* table) {
    for (size_t i = 0; i < table->capacity; i++) {
        free((void*)table->entries[i].key);
    }

    free(table->entries);
    free(table);
}

// FNV-1a hash algorithm. FNV is not a randomized or cryptographic hash
// function, so it’s possible for an attacker to create keys with a lot
// of collisions and cause lookups to slow way down.
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint64_t _hash_key(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

static size_t hash_index(const char* key, int capacity) {
    uint64_t hash = _hash_key(key);
    return (size_t)(hash & (uint64_t)(capacity - 1));
}

void* ht_get(ht* table, const char* key) {
    size_t index = hash_index(key, table->capacity);

    // loop till we find empty entry
    while (table->entries[index].key != NULL) {
        if (strcmp(key, table->entries[index].key) == 0) {
            return table->entries[index].value;
        }
        // Key wasn't in this slot, move to next (linear probing).
        index++;
        if (index >= table->capacity) {
            index = 0;
        }
    }
    return NULL;
}

static const char*
ht_set_entry(ht_entry* entries, size_t capacity, const char* key,
            void* value, size_t* plength) {
    size_t index = hash_index(key, capacity);

    while (entries[index].key != NULL) {
        if (strcmp(key, entries[index].key) == 0) {
            entries[index].value = value;
            return entries[index].key;
        }
        // linear probing.
        index++;
        if (index >= capacity) {
            index = 0;
        }
    }

    if (plength != NULL) {
        key = strdup(key);
        if (key == NULL) {
            return NULL;
        }
        (*plength)++;
    }
    entries[index].key = (char*)key;
    entries[index].value = value;
    return key;
}

static bool ht_expand(ht* table) {
    // allocate new entries array
    size_t new_capacity = table->capacity * 2;
    if (new_capacity < table->capacity) {
        return false; // overflow
    }
    ht_entry* new_entries = calloc(new_capacity, sizeof(ht_entry));
    if (new_entries == NULL) {
        return false;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        ht_entry entry = table->entries[i];
        if (entry.key != NULL) {
            ht_set_entry(new_entries, new_capacity, entry.key,
                    entry.value, NULL);
        }
    }

    free(table->entries);
    table->entries = new_entries;
    table->capacity = new_capacity;
    return true;
}

const char* ht_set(ht* table, const char* key, void* value) {
    assert(value != NULL);
    if (value == NULL) {
        return NULL;
    }

    if (table->length >= table->capacity / 2) {
        if (!ht_expand(table)) {
            return NULL;
        }
    }

    return ht_set_entry(table->entries, table->capacity, key, value,
                &table->length);
}

void* ht_remove(ht* table, const char* key) {
    ht_entry* entries = table->entries;
    size_t cap = table->capacity;

    size_t i = hash_index(key, cap);
    while (entries[i].key != NULL) {
        if (strcmp(key, entries[i].key) == 0) {
            entries[i].key = NULL;
            table->length--;
            // this could case a use after free but value cannot be NULL
            //
            // side note: is this how use after frees happen?
            void* val = entries[i].value;

            // move entry in last collision to index
            size_t j = i;
            while (true) {
                const char* k = entries[j + 1].key;
                if (k == NULL || i != hash_index(k, cap)) break;
                j++;
                if (j >= cap) j = 0;
            }
            if (j == i) return val;

            entries[i].key = entries[j].key;
            entries[i].value = entries[j].value;
            entries[j].key = NULL;
            return val;
        }
        // linear probing.
        i++;
        if (i >= cap) i = 0;
    }

    return NULL;
}

size_t ht_length(ht* table) {
    return table->length;
}

hti ht_iterator(ht* table) {
    hti it;
    it._table = table;
    it._index = 0;
    return it;
}

bool ht_next(hti *it) {
    // loop till end of entries array
    ht* table = it->_table;
    while (it->_index < table->capacity) {
        size_t i = it->_index;
        it->_index++;
        if (table->entries[i].key != NULL) {
            // Found next non-empty item, update iterator key and value.
            ht_entry entry = table->entries[i];
            it->key = entry.key;
            it->value = entry.value;
            return true;
        }
    }
    return false;
}
