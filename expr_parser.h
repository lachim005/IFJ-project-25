#include "stack.h"
#include "lexer.h"
#include "token.h"
#include "error.h"

bool reduce(Stack *expr_stack, Stack *op_stack, Token token);

bool shift(Stack *expr_stack, Stack *op_stack, Token token);

ErrorCode parse_expression(Lexer *lexer);

void add_to_stack(Stack *expr_stack, Token token);

int calculate_table_idx(TokType type);