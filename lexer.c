#include "lexer.h"
#include <stdio.h>
#include <stdbool.h>

bool lexer_init(Lexer *lexer, char *filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        return false;
    }

    lexer->file = f;

    return true;
}

void lexer_free(Lexer *lexer) {
    fclose(lexer->file);
    lexer->file = NULL;
}

ErrLex lexer_get_token(Lexer *lexer, Token *tok) {
    return ERR_LEX_UNKNOWN_ERR;
}
