/*
 * expr_parser.c
 * Implements a precedence parser for expressions
 *
 * Authors:
 * Vojtěch Borýsek (xborysv00)
 */

#include "expr_parser.h"

#define DEFAULT_CAPACITY 100


/**
 * Precedence table for operators
 */
char precedence_table[][19] = {
{ '>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', '>', '>', '>', '>', '>', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', '<', '<', '<', ' '},
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', '>', '>', '>', '>', ' ', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '<', ' '},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', ' ', '=', '<', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', ' ', ' ', '<', '>'},
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'},
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' ', ' ', '<', ' ', '<', ' '}
};


/**
 * Calculates the index in the precedence table for a given token type
 * Returns the index as an integer
 */
int calculate_table_idx(TokType type){
    switch (type)
    {
    case TOK_OP_PLUS: return 0; // +
    case TOK_OP_MINUS: return 1; // -
    case TOK_OP_MULT: return 2; // *
    case TOK_OP_DIV: return 3; // /
    case TOK_OP_GREATER: return 4; // >
    case TOK_OP_LESS: return 5; // <
    case TOK_OP_GREATER_EQ: return 6; // >=
    case TOK_OP_LESS_EQ: return 7; // <=
    case TOK_OP_EQ: return 8; // ==
    case TOK_OP_NOT_EQ: return 9; // !=
    case TOK_OP_AND: return 10; // &&
    case TOK_OP_OR: return 11; // ||
    case TOK_IDENTIFIER: case TOK_GLOBAL_VAR: case TOK_LIT_INT: 
    case TOK_LIT_DOUBLE: case TOK_LIT_STRING: return 12; // operands
    case TOK_LEFT_PAR: return 13; // (
    case TOK_RIGHT_PAR: return 14; // )
    case TOK_OP_IS: return 15; // is
    case TOK_OP_QUESTION_MARK: return 16; // ?
    case TOK_OP_COLON: return 17; // :
    case TOK_OP_NOT: return 18; // !
    case TOK_DOLLAR: return 19; // $
    default:
        break;
    }
}

ErrorCode parse_expression(Lexer *lexer) {
    Stack *expr_stack;
    Stack *op_stack;

    expr_stack = stack_init(100);
    op_stack = stack_init(100);

    Token token;
    token.type = TOK_DOLLAR;
    stack_push(expr_stack, token);

    while(1) {
        if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
            return LEXICAL_ERROR;
        } // read next token
        
        // token is from the lexer now
        int col = calculate_table_idx(token.type);

        stack_top(expr_stack, &token);
        // token is from the top of the stack now
        int row = calculate_table_idx(token.type);

        char relation = precedence_table[row][col];
        switch (relation)
        {
        case '<':
            shift(expr_stack, op_stack, token);
            break;
        case '>':
            reduce(expr_stack, op_stack, token);
            break;
        case '=':
            stack_push(expr_stack, token);
            break;
        case ' ':
            return SYNTACTIC_ERROR;
        default:
            break;
        }
    }
}

bool shift(Stack *expr_stack, Stack *op_stack, Token token) {
    if(stack_find_term(expr_stack, op_stack)){
        Token less = { .type = TOK_PREC_OPEN };
        stack_push(expr_stack, less); // inserts '<' after the topmost terminal
        push_whole_stack(op_stack, expr_stack); // pushes back all popped items
        stack_push(expr_stack, token); // pushes the current token onto the stack
        return true;
    }

    return false;

}

bool reduce(Stack *expr_stack, Stack *op_stack, Token token) {
    stack_find_term(expr_stack, op_stack);
    Token top_token;
    stack_top(op_stack, &top_token);
    switch(top_token.type){
        case TOK_OP_PLUS:
            reduce_plus(expr_stack, op_stack);
            break;
        case TOK_OP_MINUS:
            reduce_minus(expr_stack, op_stack);
            break;
        case TOK_OP_MULT:
            reduce_mult(expr_stack, op_stack);
            break;
        case TOK_OP_DIV:
            reduce_div(expr_stack, op_stack);
            break;
        case TOK_OP_GREATER:
            reduce_greater(expr_stack, op_stack);
            break;
        case TOK_OP_LESS:
            reduce_less(expr_stack, op_stack);
            break;
        case TOK_OP_GREATER_EQ:
            reduce_greater_eq(expr_stack, op_stack);
            break;
        case TOK_OP_LESS_EQ:
            reduce_less_eq(expr_stack, op_stack);
            break;
        case TOK_OP_EQ:
            reduce_eq(expr_stack, op_stack);
            break;
        case TOK_OP_NOT_EQ:
            reduce_not_eq(expr_stack, op_stack);
            break;
        case TOK_OP_AND:
            reduce_and(expr_stack, op_stack);
            break;
        case TOK_OP_OR:
            reduce_or(expr_stack, op_stack);
            break;
        case TOK_IDENTIFIER:
        case TOK_GLOBAL_VAR:
        case TOK_LIT_INT:
        case TOK_LIT_DOUBLE:
        case TOK_LIT_STRING:
            reduce_operand(expr_stack, op_stack);
            break;
        case TOK_LEFT_PAR:
        case TOK_RIGHT_PAR:
            reduce_parentheses(expr_stack, op_stack);
            break;
        case TOK_OP_IS:
            reduce_is(expr_stack, op_stack);
            break;
        case TOK_OP_QUESTION_MARK:
            reduce_question_mark(expr_stack, op_stack);
            break;
        case TOK_OP_COLON:
            reduce_colon(expr_stack, op_stack);
            break;
        case TOK_OP_NOT:
            reduce_not(expr_stack, op_stack);
            break;
        case TOK_DOLLAR:
            reduce_dollar(expr_stack, op_stack);
            break;
    }

}

ErrorCode reduce_plus(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for plus operator
}

ErrorCode reduce_minus(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for minus operator
} 

ErrorCode reduce_mult(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for multiplication operator
}

ErrorCode reduce_div(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for division operator
}

ErrorCode reduce_greater(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for greater than operator
}

ErrorCode reduce_less(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for less than operator
}

ErrorCode reduce_greater_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for greater than or equal operator
}

ErrorCode reduce_less_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for less than or equal operator
}

ErrorCode reduce_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for equality operator
}

ErrorCode reduce_not_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for not equal operator
}

ErrorCode reduce_and(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for logical AND operator
}

ErrorCode reduce_or(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for logical OR operator
}

ErrorCode reduce_operand(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for operands
}

ErrorCode reduce_parentheses(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for parentheses
}

ErrorCode reduce_is(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for 'is' operator
}

ErrorCode reduce_question_mark(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for question mark operator
}

ErrorCode reduce_colon(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for colon operator
}

ErrorCode reduce_not(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for logical NOT operator
}

ErrorCode reduce_dollar(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for dollar sign (end of expression)
}

