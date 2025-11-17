/*
 * main.c
 * Contains the flow of the program
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 * Michal Šebesta (xsebesm00)
 */
#include "ast.h"
#include "code_generator.h"
#include "error.h"
#include "parser.h"
#include "lexer.h"
#include "optimizer.h"
#include <stdio.h>

void print_error_code(ErrorCode ec) {
    switch (ec) {
    case OK:
        fprintf(stderr, "OK"); break;
    case LEXICAL_ERROR:
        fprintf(stderr, "lexical error"); break;
    case SYNTACTIC_ERROR:
        fprintf(stderr, "syntax error"); break;
    case SEM_UNDEFINED:
        fprintf(stderr, "undefined symbol"); break;
    case SEM_REDEFINITION:
        fprintf(stderr, "redefined symbol"); break;
    case SEM_BAD_PARAMS:
        fprintf(stderr, "incorrect parameter count or type"); break;
    case SEM_TYPE_COMPAT:
        fprintf(stderr, "incorrect type in expression"); break;
    case SEM_OTHER:
        fprintf(stderr, "unknown semantic error"); break;
    case INTERNAL_ERROR:
        fprintf(stderr, "internal error"); break;
    }
}

int main() {
    Lexer lexer;
    if (!lexer_init(&lexer, stdin)) return INTERNAL_ERROR;

    AstStatement *ast_root = NULL;
    Symtable *glob_symtable = NULL;
    ErrorCode ec = parse(&lexer, &ast_root, &glob_symtable);

    lexer_free(&lexer);
    if (ec != OK) {
        fprintf(stderr, "error at %d:%d: ", lexer.pos_line, lexer.pos_char);
        print_error_code(ec);
        fprintf(stderr, "\n");
        ast_free(ast_root);
        symtable_free(glob_symtable);
        return ec;
    }

    //ast_print(ast_root);

    ec = optimize_ast(ast_root, glob_symtable);
    if (ec != OK) {
        fprintf(stderr, "error: ");
        print_error_code(ec);
        fprintf(stderr, "\n");
        ast_free(ast_root);
        symtable_free(glob_symtable);
        return ec;
    }

    ec = generate_code(stdout, ast_root, glob_symtable);
    if (ec != OK) {
        fprintf(stderr, "error: ");
        print_error_code(ec);
        fprintf(stderr, "\n");
        ast_free(ast_root);
        symtable_free(glob_symtable);
        return ec;
    }

    ast_free(ast_root);
    symtable_free(glob_symtable);
    return 0;
}
