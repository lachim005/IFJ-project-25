/*
 * code_generator.h
 * Defines code generator
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 */
#ifndef _CODE_GENERATOR_H_
#define _CODE_GENERATOR_H_

#include <stdio.h>
#include "ast.h"
#include "error.h"
#include "symtable.h"

/// Generates code for the given AST
ErrorCode generate_code(FILE *output, AstStatement *root, Symtable *global_symtable);

#endif // !_CODE_GENERATOR_H_
