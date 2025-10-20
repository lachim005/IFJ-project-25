#ifndef _LEXER_H_
#define _LEXER_H_

#include "string.h"
#include "token.h"
#include <stdio.h>
#include <stdbool.h>

typedef struct lexer {
    FILE* file;
    // Current position in file. Used for friendlier error messages
    unsigned pos_line;
    unsigned pos_char;
    bool last_char_was_newline; // Used for counting lines

    // Buffers used for lexical analysis.
    // Used only in get_token, put here so we don't have to create new ones on every call
    String *buf1;
    String *buf2;
} Lexer;

typedef enum lexer_status {
    ERR_LEX_OK,
    ERR_LEX_EOF,
    ERR_LEX_UNKNOWN_ERR,
    ERR_LEX_MALLOC,
    ERR_LEX_UNEXPECTED_AFTER_EXCLAM,
    ERR_LEX_NL_IN_STRING_LITERAL,
    ERR_LEX_UNEXPECTED_EOF,
    ERR_LEX_STRING_UNEXPECTED_ESCAPE_SEQUENCE,
    ERR_LEX_NUM_LIT_UNEXPECTED_CHARACTER,
    ERR_LEX_EXPECTED_GLOBAL_VAR,
    ERR_LEX_UNEXPECTED_CHARACTER,
} ErrLex; // Lexer? More like LexERR

/// Opens the given file and prepares the given lexer structure. Returns false if something went wrong
/// TODO: Error return codes?
bool lexer_init(Lexer *lexer, FILE *input_stream);

/// Frees the inside of the given lexer structure
void lexer_free(Lexer *lexer);

/// Reads the next token and puts the information into tok. Returns the status code
ErrLex lexer_get_token(Lexer *lexer, Token *tok);

#endif // !_LEXER_H_
