#include "token.h"
#include "string.h"

void token_free(Token *token) {
    switch (token->type) {
    case TOK_IDENTIFIER:
    case TOK_GLOBAL_VAR:
    case TOK_LIT_STRING:
        str_free(&token->string_val);
        break;
    default:
        break;
    }
}
