/*
 * ast.h
 * Defines structures to represent the abstract syntax tree
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 * Vojtěch Borýsek (xborysv00)
 * Tomáš Hanák (xhanakt00)
 */
#ifndef _AST_H_
#define _AST_H_

#include "string.h"
#include <stdlib.h>

// Forward declaration to avoid circular dependency
typedef struct symtable Symtable;

typedef enum data_type {
    DT_NULL,
    DT_NUM,
    DT_STRING,
    DT_BOOL,

    // New types
    DT_UNKNOWN,
    DT_TYPE
} DataType;

#define IS_DATA_TYPE(dt) \
    ((dt) == DT_NUM || (dt) == DT_STRING || (dt) == DT_BOOL || (dt) == DT_NULL)

/// Type of expression
typedef enum ast_expr_type {
    EX_ID,
    EX_GLOBAL_ID,
    EX_GETTER,
    EX_FUN,
    EX_ADD,
    EX_SUB,
    EX_MUL,
    EX_DIV,
    EX_GREATER,
    EX_LESS,
    EX_GREATER_EQ,
    EX_LESS_EQ,
    EX_EQ,
    EX_NOT_EQ,
    EX_AND,
    EX_OR,
    EX_IS,
    EX_TERNARY,
    EX_NOT,
    EX_DOUBLE,
    EX_BOOL,
    EX_NULL,
    EX_STRING,
    EX_NEGATE, // Unary -
    EX_DATA_TYPE,
    EX_BUILTIN_FUN,
} AstExprType;

/// Structure holding expression
typedef struct ast_expression AstExpression;
struct ast_expression {
    /// Subtrees for operands or function parameters
    AstExpression **params;
    /// Number of children in params, 1 - unary, 2 - binary, 3 - ternary, everything for functions (0 - N)
    size_t child_count;
    /// Expression type
    AstExprType type;
    /// Type assumption from static analysis
    DataType assumed_type;
    /// Value for literals
    union {
        String *string_val;
        int int_val;
        double double_val;
        bool bool_val;
        DataType data_type;
    };
};

// Forward declarations
typedef struct ast_block AstBlock;
typedef struct ast_statement AstStatement;
typedef struct ast_function AstFunction;
typedef struct ast_getter AstGetter;
typedef struct ast_setter AstSetter;
typedef struct ast_variable AstVariable;
typedef struct ast_if_statement AstIfStatement;
typedef struct ast_while_statement AstWhileStatement;

/// Statement type, used to tag the union in AstStatement
typedef enum ast_statement_type {
    ST_ROOT,
    ST_BLOCK,
    ST_IF,
    ST_WHILE,
    ST_RETURN,
    ST_LOCAL_VAR,
    ST_GLOBAL_VAR,
    ST_SETTER_CALL,
    ST_FUNCTION,
    ST_GETTER,
    ST_SETTER,
    ST_EXPRESSION,
    ST_END
} AstStatementType;

/// Structure holding a block with a statement list
typedef struct ast_block {
    /// First statement in the block
    AstStatement *statements;
} AstBlock;

/// Structure holding a function
typedef struct ast_function {
    /// Name of the function
    String *name;

    /// Number of parameters
    size_t param_count;

    /// Name of parameters
    String **param_names;

    /// Function body
    AstBlock *body;

    /// SymTable for local variables
    Symtable *symtable;
} AstFunction;

/// Structure holding a getter
typedef struct ast_getter {
    /// Name of the getter
    String *name;

    /// Body of the getter
    AstBlock *body;

    /// SymTable for local variables
    Symtable *symtable;
} AstGetter;

/// Structure holding a setter
typedef struct ast_setter {
    /// Name of the setter
    String *name;

    /// Name of the parameter
    String *param_name;

    /// Body of the setter
    AstBlock *body;

    /// SymTable for local variables
    Symtable *symtable;
} AstSetter;

/// Structure holding a var
typedef struct ast_variable {
    /// Name of the variable
    String *name;

    /// Expression assigned to the variable
    AstExpression *expression;
} AstVariable;

/// Structure holding a function call
typedef struct ast_function_call {
    /// Name of the function
    String *name;

    /// Arguments expressions
    AstExpression *arguments;

    /// Number of arguments
    size_t argument_count;
} AstFunctionCall;

/// Structure holding a if statement
typedef struct ast_if_statement {
    /// Condition expression
    AstExpression *condition;

    /// True branch
    AstBlock *true_branch;

    /// False branch
    AstBlock *false_branch;
} AstIfStatement;

typedef struct ast_while_statement {
    /// Condition expression
    AstExpression *condition;

    /// Body of the while loop
    AstBlock *body;
} AstWhileStatement;

/// Stucture representing a statement
typedef struct ast_statement {
    /// Statement type
    AstStatementType type;

    /// Next statement in the statement list
    AstStatement *next;

    /// Union holding specific statement data
    union {
        AstBlock *block;
        AstIfStatement *if_st;
        AstWhileStatement *while_st;
        AstExpression *return_expr;
        AstVariable *local_var;
        AstVariable *global_var;
        AstVariable *setter_call;
        AstFunction *function;
        AstGetter *getter;
        AstSetter *setter;
        AstExpression *expression;
    };
} AstStatement;

// AST creation functions

/// Creates a new AST expression node
AstExpression *ast_expr_create(AstExprType type, size_t child_count);

/// Initializes a new AST statement
AstStatement *ast_statement_init();

/// Creates a new AST block
AstBlock *ast_block_create();

/// Adds an if statement to the AST
bool ast_add_if_statement(AstStatement *statement, AstExpression *condition);

/// Adds an else branch to an existing if statement
bool ast_add_else_branch(AstStatement *if_statement);

/// Adds a while statement to the AST
bool ast_add_while_statement(AstStatement *statement, AstExpression *condition);

/// Adds a return statement to the AST
bool ast_add_return_statement(AstStatement *statement, AstExpression *return_expr);

/// Adds a local variable to the AST
bool ast_add_local_var(AstStatement *statement, char *name, AstExpression *expression);

/// Adds a global variable to the AST
bool ast_add_global_var(AstStatement *statement, char *name, AstExpression *expression);

/// Adds a getter to the AST
bool ast_add_getter(AstStatement *statement, char *name, Symtable *symtable);

/// Adds a setter to the AST
bool ast_add_setter(AstStatement *statement, char *name, char *param_name, Symtable *symtable);

/// Adds a function to the AST program
bool ast_add_function(AstStatement *statement, char *name, size_t param_count, Symtable *symtable, String **param_names);

/// Adds a setter call to the AST
bool ast_add_setter_call(AstStatement *statement, char *name, AstExpression *expression);

/// Adds a block to the AST
bool ast_add_block(AstStatement *statement);

/// Adds an inline expression to the AST
bool ast_add_inline_expression(AstStatement *statement, AstExpression *expression);

// AST cleanup functions

/// Frees an AST expression recursively
void ast_expr_free(AstExpression *expr);

/// Frees an AST statement recursively
void ast_statement_free(AstStatement *statement);

/// Frees an AST block recursively
void ast_block_free(AstBlock *block);

/// Frees the entire AST starting from the root statement
void ast_free(AstStatement *root);

// AST printing functions

/// Prints the entire AST tree to stdout with indentation
void ast_print(AstStatement *root);

#endif // !_AST_H_
