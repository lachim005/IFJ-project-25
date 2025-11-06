/*
 * parser.c
 * Implements parser
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 */

#include "parser.h"
#include "error.h"
#include "symtable.h"
#include <string.h>

#define RETRUN_CODE(code) do { \
    token_free(&token); \
    return code; \
} while(0)

#define CHECK_TOKEN(lexer, token) do { \
    token_free(&token); \
    if(lexer_get_token(lexer, &token) != ERR_LEX_OK) { \
        return LEXICAL_ERROR; \
    } \
} while(0)

#define CHECK_TOKEN_SKIP_NEWLINE(lexer, token) do { \
    token_free(&token); \
    do { \
        if(lexer_get_token(lexer, &token) != ERR_LEX_OK) { \
            return LEXICAL_ERROR; \
        } \
    } while(token.type == TOK_NEWLINE); \
} while(0)


/// Checks prolugue of the program 
/// Rule: import "ifj25" for Ifj
ErrorCode check_prologue(Lexer *lexer) {
    Token token;
    token.type = TOK_KW_IFJ;

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_KW_IMPORT)
        RETRUN_CODE(SYNTACTIC_ERROR);

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LIT_STRING || strcmp(token.string_val->val, "ifj25"))
        RETRUN_CODE(SYNTACTIC_ERROR);

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_KW_FOR)
        RETRUN_CODE(SYNTACTIC_ERROR);

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_KW_IFJ)
        RETRUN_CODE(SYNTACTIC_ERROR);

    RETRUN_CODE(OK);
}


ErrorCode parse() {
    Lexer lexer_structure;
    Lexer *lexer = &lexer_structure;
    if (!lexer_init(lexer, stdin)){
        return INTERNAL_ERROR;
    }

    Symtable *symtable = symtable_init();
    printf("%d\n", check_prologue(lexer));
}

int main() {
    parse();
    return 0;
}