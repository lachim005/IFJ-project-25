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

/// Generates code for a compound statement
ErrorCode generate_compound_statement(FILE *output, AstBlock *st);

/// Generates code for a function call
ErrorCode generate_function_call(FILE *output, AstExpression *call);

/// Generates code which will evaluate the given expression
/// Leaves the resulting value at the top of the stack
ErrorCode generate_expression_evaluation(FILE *output, AstExpression *st);

/// Generates code for local variable assignment
ErrorCode generate_var_assignment(FILE *output, char *scope, AstVariable *st);

/// Generates code for global variable assignment
ErrorCode generate_global_assignment(FILE *output, AstVariable *st);

/// Generates code for setter assignment
ErrorCode generate_setter_assignment(FILE *output, AstVariable *st);

/// Generates code for a return statement
ErrorCode generate_return_statement(FILE *output, AstExpression *expr);

/// Generates code for a given statement
ErrorCode generate_statement(FILE *output, AstStatement *st);

/// Generates code for defining a global variable
void declare_global_var(SymtableItem *item, void *par);

/// Generates code for defining a local variable
void declare_local_var(SymtableItem *item, void *par);

// Stores function parameters into local variables
ErrorCode store_function_parameters(FILE *output, AstFunction *fun);

/// Generates code for function/getter/setter body
ErrorCode define_function(FILE *output, AstFunction *fun);

/// Writes code that calls the main function and handles the exit code
void write_runtime(FILE *output);

/// Generates code for the given AST
ErrorCode generate_code(FILE *output, AstStatement *root, Symtable *global_symtable);

#endif // !_CODE_GENERATOR_H_
