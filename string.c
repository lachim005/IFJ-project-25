#include "string.h"
#include <stdlib.h>
#include <string.h>

String *str_init() {
    String *str = (String *)malloc(sizeof(String));
    if (str == NULL) {
        return NULL;
    }

    str->val = (char *)malloc(_STRING_INIT_SIZE * sizeof(char));
    if (str->val == NULL) {
        free(str);
        return NULL;
    }

    str->capacity = _STRING_INIT_SIZE;
    str->length = 0;
    str->val[0] = '\0';
    
    return str;
}

bool str_append_string(String *str, char *str_to_append) {
    if (str == NULL || str_to_append == NULL) {
        return false;
    }

    unsigned append_length = strlen(str_to_append);
    if (append_length == 0) {
        return true;
    }

    while (str->length + append_length + 1 > str->capacity) {
        unsigned new_capacity = str->capacity * _STRING_SCALE_FACTOR;
        char *new_val = (char *)realloc(str->val, new_capacity * sizeof(char));
        if (new_val == NULL) {
            return false;
        }
        str->val = new_val;
        str->capacity = new_capacity;
    }

    strcpy(str->val + str->length, str_to_append);
    str->length += append_length;

    return true;
}

bool str_append_char(String *str, char ch) {
    if (str == NULL) {
        return false;
    }

    if (str->length + 2 > str->capacity) {
        unsigned new_capacity = str->capacity * _STRING_SCALE_FACTOR;
        char *new_val = (char *)realloc(str->val, new_capacity * sizeof(char));
        if (new_val == NULL) {
            return false;
        }
        str->val = new_val;
        str->capacity = new_capacity;
    }

    str->val[str->length] = ch;
    str->length++;
    str->val[str->length] = '\0';

    return true;
}

void str_clear(String *str) {
    str->length = 0;
    str->val[0] = '\0';
}

void str_free(String **str) {
    if (str != NULL && *str != NULL) {
        free((*str)->val);
        free(*str);
        *str = NULL;
    }
}