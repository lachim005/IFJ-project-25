#include "lexer.h"
#include "string.h"
#include "token.h"
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum lex_fsm_state {
    S_START,
    // Operators and comments
    S_LESS_THAN,
    S_GREATER_THAN,
    S_EXCLAMATION,
    S_EQ,
    S_DIV,
    S_COMMENT,
    S_MULTILINE_COMMENT,
    S_MULTILINE_COMMENT_END,
} LexFsmState;

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

#define FOUND_TOK(token) {tok->type = token; found_tok=true; continue;}
#define MOVE_STATE(s) {state = s; continue;}
#define UNGET ungetc(ch, lexer->file);

ErrLex lexer_get_token(Lexer *lexer, Token *tok) {
    LexFsmState state = S_START;

    // Allocates some buffers
    String *buf1 = str_init();
    if (buf1 == NULL) goto ERR_BUF1;
    String *buf2 = str_init();
    if (buf2 == NULL) goto ERR_BUF2;

    bool found_tok = false;

    int ch;
    while (!found_tok && ((ch = fgetc(lexer->file)) != EOF)) {
        printf("lexing char: %c\n", ch);
        switch (state) {
        case S_START:
            switch (ch) {
                case '\n': FOUND_TOK(TOK_EOL);
                case '+': FOUND_TOK(TOK_OP_PLUS);
                case '-': FOUND_TOK(TOK_OP_MINUS);
                case '*': FOUND_TOK(TOK_OP_MULT);
                case '/': MOVE_STATE(S_DIV);
                case '.': FOUND_TOK(TOK_OP_DOT);
                case '>': MOVE_STATE(S_GREATER_THAN);
                case '<': MOVE_STATE(S_LESS_THAN);
                case '!': MOVE_STATE(S_EXCLAMATION);
                case '=': MOVE_STATE(S_EQ);
            }
            if (isspace(ch)) continue;
            break;
        case S_LESS_THAN:
            if (ch == '=') FOUND_TOK(TOK_OP_LESS_EQ);
            UNGET;
            FOUND_TOK(TOK_OP_LESS);
        case S_GREATER_THAN:
            if (ch == '=') FOUND_TOK(TOK_OP_GREATER_EQ);
            UNGET;
            FOUND_TOK(TOK_OP_GREATER);
        case S_EXCLAMATION:
            if (ch == '=') FOUND_TOK(TOK_OP_NOT_EQ);
            return ERR_LEX_UNEXPECTED_AFTER_EXCLAM;
        case S_EQ:
            if (ch == '=') FOUND_TOK(TOK_OP_EQ);
            UNGET;
            FOUND_TOK(TOK_OP_ASSIGN);
        case S_DIV:
            if (ch == '*') MOVE_STATE(S_MULTILINE_COMMENT);
            if (ch == '/') MOVE_STATE(S_COMMENT);
            UNGET;
            FOUND_TOK(TOK_OP_DIV);
        case S_COMMENT:
            if (ch == '\n') MOVE_STATE(S_START);
            continue;
        case S_MULTILINE_COMMENT:
            if (ch == '*') MOVE_STATE(S_MULTILINE_COMMENT_END);
            continue;
        case S_MULTILINE_COMMENT_END:
            if (ch == '/') MOVE_STATE(S_START);
            if (ch != '*') MOVE_STATE(S_MULTILINE_COMMENT);
            continue;
        }
    }

    str_free(&buf1);
    str_free(&buf2);

    if (found_tok) return ERR_LEX_OK;
    if (ch == EOF) return ERR_LEX_EOF;
    return ERR_LEX_UNKNOWN_ERR;

ERR_BUF2:
    str_free(&buf1);
ERR_BUF1:
    return ERR_LEX_MALLOC;
}
