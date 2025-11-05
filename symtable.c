/*
 * symtable.c
 * Defines symtable interface
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 * Tomáš Hanák (xhanakt00)
 * Šimon Halas (xhalass00)
 */

#include "symtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

enum { SLOT_EMPTY = 0, SLOT_OCCUPIED = 1, SLOT_DELETED = 2 };

// Simple djb2 hash function for strings
static unsigned long hash_str(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++) != 0) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Allocate and initialize state array
static int *state_alloc(size_t count) {
    int *s = malloc(sizeof(int) * count);
    if (!s) return NULL;
    for (size_t i = 0; i < count; ++i) s[i] = SLOT_EMPTY;
    return s;
}

Symtable *symtable_init(void) {
    Symtable *st = malloc(sizeof(Symtable));
    if (!st) return NULL;

    st->capacity = INITIAL_CAPACITY;
    st->size = 0;

    st->data = malloc(sizeof(SymtableItem) * st->capacity);
    if (!st->data) {
        free(st);
        return NULL;
    }
    st->state = state_alloc(st->capacity);
    if (!st->state) {
        free(st->data);
        free(st);
        return NULL;
    }
    return st;
}

void symtable_free(Symtable *st) {
    if (!st) return;
    free(st->data);
    free(st->state);
    free(st);
}

// Rehash symtable to new capacity
static bool symtable_rehash(Symtable *st, size_t new_capacity) {
    SymtableItem *old_data = st->data;
    int *old_state = st->state;
    size_t old_cap = st->capacity;

    SymtableItem *new_data = malloc(sizeof(SymtableItem) * new_capacity);
    if (!new_data) return false;
    int *new_state = state_alloc(new_capacity);
    if (!new_state) {
        free(new_data);
        return false;
    }

    // Move occupied items to new table
    for (size_t i = 0; i < new_capacity; ++i) new_state[i] = SLOT_EMPTY;
    for (size_t i = 0; i < old_cap; ++i) {
        if (old_state[i] == SLOT_OCCUPIED) {
            unsigned long h = hash_str(old_data[i].key.val);
            size_t probe = (size_t)(h % new_capacity);
            while (new_state[probe] == SLOT_OCCUPIED) {
                probe = (probe + 1) % new_capacity;
            }
            new_data[probe] = old_data[i];
            new_state[probe] = SLOT_OCCUPIED;
        }
    }

    free(old_data);
    free(old_state);

    st->data = new_data;
    st->state = new_state;
    st->capacity = new_capacity;
    return true;
}

SymtableItem *symtable_find(Symtable *st, String *key) {
    if (!st || !key) return NULL;
    if (st->capacity == 0) return NULL;

    unsigned long h = hash_str(key->val);
    size_t cap = st->capacity;
    size_t probe = (size_t)(h % cap);

    for (size_t i = 0; i < cap; ++i) {
        size_t idx = (probe + i) % cap;
        int s = st->state[idx];
        if (s == SLOT_EMPTY) return NULL; // not found, and no further possible
        if (s == SLOT_OCCUPIED && strcmp(st->data[idx].key.val, key->val) == 0) {
            return &st->data[idx];
        }
        // continue on deleted or occupied-but-not-equal
    }
    return NULL;
}

bool symtable_contains(Symtable *st, String *key) {
    return symtable_find(st, key) != NULL;
}

SymtableItem *symtable_insert(Symtable *st, String *key) {
    if (!st || !key) return NULL;

    // Prevent duplicates
    if (symtable_contains(st, key)) return NULL;

    // Grow when load factor > 0.7
    if ((st->size + 1) * 10 >= st->capacity * 7) {
        size_t newcap = st->capacity * 2;
        if (!symtable_rehash(st, newcap)) return NULL;
    }

    unsigned long h = hash_str(key->val);
    size_t cap = st->capacity;
    size_t probe = (size_t)(h % cap);
    ssize_t first_deleted = -1;

    for (size_t i = 0; i < cap; ++i) {
        size_t idx = (probe + i) % cap;
        int s = st->state[idx];
        if (s == SLOT_OCCUPIED) {
            if (strcmp(st->data[idx].key.val, key->val) == 0) {
                return NULL; // Should not happen due to earlier check, checks duplicates
            }
            continue;
        }
        if (s == SLOT_DELETED) {
            if (first_deleted == -1) first_deleted = (ssize_t)idx;
            continue;
        }
        // s == SLOT_EMPTY -> found insertion point
        size_t use_idx = (first_deleted != -1) ? (size_t)first_deleted : idx;
        st->data[use_idx].key = *key;
        st->data[use_idx].name.val = NULL;
        st->data[use_idx].type = SYM_VAR;
        st->state[use_idx] = SLOT_OCCUPIED;
        st->size++;
        return &st->data[use_idx];
    }

    // Table full, should not happen due to rehashing
    return NULL;
}