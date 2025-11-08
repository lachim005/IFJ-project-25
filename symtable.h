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
#include "ast.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 16
#define ST_SCOPE_STACK_INITIAL_CAPACITY 16

typedef enum sym_type {
    SYM_GLOBAL_VAR,
    SYM_FUNCTION,
    SYM_GETTER,
    SYM_SETTER,
    SYM_VAR,
} SymType;

typedef struct symtable_item {
    char *key;
    String *name;
    SymType type;
    size_t param_count;
    bool is_defined;
    DataType data_type;
} SymtableItem;

typedef struct symtable {
    SymtableItem *data; /* array of slots */
    int *state;        /* 0 = empty, 1 = occupied, 2 = deleted */
    size_t size;        /* number of stored items */
    size_t capacity;    /* slots count */
    // Scope stack
    int *scope_stack;
    size_t scope_stack_count;
    size_t scope_stack_capacity;
    // Undefined items counter
    size_t undefined_items_counter;
} Symtable;

/// Creates a new symtable
Symtable *symtable_init(void);

/// Frees a symtable
void symtable_free(Symtable *st);

/// Finds an item in the symtable with the key. Returns a pointer to the item or NULL if it is not in the symtable.
SymtableItem *symtable_find(Symtable *st, const char *key);

/// Returns true if the symtable contains an item with the given key
bool symtable_contains(Symtable *st, const char *key);

/// Inserts an item in the symtable with the key. Returns a pointer to the new item or NULL if it is not created.
SymtableItem *symtable_insert(Symtable *st, const char *key);

/// Pushes a new scope to the scope stack. Returns false if the stack outgrows the capacity and realloc fails
bool enter_scope(Symtable *st);

/// Pop a scope from the scope stack
void exit_scope(Symtable *st);

/// Returns the id of the current scope
int current_scope(Symtable *st);

/// Returns false if some internal error happens, true if the search is successful. In out_item, there is the found item or NULL if it is not in the symtable
bool find_local_var(Symtable *st, char *var_name, SymtableItem **out_item);

/// Adds a new entry to the symtable for the var in the current scope. Returns the new entry or NULL if something fails
SymtableItem *add_var_at_current_scope(Symtable *st, char *var_name, DataType data_type);

/// Checks if the variable is declared in the current scope
bool contains_var_at_current_scope(Symtable *st, char *var_name);

/// Adds a new entry to the symtable for the getter of the var. Returns the new entry or NULL if something fails
SymtableItem *symtable_add_getter(Symtable *st, char *var_name, bool is_defined);

/// Adds a new entry to the symtable for the setter of the var. Returns the new entry or NULL if something fails
SymtableItem *symtable_add_setter(Symtable *st, char *var_name, bool is_defined);

/// Appends the scope id to the string. Returns false if some error happens
bool append_scope_id(String *s, int scope);

/// Adds a new entry to the symtable for the function with param_count parameters. Returns the new entry or NULL if something fails
SymtableItem *symtable_add_function(Symtable *st, char *var_name, int param_count, bool is_defined);

/// Checks if a getter with the given name exists in the symtable
bool symtable_contains_getter(Symtable *st, char *var_name, SymtableItem **out_item);

/// Checks if a setter with the given name exists in the symtable
bool symtable_contains_setter(Symtable *st, char *var_name, SymtableItem **out_item);

/// Checks if a function with the given name and parameter count exists in the symtable
bool symtable_contains_function(Symtable *st, char *var_name, int param_count, SymtableItem **out_item);

/// Adds a new entry to the symtable for the global variable. Returns the new entry or NULL if something fails
SymtableItem *symtable_add_global_var(Symtable *st, char *var_name, DataType data_type, bool is_defined);

/// Checks if a global variable with the given name exists in the symtable
bool symtable_contains_global_var(Symtable *st, char *var_name, SymtableItem **out_item);

/// Increments the undefined items counter
void symtable_increment_undefined_items_counter(Symtable *st);

/// Decrements the undefined items counter
void symtable_decrement_undefined_items_counter(Symtable *st);

/// Returns the number of undefined items in the symtable
size_t symtable_get_undefined_items_count(Symtable *st);

/// TODO: Removing? Probably not needed

#endif // !_SYMTABLE_H_
