/*
 * code_generator.h
 * Defines code generator
 *
 * IFJ project 2025
 * FIT VUT
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

/// Generates code for an if statement
ErrorCode generate_if_statement(FILE *output, AstIfStatement *st);

/// Generates code for a while cycle
ErrorCode generate_while_statement(FILE *output, AstWhileStatement *st);

/// Generates code for a function call
ErrorCode generate_function_call(FILE *output, AstExpression *call);

/// Generates code for an AND expression ex
ErrorCode generate_and_expr(FILE *output, AstExpression *ex);

/// Generates code for an OR expression ex
ErrorCode generate_or_expr(FILE *output, AstExpression *ex);

/// Generates code for an is expression ex
ErrorCode generate_is_expr(FILE *output, AstExpression *ex);

/// Generates code for a ternary expression ex
ErrorCode generate_ternary_expr(FILE *output, AstExpression *ex);

/// Converts a given input string to the IFJcode25 literal format
ErrorCode convert_string(char *input, String **out);

/// Generates code for a builtin function call
ErrorCode generate_builtin_function_call(FILE *output, AstExpression *ex);

/// Generates code for a + expression
ErrorCode generate_add_expression(FILE *output, AstExpression *ex);

/// Generates code for a * expression
ErrorCode generate_mul_expression(FILE *output, AstExpression *ex);

/// Generates code for evaluating a binary operator which only accepts float on both sides
ErrorCode generate_binary_operator_with_floats(FILE *output, AstExpression *ex, char *stack_op);

/// Generates code for a == expression
ErrorCode generate_equals_expression(FILE *output, AstExpression *ex);

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
