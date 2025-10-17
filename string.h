#ifndef _STRING_H_
#define _STRING_H_

#define _STRING_INIT_SIZE 32
#define _STRING_SCALE_FACTOR 2

#include <stdbool.h>

typedef struct string {
    char *val;
    /// Capacity allocated at *val
    unsigned capacity;
    /// Current length of the string without the null terminator
    unsigned length;
} String;

/// Creates a new string structure and returns a pointer to it or NULL if allocation failed
String *str_init();

/// Appends a given string to a string. Returns true if the operation was successfull
bool str_append_string(String *str, char *str_to_append);

/// Appends a given character to a string. Returns true if the operation was successfull
bool str_append_char(String *str, char ch);

/// Empties a given string
void str_clear(String *str);

/// Frees a given string
void str_free(String **str);

#endif // !_STRING_H_
