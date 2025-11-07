/*
 * expr_parser.c
 * Implements a precedence parser for expressions
 *
 * Authors:
 * Vojtěch Borýsek (xborysv00)
 */

#include "expr_parser.h"
#include "ast.h"
#include "error.h"
#include "token.h"

#define DEFAULT_CAPACITY 100


/**
 * Precedence table for operators
 */
char precedence_table[][20] = {
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
        return -1;
        break;
    }
}

ErrorCode parse_expression(Lexer *lexer, AstExpression **out_expr) {
    Stack *expr_stack;
    Stack *op_stack;

    expr_stack = stack_init(100);
    op_stack = stack_init(100);

    Token token;
    token.type = TOK_DOLLAR;
    stack_push(expr_stack, token);
    Token last_used_token = { .type = TOK_KW_CLASS }; // just some non-relevant initial value
    Token stack_token;

    if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
        // Read the first token
        return LEXICAL_ERROR;
    }
    while(1) {
        stack_find_term(expr_stack, op_stack); // pops items until topmost terminal is found
        stack_top(expr_stack, &stack_token); // topmost terminal is now in token
        push_whole_stack(op_stack, expr_stack); // pushes back all popped items

        if(stack_token.type == TOK_DOLLAR && token.type == TOK_RIGHT_PAR){
            lexer_unget_token(lexer, token);
            token.type = TOK_DOLLAR;
        } // ')' acts as end of expression if there is no matching '('

        int col = calculate_table_idx(token.type);
        int row = calculate_table_idx(stack_token.type);

        if(token.type == TOK_DOLLAR && stack_token.type == TOK_DOLLAR){
            break;
        } // end of expression

        char relation = precedence_table[row][col];

        ErrorCode reduce_res;

        switch (relation)
        {
        case '<':
            if (!shift(expr_stack, op_stack, token)) return INTERNAL_ERROR;
            if(last_used_token.type != TOK_DOLLAR){
                if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
                    return LEXICAL_ERROR;
                } // read next token
            }
            break;
        case '>':
            reduce_res = reduce(expr_stack, op_stack);
            if (reduce_res != OK) return reduce_res;
            break;
        case '=':
            if (!stack_push(expr_stack, token)) return INTERNAL_ERROR;
            break;
        case ' ':
            return SYNTACTIC_ERROR;
        default:
            break;
        }
        if(token.type != TOK_EOL) {
            last_used_token = token;
        } // store last used token for EOL check
    }

    if(expr_stack->top != 2 || !stack_empty(op_stack)){ // should contain only DOLLAR and E
        return SYNTACTIC_ERROR;
    }

    stack_top(expr_stack, &token);
    *out_expr = token.expr_val;
    stack_destroy(expr_stack);
    stack_destroy(op_stack);
    return OK;
}

bool eol_possible(Token token){
    if(token.type >= TOK_OP_PLUS && token.type <= TOK_OP_IS){
        return true;
    } // EOL can follow operator and EOL
    return false;
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

bool reduce(Stack *expr_stack, Stack *op_stack) {
    stack_find_term(expr_stack, op_stack);
    Token top_token;
    stack_top(expr_stack, &top_token);
    push_whole_stack(op_stack, expr_stack);

    ErrorCode reduction_res = SYNTACTIC_ERROR;
    switch (top_token.type) {
        case TOK_OP_PLUS:
            reduction_res = reduce_binary(expr_stack, TOK_OP_PLUS, EX_ADD);
            break;
        case TOK_OP_MINUS:
            reduction_res = reduce_binary(expr_stack, TOK_OP_MINUS, EX_SUB);
            break;
        case TOK_OP_MULT:
            reduction_res = reduce_binary(expr_stack, TOK_OP_MULT, EX_MUL);
            break;
        case TOK_OP_DIV:
            reduction_res = reduce_binary(expr_stack, TOK_OP_DIV, EX_DIV);
            break;
        case TOK_OP_GREATER:
            reduction_res = reduce_binary(expr_stack, TOK_OP_GREATER, EX_GREATER);
            break;
        case TOK_OP_LESS:
            reduction_res = reduce_binary(expr_stack, TOK_OP_LESS, EX_LESS);
            break;
        case TOK_OP_GREATER_EQ:
            reduction_res = reduce_binary(expr_stack, TOK_OP_GREATER_EQ, EX_GREATER_EQ);
            break;
        case TOK_OP_LESS_EQ:
            reduction_res = reduce_binary(expr_stack, TOK_OP_LESS_EQ, EX_LESS_EQ);
            break;
        case TOK_OP_EQ:
            reduction_res = reduce_binary(expr_stack, TOK_OP_EQ, EX_EQ);
            break;
        case TOK_OP_NOT_EQ:
            reduction_res = reduce_binary(expr_stack, TOK_OP_NOT_EQ, EX_NOT_EQ);
            break;
        case TOK_OP_AND:
            reduction_res = reduce_binary(expr_stack, TOK_OP_AND, EX_AND);
            break;
        case TOK_OP_OR:
            reduction_res = reduce_binary(expr_stack, TOK_OP_OR, EX_OR);
            break;
        case TOK_IDENTIFIER:
            reduction_res = reduce_identifier(expr_stack);
            break;
        case TOK_GLOBAL_VAR:
        case TOK_LIT_INT:
        case TOK_LIT_DOUBLE:
        case TOK_LIT_STRING:
            break;
        case TOK_LEFT_PAR:
        case TOK_RIGHT_PAR:
            break;
        case TOK_OP_IS:
            reduction_res = reduce_binary(expr_stack, TOK_OP_IS, EX_IS);
            break;
        case TOK_OP_QUESTION_MARK:
            break;
        case TOK_OP_COLON:
            break;
        case TOK_OP_NOT:
            break;
        case TOK_DOLLAR:
            break;
        default:
            return SYNTACTIC_ERROR;
    }
    return reduction_res;
}

ErrorCode reduce_binary(Stack *expr_stack, TokType op_type, AstExprType expr_type) {
    TokType rule[] = { TOK_PREC_OPEN, TOK_E, op_type, TOK_E };
    if (!stack_is_sequence_on_top(expr_stack, rule, 4)) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(expr_type, 2);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    // Pop right side
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    expr->params[1] = top.expr_val;
    // Pop operator
    stack_pop(expr_stack);
    // Pop left side
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    expr->params[0] = top.expr_val;
    // Pop <
    stack_pop(expr_stack);

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_identifier(Stack *expr_stack) {
    TokType rule[] = { TOK_PREC_OPEN, TOK_IDENTIFIER };
    if (!stack_is_sequence_on_top(expr_stack, rule, 2)) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(EX_ID, 0);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    stack_top(expr_stack, &top);
    stack_pop(expr_stack); // Pop id
    stack_pop(expr_stack); // Pop <
    expr->id = top.string_val;

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}
