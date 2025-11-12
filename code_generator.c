#include "code_generator.h"
#include "ast.h"
#include "symtable.h"
#include <stdio.h>

void declare_global_var(SymtableItem *item, void *par) {
    if (item->type != SYM_GLOBAL_VAR) return;

    fprintf((FILE*)par, "DEFVAR GF@%s\n", item->name->val);
}

ErrorCode generate_code(FILE *output, AstStatement *root, Symtable *global_symtable) {
    // IFJcode Header
    fprintf(output, ".IFJcode25\n");

    // Declare global variables
    symtable_foreach(global_symtable, declare_global_var, output);

    fprintf(output, "EXIT int@1\n");
    return OK;
}
