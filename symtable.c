/*
 * symtable.c
 * Defines symtable interface
 *
 * IFJ project 2025
 * FIT VUT
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 * Tomáš Hanák (xhanakt00)
 * Šimon Halas (xhalass00)
 */

#include "symtable.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    st->scope_stack = malloc(sizeof(int) * ST_SCOPE_STACK_INITIAL_CAPACITY);
    if (st->scope_stack == NULL) {
        free(st->state);
        free(st->data);
        free(st);
        return NULL;
    }
    st->scope_stack_count = 0;
    st->scope_stack_capacity = ST_SCOPE_STACK_INITIAL_CAPACITY;
    st->undefined_items_counter = 0;
    
    return st;
}

void symtable_free(Symtable *st) {
    if (!st) return;
    // Free all keys
    for (size_t i = 0; i < st->capacity; ++i) {
        if (st->state[i] == SLOT_OCCUPIED && st->data[i].key) {
            SymtableItem it = st->data[i];
            free(it.key);
            str_free(&it.name);
            if (it.param_types) {
                free(it.param_types);
            }
            if (it.data_type_known && it.data_type == DT_STRING) {
                str_free(&it.string_val);
            }
        }
    }
    free(st->data);
    free(st->state);
    free(st->scope_stack);
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
            unsigned long h = hash_str(old_data[i].key);
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

SymtableItem *symtable_find(Symtable *st, const char *key) {
    if (!st || !key) return NULL;
    if (st->capacity == 0) return NULL;

    unsigned long h = hash_str(key);
    size_t cap = st->capacity;
    size_t probe = (size_t)(h % cap);

    for (size_t i = 0; i < cap; ++i) {
        size_t idx = (probe + i) % cap;
        int s = st->state[idx];
        if (s == SLOT_EMPTY) return NULL; // not found, and no further possible
        if (s == SLOT_OCCUPIED && strcmp(st->data[idx].key, key) == 0) {
            return &st->data[idx];
        }
        // continue on deleted or occupied-but-not-equal
    }
    return NULL;
}

bool symtable_contains(Symtable *st, const char *key) {
    return symtable_find(st, key) != NULL;
}

SymtableItem *symtable_insert(Symtable *st, const char *key) {
    if (!st || !key) return NULL;

    // Prevent duplicates
    if (symtable_contains(st, key)) return NULL;

    // Grow when load factor > 0.7
    if ((st->size + 1) * 10 >= st->capacity * 7) {
        size_t newcap = st->capacity * 2;
        if (!symtable_rehash(st, newcap)) return NULL;
    }

    unsigned long h = hash_str(key);
    size_t cap = st->capacity;
    size_t probe = (size_t)(h % cap);
    ssize_t first_deleted = -1;

    for (size_t i = 0; i < cap; ++i) {
        size_t idx = (probe + i) % cap;
        int s = st->state[idx];
        if (s == SLOT_OCCUPIED) {
            if (strcmp(st->data[idx].key, key) == 0) {
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
        // Allocate and copy the key
        st->data[use_idx].key = malloc(strlen(key) + 1);
        if (!st->data[use_idx].key) return NULL;
        strcpy(st->data[use_idx].key, key);
        st->data[use_idx].name = str_init();
        if (!st->data[use_idx].name) return NULL;
        str_append_string(st->data[use_idx].name, key);
        st->data[use_idx].data_type = DT_UNKNOWN;
        st->data[use_idx].is_defined = true;
        st->data[use_idx].type = SYM_VAR;
        st->data[use_idx].param_types = NULL;
        st->data[use_idx].data_type_known = false;
        st->data[use_idx].string_val = NULL;
        st->state[use_idx] = SLOT_OCCUPIED;
        st->size++;
        return &st->data[use_idx];
    }

    // Table full, should not happen due to rehashing
    return NULL;
}

void symtable_foreach(Symtable *st, void (*fun)(SymtableItem*, void*), void *par) {
    for (unsigned i = 0; i < st->capacity; i++) {
        if (st->state[i] != SLOT_OCCUPIED) continue;
        fun(st->data + i, par);
    }
}

// Global counter for scope ids, so every scope has a unique id
int scope_id_counter = 0;

bool enter_scope(Symtable *st) {
    if (st->scope_stack_count == st->scope_stack_capacity) {
        st->scope_stack_capacity <<= 1;
        int *new_st = realloc(st->scope_stack, sizeof(int) * st->scope_stack_capacity);
        if (new_st == NULL) return false;
        st->scope_stack = new_st;
    }
    st->scope_stack[st->scope_stack_count++] = scope_id_counter++;
    return true;
}

void exit_scope(Symtable *st) {
    if (st->scope_stack_count == 0){
        return;
    }
    st->scope_stack_count--;
}

int current_scope(Symtable *st) {
    return st->scope_stack[st->scope_stack_count - 1];
}

bool append_scope_id(String *s, int scope) {
    // Converts buffer id to string
    char buf[64];
    int written = snprintf(buf, 64, "%d", scope);
    if (written >= 64) return false;
    str_append_string(s, buf);
    return true;
}

bool find_local_var(Symtable *st, char *var_name, SymtableItem **out_item) {
    if (st == NULL) {
        return true;
    }

    // Assumes format var_name?scope_id
    String *s = str_init();
    if (s == NULL) return false;
    
    for (int i = st->scope_stack_count - 1; i >= 0; i--) {
        // Constructs symtable name
        str_clear(s);
        str_append_string(s, var_name);
        str_append_char(s, '?');
        if (!append_scope_id(s, st->scope_stack[i])) {
            str_free(&s);
            return false;
        }

        SymtableItem *it = symtable_find(st, s->val);
        if (it != NULL) {
            *out_item = it;
            str_free(&s);
            return true;
        }
    }
    
    // Not found
    *out_item = NULL;
    str_free(&s);
    return true;
}

SymtableItem *add_var_at_current_scope(Symtable *st, char *var_name, DataType data_type) {
    // Assumes format var_name?scope_id
    String *s = str_init();
    if (s == NULL) return NULL;
    
    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '?');
    if (!append_scope_id(s, current_scope(st))) {
        str_free(&s);
        return NULL;
    }
    
    SymtableItem *result = symtable_insert(st, s->val);
    str_free(&s);
    if (result == NULL) {
        return NULL;
    }

    str_clear(result->name);
    str_append_string(result->name, var_name);
    result->data_type = data_type;
    return result;
}

bool contains_var_at_current_scope(Symtable *st, char *var_name) {
    // Assumes format var_name?scope_id
    String *s = str_init();
    if (s == NULL) return false;
    
    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '?');
    if (!append_scope_id(s, current_scope(st))) {
        str_free(&s);
        return false;
    }
    
    bool result = symtable_contains(st, s->val);
    str_free(&s);
    return result;
}

SymtableItem *symtable_add_getter(Symtable *st, char *var_name, bool is_defined) {
    // Assumes format var_name!
    String *s = str_init();
    if (s == NULL) return NULL;

    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '!');

    SymtableItem *result = symtable_insert(st, s->val);
    str_free(&s);

    if (result == NULL)
        return NULL;

    str_clear(result->name);
    str_append_string(result->name, var_name);
    result->is_defined = is_defined;
    result->param_count = 0;
    result->type = SYM_GETTER;

    return result;
}

