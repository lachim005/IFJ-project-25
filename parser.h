/*
 * parser.c
 * Defines parser
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 */

#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"
#include "error.h"
#include "symtable.h"
#include "token.h"
#include <stdbool.h>

/// Main parsing function
ErrorCode parse();

/// Adds all builtin functions to the symbol table
bool add_builtin_functions(Symtable *symtab);

/// Checks prologue of the program (import "ifj25" for Ifj)
ErrorCode check_prologue(Lexer *lexer);

/// Checks class Program { ... }
ErrorCode check_class_program(Lexer *lexer, Symtable *symtable);

/// Checks the body of a class
ErrorCode check_class_body(Lexer *lexer, Symtable *symtable);

/// Checks the global variable declaration
ErrorCode check_global_var(Lexer *lexer, Symtable *symtable, String *var_name);

/// Checks statics (static functions, getters, and setters)
ErrorCode check_statics(Lexer *lexer, Symtable *symtable);

/// Checks Setter: static identifier = (val) { ... }
ErrorCode checks_setter(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier);

/// Checks Getter: static identifier { ... }
ErrorCode check_getter(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier);

/// Checks Function: static identifier(...) { ... }
ErrorCode check_function(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier);

/// Checks function body
ErrorCode check_body(Lexer *lexer, Symtable *globaltable, Symtable *localtable);

/// Checks local variable declaration
ErrorCode check_local_var(Lexer *lexer, Symtable *globaltable, Symtable *localtable);

/// Checks assignment or function call
ErrorCode check_assignment_or_function_call(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier);

/// Checks if statement
ErrorCode check_if_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable);

/// Checks while statement
ErrorCode check_while_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable);

/// Checks return statement
ErrorCode check_return_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable);

/// Semantic analysis of expression - checks definitions and type compatibility
ErrorCode semantic_check_expression(AstExpression *expr, Symtable *globaltable, Symtable *localtable, DataType *out_type);

#endif
