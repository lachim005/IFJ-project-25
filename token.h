#ifndef _TOKEN_H_
#define _TOKEN_H_

#include "string.h"

typedef enum tok_type {
    TOK_IDENTIFIER, // Name stored in string_val
    TOK_GLOBAL_VAR, // Name stored in string_val
    TOK_EOL,
    // Literals
    TOK_LIT_INT, // Value stored in int_val
    TOK_LIT_DOUBLE, // Value stored in double_val
    TOK_LIT_STRING, // Value stored in string_val
    // Operators
    TOK_OP_ASSIGN, // =
    TOK_OP_PLUS, // +
    TOK_OP_MINUS, // -
    TOK_OP_MULT, // *
    TOK_OP_DIV, // /
    TOK_OP_DOT, // .
    TOK_OP_GREATER, // >
    TOK_OP_LESS, // <
    TOK_OP_GREATER_EQ, // >=
    TOK_OP_LESS_EQ, // <=
    TOK_OP_EQ, // ==
    TOK_OP_NOT_EQ, // !=
    TOK_OP_IS, // include in keywords table!
    // Keywords
    TOK_KW_CLASS,
    TOK_KW_IF,
    TOK_KW_ELSE,
    TOK_KW_NULL,
    TOK_KW_RETURN,
    TOK_KW_VAR,
    TOK_KW_WHILE,
    TOK_KW_IFJ,
    TOK_KW_STATIC,
    TOK_KW_IMPORT,
    TOK_KW_FOR,
    // Parantheses
    TOK_LEFT_PAR, // (
    TOK_RIGHT_PAR, // )
    TOK_LEFT_BRACE, // {
    TOK_RIGHT_BRACE, // }
    // Data types (include in keywords table!)
    TOK_TYPE_NUM,
    TOK_TYPE_STRING,
    TOK_TYPE_NULL,
} TokType;

typedef struct token {
    TokType type;
    union {
        String *string_val;
        int int_val;
        double double_val;
    };
} Token;

#endif // !_TOKEN_H_