bool symtable_contains_getter(Symtable *st, char *var_name, SymtableItem **out_item) {
    // Assumes format var_name!
    String *s = str_init();
    if (s == NULL) return false;

    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '!');

    bool result = symtable_contains(st, s->val);

    if (out_item) {
        *out_item = symtable_find(st, s->val);
    }

    str_free(&s);
    
    return result;
}

SymtableItem *symtable_add_setter(Symtable *st, char *var_name, bool is_defined) {
    // Assumes format var_name*
    String *s = str_init();
    if (s == NULL) return NULL;

    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '*');

    SymtableItem *result = symtable_insert(st, s->val);
    str_free(&s);

    if (result == NULL)
        return NULL;

    str_clear(result->name);
    str_append_string(result->name, var_name);
    result->is_defined = is_defined;
    result->param_count = 1;
    result->type = SYM_SETTER;

    return result;
}

bool symtable_contains_setter(Symtable *st, char *var_name, SymtableItem **out_item) {
    // Assumes format var_name*
    String *s = str_init();
    if (s == NULL) return false;

    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '*');

    bool result = symtable_contains(st, s->val);

    if (out_item) {
        *out_item = symtable_find(st, s->val);
    }

    str_free(&s);

    return result;
}

SymtableItem *symtable_add_function(Symtable *st, char *var_name, int param_count, bool is_defined) {
    // Assumes format var_name$param_count
    String *s = str_init();
    if (s == NULL) return NULL;

    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '$');

    // Append param count
    char buf[64];
    int written = snprintf(buf, 64, "%d", param_count);
    if (written >= 64) {
        str_free(&s);
        return NULL;
    }
    str_append_string(s, buf);

    SymtableItem *result = symtable_insert(st, s->val);
    str_free(&s);

    if (result == NULL)
        return NULL;

    str_clear(result->name);
    str_append_string(result->name, var_name);
    result->is_defined = is_defined;
    result->param_count = (size_t)param_count;
    result->type = SYM_FUNCTION;
    return result;
}

