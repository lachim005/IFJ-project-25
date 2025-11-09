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
#include "ast.h"
#include <stdbool.h>

/// Main parsing function
ErrorCode parse();

/// Adds all builtin functions to the symbol table
bool add_builtin_functions(Symtable *symtab);

/// Checks prologue of the program (import "ifj25" for Ifj)
ErrorCode check_prologue(Lexer *lexer);

/// Checks class Program { ... }
ErrorCode check_class_program(Lexer *lexer, Symtable *symtable, AstStatement *statement);

/// Checks the body of a class
ErrorCode check_class_body(Lexer *lexer, Symtable *symtable, AstStatement *statement);

/// Checks the global variable declaration
ErrorCode check_global_var(Lexer *lexer, Symtable *globaltable, Symtable *localtable, String *var_name, AstStatement *statement);

/// Checks statics (static functions, getters, and setters)
ErrorCode check_statics(Lexer *lexer, Symtable *symtable, AstStatement *statement);

/// Checks Setter: static identifier = (val) { ... }
ErrorCode checks_setter(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier, AstStatement *statement);

/// Checks Getter: static identifier { ... }
ErrorCode check_getter(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier, AstStatement *statement);

/// Checks Function: static identifier(...) { ... }
ErrorCode check_function(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier, AstStatement *statement);

/// Checks function body
ErrorCode check_body(Lexer *lexer, Symtable *globaltable, Symtable *localtable, bool known, AstStatement *statement);

/// Checks local variable declaration
ErrorCode check_local_var(Lexer *lexer, Symtable *globaltable, Symtable *localtable, bool known, AstStatement *statement);

/// Checks assignment or function call
ErrorCode check_assignment_or_function_call(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier, bool known, AstStatement *statement);

/// Checks if statement
ErrorCode check_if_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable, AstStatement *statement);

/// Checks while statement
ErrorCode check_while_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable, AstStatement *statement);

/// Checks Ifj statement
ErrorCode check_ifj_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token keyword, AstStatement *statement);

/// Checks return statement
ErrorCode check_return_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable, AstStatement *statement);

/// Semantic analysis of expression - checks definitions and type compatibility
ErrorCode semantic_check_expression(AstExpression *expr, Symtable *globaltable, Symtable *localtable, DataType *out_type);

// Helper structure for managing function parameters
typedef struct {
    String **names;
    size_t count;
    size_t capacity;
} ParamList;

/// Initializes a parameter list
ParamList *param_list_init();

/// Adds a parameter name to the list (creates a copy of the string)
bool param_list_add(ParamList *list, const char *name);

/// Frees the parameter list and all its strings
void param_list_free(ParamList *list);

#endif
