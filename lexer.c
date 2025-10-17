#include "lexer.h"
#include "string.h"
#include "token.h"
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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
    // String literals
    S_STR_START,
    S_STR_INSIDE,
    S_STR_ESCAPE,
    S_STR_ESCAPE_CODE_1,
    S_STR_ESCAPE_CODE_2,
    S_STR_END,
    S_STR_EMPTY,
    S_STR_MULTILINE_INSIDE,
    S_STR_MULTILINE_END1,
    S_STR_MULTILINE_END2,
    // Number literals
    S_ZERO,
    S_INT_LIT,
    S_INT_HEX_LIT,
    S_FLOAT_DOT,
    S_FLOAT_DECIMAL,
    S_FLOAT_E,
    S_FLOAT_E_SIGN,
    S_FLOAT_EXPONENT,
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

bool hex2int(char *hexStr, int *out) {
    int val = 0;
    for (int i = 0; hexStr[i] != '\0'; i++) {
        char c = tolower(hexStr[i]);
        val *= 16;
        if (isdigit(c)) {
            val += c - '0';
            continue;
        }
        if (isalpha(c)) {
            val += c - 'a' + 10;
            continue;
        }
        return false;
    }
    *out = val;
    return true;
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
                case '"': MOVE_STATE(S_STR_START);
                case '0': str_append_char(buf1, ch); MOVE_STATE(S_ZERO);
            }
            if (isdigit(ch)) { str_append_char(buf1, ch); MOVE_STATE(S_INT_LIT); }
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
        case S_STR_START:
            if (ch == '"') MOVE_STATE(S_STR_EMPTY);
            if (ch == '\\') MOVE_STATE(S_STR_ESCAPE);
            str_append_char(buf1, ch);
            MOVE_STATE(S_STR_INSIDE);
        case S_STR_INSIDE:
            if (ch == '"') MOVE_STATE(S_STR_END);
            if (ch == '\\') MOVE_STATE(S_STR_ESCAPE);
            str_append_char(buf1, ch);
            continue;
        case S_STR_ESCAPE:
            if (ch == 'x') MOVE_STATE(S_STR_ESCAPE_CODE_1);
            char escaped_char = 0;
            switch (ch) {
                case '"': escaped_char = '"'; break;
                case 'r': escaped_char = '\r'; break;
                case 'n': escaped_char = '\n'; break;
                case 't': escaped_char = '\t'; break;
                case '\\': escaped_char = '\\'; break;
            }
            if (escaped_char == 0) return ERR_LEX_STRING_UNEXPECTED_ESCAPE_SEQUENCE;
            str_append_char(buf1, escaped_char);
            MOVE_STATE(S_STR_INSIDE);
        case S_STR_ESCAPE_CODE_1:
            str_append_char(buf2, ch);
            MOVE_STATE(S_STR_ESCAPE_CODE_2);
        case S_STR_ESCAPE_CODE_2:
            str_append_char(buf2, ch);
            int code;
            if (!hex2int(buf2->val, &code)) return ERR_LEX_STRING_UNEXPECTED_ESCAPE_SEQUENCE;
            str_clear(buf2);
            str_append_char(buf1, code);
            MOVE_STATE(S_STR_INSIDE)
        case S_STR_END:
            UNGET;
            tok->string_val = str_init();
            if (tok->string_val == NULL) return ERR_LEX_MALLOC;
            str_append_string(tok->string_val, buf1->val);
            FOUND_TOK(TOK_LIT_STRING);
        case S_STR_EMPTY:
            if (ch == '"') MOVE_STATE(S_STR_MULTILINE_INSIDE);
            UNGET;
            tok->string_val = str_init();
            if (tok->string_val == NULL) return ERR_LEX_MALLOC;
            FOUND_TOK(TOK_LIT_STRING);
        case S_STR_MULTILINE_INSIDE:
            // TODO: Fix multiline string literal whitespace trimming
            if (ch == '"') MOVE_STATE(S_STR_MULTILINE_END1);
            str_append_char(buf1, ch);
            continue;
        case S_STR_MULTILINE_END1:
            if (ch == '"') MOVE_STATE(S_STR_MULTILINE_END2);
            str_append_char(buf1, '"');
            str_append_char(buf1, ch);
            MOVE_STATE(S_STR_MULTILINE_INSIDE);
        case S_STR_MULTILINE_END2:
            if (ch == '"') MOVE_STATE(S_STR_END);
            str_append_char(buf1, '"');
            str_append_char(buf1, '"');
            str_append_char(buf1, ch);
            MOVE_STATE(S_STR_MULTILINE_INSIDE);
        case S_ZERO:
            if (ch == 'x') MOVE_STATE(S_INT_HEX_LIT);
            // Continues down:
        case S_INT_LIT:
            str_append_char(buf1, ch);
            if (ch == 'e' || ch == 'E') MOVE_STATE(S_FLOAT_E);
            if (ch == '.') MOVE_STATE(S_FLOAT_DOT);
            if (isdigit(ch)) MOVE_STATE(S_INT_LIT);
            UNGET;
            str_remove_last(buf1);
            tok->int_val = strtol(buf1->val, NULL, 10); // Should not fail
            FOUND_TOK(TOK_LIT_INT);
        case S_INT_HEX_LIT:
            if (isdigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'Z')) {
                str_append_char(buf1, ch);
                continue;
            }
            UNGET;
            hex2int(buf1->val, &tok->int_val); // Should not fail
            FOUND_TOK(TOK_LIT_INT);
        case S_FLOAT_DOT:
            if (!isdigit(ch)) return ERR_LEX_NUM_LIT_UNEXPECTED_CHARACTER;
            str_append_char(buf1, ch);
            MOVE_STATE(S_FLOAT_DECIMAL);
        case S_FLOAT_E:
            str_append_char(buf1, ch);
            if (ch == '+' || ch == '-') MOVE_STATE(S_FLOAT_E_SIGN);
            if (isdigit(ch)) MOVE_STATE(S_FLOAT_EXPONENT);
            return ERR_LEX_NUM_LIT_UNEXPECTED_CHARACTER;
        case S_FLOAT_E_SIGN:
            str_append_char(buf1, ch);
            if (isdigit(ch)) MOVE_STATE(S_FLOAT_EXPONENT);
            return ERR_LEX_NUM_LIT_UNEXPECTED_CHARACTER;
        case S_FLOAT_DECIMAL:
            if (ch == 'e' || ch == 'E') { str_append_char(buf1, 'e'); MOVE_STATE(S_FLOAT_E); }
            // Continues down:
        case S_FLOAT_EXPONENT:
            str_append_char(buf1, ch);
            if (isdigit(ch)) continue;
            str_remove_last(buf1);
            UNGET;
            tok->double_val = strtod(buf1->val, NULL); // Should not fail
            FOUND_TOK(TOK_LIT_DOUBLE);
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
