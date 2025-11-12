#include "code_generator.h"
#include "ast.h"
#include <stdio.h>

ErrorCode generate_code(FILE *output, AstStatement *root, Symtable *global_symtable) {
    fprintf(output, "EXIT 1\n");
    return OK;
}
