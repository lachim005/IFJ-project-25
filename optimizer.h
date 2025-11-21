/*
 * optimizer.h
 * Header file for optimizer module
 *
 * IFJ project 2025
 * FIT VUT
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 */

#ifndef _OPTIMIZER_H_
#define _OPTIMIZER_H_

#include "ast.h"
#include "error.h"
#include "symtable.h"

/// Optimizes the given AST
ErrorCode optimize_ast(AstStatement *root, Symtable *globaltable);

/// Optimizes a given expression (evaluates if possible)
ErrorCode optimize_expression(AstExpression *expr, Symtable *globaltable, Symtable *localtable);

/// Optimizes a single statement
ErrorCode optimize_statement(AstStatement *statement, Symtable *globaltable, Symtable *localtable);

/// Optimizes a block of statements
ErrorCode optimize_block(AstBlock *block, Symtable *globaltable, Symtable *localtable);

/// Optimizes a root level statement list
ErrorCode optimize_root(AstStatement *statement, Symtable *globaltable, Symtable *localtable);

/// Updates symtable item with known value from expression
void update_symtable_value(SymtableItem *item, AstExpression *expr);

/// Clears a known value from a symtable item
void clear_symtable_item_value(SymtableItem *it, void* par);

/// Clears all known values from a symtable
void clear_symtable_values(Symtable *st);

#endif // !_OPTIMIZER_H_