bool symtable_contains_function(Symtable *st, char *var_name, int param_count, SymtableItem **out_item) {
    // Assumes format var_name$param_count
    String *s = str_init();
    if (s == NULL) return false;

    // Constructs symtable name
    str_append_string(s, var_name);
    str_append_char(s, '$');

    // Append param count
    char buf[64];
    int written = snprintf(buf, 64, "%d", param_count);
    if (written >= 64) {
        str_free(&s);
        return false;
    }
    str_append_string(s, buf);

    bool result = symtable_contains(st, s->val);
    if (out_item) {
        *out_item = symtable_find(st, s->val);
    }

    str_free(&s);
    return result;
}

SymtableItem *symtable_add_global_var(Symtable *st, char *var_name, DataType data_type, bool is_defined) {
    SymtableItem *result = symtable_insert(st, var_name);
    if (result == NULL) {
        return NULL;
    }

    str_clear(result->name);
    str_append_string(result->name, var_name);
    result->data_type = data_type;
    result->is_defined = is_defined;
    result->type = SYM_GLOBAL_VAR;
    return result;
}

bool symtable_contains_global_var(Symtable *st, char *var_name, SymtableItem **out_item) {
    SymtableItem *item = symtable_find(st, var_name);
    if (item != NULL && item->type == SYM_GLOBAL_VAR) {
        if (out_item) {
            *out_item = item;
        }
        return true;
    }
    if (out_item) {
        *out_item = NULL;
    }
    return false;
}

SymtableItem *add_builtin_function(Symtable *symtab, const char *name, int param_count, DataType return_type, DataType *param_types) {
    // Constructs symtable name
    String *s = str_init();
    if (s == NULL) return NULL;

    str_append_string(s, name);

    SymtableItem *result = symtable_insert(symtab, s->val);
    str_free(&s);

    if (result == NULL)
        return NULL;

    str_clear(result->name);
    str_append_string(result->name, name);
    result->is_defined = true;
    result->param_count = (size_t)param_count;
    result->type = SYM_FUNCTION;
    result->data_type = return_type;

    // Copy parameter types if provided
    if (param_types != NULL && param_count > 0) {
        result->param_types = malloc(sizeof(DataType) * param_count);
        if (result->param_types == NULL) {
            return NULL;
        }
        for (int i = 0; i < param_count; i++) {
            result->param_types[i] = param_types[i];
        }
    } else {
        result->param_types = NULL;
    }

    return result;
}

bool symtable_contains_builtin_function(Symtable *st, const char *name, SymtableItem **out_item) {
    // Constructs symtable name
    String *s = str_init();
    if (s == NULL) return false;

    str_append_char(s, '#');
    str_append_string(s, name);

    bool result = symtable_contains(st, s->val);
    if (out_item) {
        *out_item = symtable_find(st, s->val);
    }

    str_free(&s);
    return result;
}

void symtable_increment_undefined_items_counter(Symtable *st) {
    if (st) {
        st->undefined_items_counter++;
    }
}

void symtable_decrement_undefined_items_counter(Symtable *st) {
    if (st && st->undefined_items_counter > 0) {
        st->undefined_items_counter--;
    }
}

size_t symtable_get_undefined_items_count(Symtable *st) {
    if (st) {
        return st->undefined_items_counter;
    }
    return 0;
}
