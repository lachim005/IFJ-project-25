/*
 * expr_parser.h
 * Defines a precedence parser for expressions
 *
 * Authors:
 * Vojtěch Borýsek (xborysv00)
 */

#include "stack.h"
#include "lexer.h"
#include "token.h"
#include "error.h"
#include "ast.h"

bool reduce(Stack *expr_stack, Stack *op_stack, Token token);

bool shift(Stack *expr_stack, Stack *op_stack, Token token);

int calculate_table_idx(TokType type);

void add_to_stack(Stack *expr_stack, Token token);

ErrorCode parse_expression(Lexer *lexer);

ErrorCode reduce_unary(Stack *op_stack, Token *operand, Token *operator);

ErrorCode reduce_binary(Stack *op_stack, Token *left, Token *right, Token *op);

ErrorCode reduce_ternary(Stack *op_stack, Token *cond, Token *true_expr, Token *false_expr, Token *question, Token *colon);

ErrorCode reduce_plus(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_minus(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_mult(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_div(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_greater(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_less(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_greater_eq(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_less_eq(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_eq(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_not_eq(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_and(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_or(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_operand(Stack *expr_stack, Stack *op_stack, Token top_token);

ErrorCode reduce_parentheses(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_is(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_question_mark(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_colon(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_not(Stack *expr_stack, Stack *op_stack);

ErrorCode reduce_dollar(Stack *expr_stack, Stack *op_stack);