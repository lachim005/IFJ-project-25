/*
 * expr_parser.h
 * Defines a precedence parser for expressions
 *
 * IFJ project 2025
 * FIT VUT
 *
 * Authors:
 * Vojtěch Borýsek (xborysv00)
 */
#include "stack.h"
#include "lexer.h"
#include "token.h"
#include "error.h"
#include "ast.h"

ErrorCode reduce(Stack *expr_stack, Stack *op_stack);

ErrorCode shift(Stack *expr_stack, Stack *op_stack, Token token, Lexer *lexer);

int calculate_table_idx(TokType type);

void add_to_stack(Stack *expr_stack, Token token);

bool eol_possible(Token token);

ErrorCode parse_expression(Lexer *lexer, AstExpression **out_expr);

bool eol_possible(Token token);

ErrorCode reduce_binary(Stack *expr_stack, TokType op_type, AstExprType expr_type);

ErrorCode reduce_identifier(Stack *expr_stack, TokType id_type, AstExprType expr_type);

ErrorCode reduce_unary_prefix_op(Stack *expr_stack, TokType op_type, AstExprType expr_type);

ErrorCode reduce_ternary(Stack *expr_stack);

ErrorCode reduce_par(Stack *expr_stack);

ErrorCode reduce_literal(Stack *expr_stack, TokType lit_type, AstExprType expr_type);

ErrorCode reduce_data_type(Stack *expr_stack, TokType data_type);

ErrorCode reduce_function_call(Stack *expr_stack, Lexer *lexer, String *id);

ErrorCode reduce_buildtin_call(Stack *expr_stack, Lexer *lexer);
