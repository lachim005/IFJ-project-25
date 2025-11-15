/*
 * token.c
 * Implements token operations
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 */
#include "token.h"
#include "ast.h"
#include "string.h"

void token_free(Token *token) {
    switch (token->type) {
    case TOK_IDENTIFIER:
    case TOK_GLOBAL_VAR:
    case TOK_LIT_STRING:
        str_free(&token->string_val);
        break;
    case TOK_E:
        ast_expr_free(token->expr_val);
        break;
    default:
        break;
    }
}
