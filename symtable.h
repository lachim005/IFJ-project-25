/*
 * symtable.h
 * Defines symtable interface
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 * Tomáš Hanák (xhanakt00)
 */
#ifndef _SYMTABLE_H_
#define _SYMTABLE_H_

#include "string.h"
#include <stdbool.h>

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
    // ...
} SymtableItem;

typedef struct symtable {
    // TODO
} Symtable;

/// Creates a new symtable
Symtable *symtable_init();

/// Frees a symtable
void symtable_free(Symtable *st);

/// Finds an item in the symtable with the key. Returns a pointer to the item or NULL if it is not in the symtable.
SymtableItem *symtable_find(SymtableItem *st, String *key);

/// Returns true if the symtable contains an item with the given key
bool symtable_contains(SymtableItem *st, String *key);

/// Inserts an item in the symtable with the key. Returns a pointer to the new item or NULL if it is not created.
SymtableItem *symtable_insert(SymtableItem *st, String *key);

// TODO: Removing? Probably not needed

#endif // !_SYMTABLE_H_
