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

int main() {
    Lexer lexer;
    if (!lexer_init(&lexer, stdin)) return INTERNAL_ERROR;

    AstStatement *ast_root = NULL;
    Symtable *glob_symtable = NULL;
    ErrorCode ec = parse(&lexer, &ast_root, &glob_symtable);

    lexer_free(&lexer);
    if (ec != OK) {
        fprintf(stderr, "Error: %d\n", ec);
        ast_free(ast_root);
        symtable_free(glob_symtable);
        return ec;
    }

    // ast_print(ast_root);

    generate_code(stdout, ast_root, glob_symtable);

    ast_free(ast_root);
    symtable_free(glob_symtable);
    return 0;
}
