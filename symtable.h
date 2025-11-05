/*
 * symtable.h
 * Defines symtable interface
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 * Tomáš Hanák (xhanakt00)
 * Šimon Halas (xhalass00)
 */
#ifndef _SYMTABLE_H_
#define _SYMTABLE_H_

#include "string.h"
#include <stdbool.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 16

typedef enum sym_type {
    SYM_GLOBAL_VAR,
    SYM_FUNCTION,
    SYM_GETTER,
    SYM_SETTER,
    SYM_VAR,
} SymType;

typedef struct symtable_item {
    String key;
    String name;
    SymType type;
} SymtableItem;

typedef struct symtable {
    SymtableItem *data; /* array of slots */
    int *state;        /* 0 = empty, 1 = occupied, 2 = deleted */
    size_t size;        /* number of stored items */
    size_t capacity;    /* slots count */
} Symtable;

/// Creates a new symtable
Symtable *symtable_init(void);

/// Frees a symtable
void symtable_free(Symtable *st);

/// Finds an item in the symtable with the key. Returns a pointer to the item or NULL if it is not in the symtable.
SymtableItem *symtable_find(Symtable *st, String *key);

/// Returns true if the symtable contains an item with the given key
bool symtable_contains(Symtable *st, String *key);

/// Inserts an item in the symtable with the key. Returns a pointer to the new item or NULL if it is not created.
SymtableItem *symtable_insert(Symtable *st, String *key);

// TODO: Removing? Probably not needed

#endif // !_SYMTABLE_H_
