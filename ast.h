/*
 * ast.h
 * Defines structures to represent the abstract syntax tree
 *
 * Authors:
 * Michal Šebesta (xsebesm00)
 * Vojtěch Borýsek (xborysv00)
 */
#ifndef _AST_H_
#define _AST_H_

#include "string.h"
#include <stdlib.h>

/// Type of expression
typedef enum ast_expr_type {
    EX_ID,
    EX_GLOBAL_ID,
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
    EX_INT,
    EX_DOUBLE,
    EX_STRING,
    EX_NEGATE, // Unary -
} AstExprType;

/// Structure holding expression
typedef struct ast_expression AstExpression;
struct ast_expression {
    /// Identifier for EX_ID or EX_FUN
    String *id;
    /// Subtrees for operands or function parameters
    AstExpression **params;
    /// Number of children in params, 1 - unary, 2 - binary, 3 - ternary, everything for functions (0 - N)
    size_t child_count;
    /// Expression type
    AstExprType type;
    /// Value for literals
    union {
        String *string_val;
        int int_val;
        double double_val;
    };
};
AstExpression *ast_expr_create(AstExprType type, size_t child_count);

typedef struct ast_scope AstScope;
typedef struct ast_statement AstStatement;

/// Statement type, used to tag the union in AstStatement
typedef enum ast_statement_type {
    ST_SCOPE,
    // ...
} AstStatementType;

/// Statement if
typedef struct ast_st_if {
    /// Condition inside if
    AstExpression *condition;
    /// Statement to execute if the condition is true
    AstStatement *st_true;
    /// Statement to execute if the condition is false
    AstStatement *st_false;
} AstStIf;

/// Stucture representing a statement
typedef struct ast_statement {
    /// Statement type
    AstStatementType type;
    union {
        AstScope *st_scope;
        AstStIf *st_if;
        // ...
    };
} AstStatement;

/// Structure holding a scope with a statement list
typedef struct ast_scope {
    // TODO: Symtable
    // TODO: Statement list
} AstScope;

/// Structure holding a function
typedef struct ast_function {
    /// Body of the function
    AstExpression *body;
    // TODO: Metadata
} AstFunction;

/// Structure holding the whole program
typedef struct ast_program {
    // TODO: Symtable for global vars
    // TODO: FunctionList
} AstProgram;

#endif // !_AST_H_
