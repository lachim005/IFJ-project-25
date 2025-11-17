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
#include "lexer.h"
#include "token.h"

#define DEFAULT_CAPACITY 100


/**
 * Precedence table for operators
 */
char precedence_table[][20] = {
{ '>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //+
{ '>', '>', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //-
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //*
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, ///
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //>
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //<
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //>=
{ '<', '<', '<', '<', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //<=
{ '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '<', '<', '>', '<', '>', '>', '<', '>'}, //==
{ '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '<', '<', '>', '<', '>', '>', '<', '>'}, //!=
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '<', '<', '>', '<', '>', '>', '<', '>'}, //&&
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '<', '>', '<', '>', '>', '<', '>'}, //||
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', '>', '>', '>', '>', '>', '>'}, //id
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '=', '<', '<', '<', '<', ' '}, //(
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', ' ', ' ', '>', '>', '>', '>', ' ', '>'}, //)
{ '<', '<', '<', '<', '<', '<', '<', '<', '>', '>', '>', '>', '<', '<', '>', '>', '>', '>', '<', '>'}, //is
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', ' ', '=', '<', '>'}, //?
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '>', '<', '<', ' ', '<', '>'}, //:
{ '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '>', '<', '<', '>', '<', '>', '>', '<', '>'}, //!
{ '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', '<', ' ', '<', '<', ' ', '<', ' '}  //$
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
    case TOK_IDENTIFIER: case TOK_GLOBAL_VAR:
    case TOK_LIT_NUM: case TOK_LIT_STRING:
    case TOK_TYPE_NULL: case TOK_TYPE_NUM: case TOK_TYPE_STRING: case TOK_TYPE_BOOL:
    case TOK_KW_TRUE: case TOK_KW_FALSE: case TOK_KW_NULL:
    case TOK_KW_IFJ:
        return 12; // operands
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
    Token last_used_token = { .type = TOK_OP_PLUS }; // just some non-relevant initial value
    Token stack_token;

    if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
        // Read the first token
        stack_destroy(expr_stack);
        stack_destroy(op_stack);
        return LEXICAL_ERROR;
    }
    while(1) {
        stack_find_term(expr_stack, op_stack); // pops items until topmost terminal is found
        stack_top(expr_stack, &stack_token); // topmost terminal is now in token
        push_whole_stack(op_stack, expr_stack); // pushes back all popped items

        // Checking if we have reached the end
        if((stack_token.type == TOK_DOLLAR && token.type == TOK_RIGHT_PAR)
            || token.type == TOK_COMMA
            || token.type == TOK_RIGHT_BRACE
            || (token.type == TOK_EOL && !eol_possible(last_used_token))) {
            lexer_unget_token(lexer, &token);
            token.type = TOK_DOLLAR;
        }
        if (token.type == TOK_EOL) {
            if (lexer_get_token(lexer, &token) != ERR_LEX_OK) {
                stack_destroy(expr_stack);
                stack_destroy(op_stack);
                return LEXICAL_ERROR;
            }
            continue;
        }

        if(token.type == TOK_DOLLAR && stack_token.type == TOK_DOLLAR){
            break;
        } // end of expression

        if(token.type != TOK_EOL) {
            last_used_token = token;
        } // store last used token for EOL check

        int col = calculate_table_idx(token.type);
        int row = calculate_table_idx(stack_token.type);
        if (col < 0 || row < 0) {
            stack_destroy(expr_stack);
            stack_destroy(op_stack);
            return SYNTACTIC_ERROR;
        }

        char relation = precedence_table[row][col];

        ErrorCode result;

        switch (relation)
        {
        case '<':
            result = shift(expr_stack, op_stack, token, lexer);
            if (result != OK) {
                stack_destroy(expr_stack);
                stack_destroy(op_stack);
                return result;
            }
            if(last_used_token.type != TOK_DOLLAR){
                if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
                    stack_destroy(expr_stack);
                    stack_destroy(op_stack);
                    return LEXICAL_ERROR;
                } // read next token
            }
            break;
        case '>':
            result = reduce(expr_stack, op_stack);
            if (result != OK) {
                stack_destroy(expr_stack);
                stack_destroy(op_stack);
                return result;
            }
            break;
        case '=':
            if (!stack_push(expr_stack, token)) {
                stack_destroy(expr_stack);
                stack_destroy(op_stack);
                return INTERNAL_ERROR;
            }
            if(last_used_token.type != TOK_DOLLAR){
                if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
                    stack_destroy(expr_stack);
                    stack_destroy(op_stack);
                    return LEXICAL_ERROR;
                } // read next token
            }
            break;
        case ' ':
            stack_destroy(expr_stack);
            stack_destroy(op_stack);
            return SYNTACTIC_ERROR;
        default:
            break;
        }
    }

    if(expr_stack->top != 2 || !stack_empty(op_stack)){ // should contain only DOLLAR and E
        stack_destroy(expr_stack);
        stack_destroy(op_stack);
        return SYNTACTIC_ERROR;
    }

    stack_top(expr_stack, &token);
    stack_pop(expr_stack);
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

ErrorCode shift(Stack *expr_stack, Stack *op_stack, Token token, Lexer *lexer) {
    if (token.type == TOK_IDENTIFIER) {
        // Could be a function
        Token next_tok;
        if (lexer_get_token(lexer, &next_tok) != ERR_LEX_OK) return LEXICAL_ERROR;
        if (next_tok.type == TOK_LEFT_PAR) {
            // This, is a function!
            return reduce_function_call(expr_stack, lexer, token.string_val);
        } else {
            lexer_unget_token(lexer, &next_tok);
        }
    } else if (token.type == TOK_KW_IFJ) {
        return reduce_buildtin_call(expr_stack, lexer);
    }
    if(stack_find_term(expr_stack, op_stack)){
        Token less = { .type = TOK_PREC_OPEN };
        stack_push(expr_stack, less); // inserts '<' after the topmost terminal
        push_whole_stack(op_stack, expr_stack); // pushes back all popped items
        stack_push(expr_stack, token); // pushes the current token onto the stack
        return OK;
    }
    return INTERNAL_ERROR;
}

ErrorCode reduce(Stack *expr_stack, Stack *op_stack) {
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
            if (reduction_res != OK)
            {
                reduction_res = reduce_unary_prefix_op(expr_stack, TOK_OP_MINUS, EX_NEGATE);
            }
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
            reduction_res = reduce_identifier(expr_stack, TOK_IDENTIFIER, EX_ID);
            break;
        case TOK_GLOBAL_VAR:
            reduction_res = reduce_identifier(expr_stack, TOK_GLOBAL_VAR, EX_GLOBAL_ID);
            break;
        case TOK_LIT_NUM:
            reduction_res = reduce_literal(expr_stack, TOK_LIT_NUM, EX_DOUBLE);
            break;
        case TOK_LIT_STRING:
            reduction_res = reduce_literal(expr_stack, TOK_LIT_STRING, EX_STRING);
            break;
        case TOK_KW_TRUE:
            reduction_res = reduce_literal(expr_stack, TOK_KW_TRUE, EX_BOOL);
            break;
        case TOK_KW_FALSE:
            reduction_res = reduce_literal(expr_stack, TOK_KW_FALSE, EX_BOOL);
            break;
        case TOK_KW_NULL:
            reduction_res = reduce_literal(expr_stack, TOK_KW_NULL, EX_NULL);
            break;
        case TOK_RIGHT_PAR:
            reduction_res = reduce_par(expr_stack);
            break;
        case TOK_OP_IS:
            reduction_res = reduce_binary(expr_stack, TOK_OP_IS, EX_IS);
            break;
        case TOK_OP_QUESTION_MARK:
            break;
        case TOK_OP_COLON:
            reduction_res = reduce_ternary(expr_stack);
            break;
        case TOK_OP_NOT:
            reduction_res = reduce_unary_prefix_op(expr_stack, TOK_OP_NOT, EX_NOT);
            break;
        case TOK_TYPE_NULL:
        case TOK_TYPE_NUM:
        case TOK_TYPE_STRING:
        case TOK_TYPE_BOOL:
            reduction_res = reduce_data_type(expr_stack, top_token.type);
            break;
        default:
            return SYNTACTIC_ERROR;
    }
    return reduction_res;
}

#define RULE_SIZE(rule) sizeof(rule) / sizeof(TokType)

ErrorCode reduce_binary(Stack *expr_stack, TokType op_type, AstExprType expr_type) {
    TokType rule[] = { TOK_PREC_OPEN, TOK_E, op_type, TOK_E };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

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

ErrorCode reduce_identifier(Stack *expr_stack, TokType id_type, AstExprType expr_type) {
    TokType rule[] = { TOK_PREC_OPEN, id_type };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(expr_type, 0);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    stack_top(expr_stack, &top);
    stack_pop(expr_stack); // Pop id
    stack_pop(expr_stack); // Pop <
    expr->string_val = top.string_val;

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_unary_prefix_op(Stack *expr_stack, TokType op_type, AstExprType expr_type) {
    TokType rule[] = { TOK_PREC_OPEN, op_type, TOK_E };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(expr_type, 1);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    // Pop right side
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    expr->params[0] = top.expr_val;
    // Pop operator
    stack_pop(expr_stack);
    // Pop <
    stack_pop(expr_stack);

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_ternary(Stack *expr_stack) {
    TokType rule[] = { TOK_PREC_OPEN, TOK_E, TOK_OP_QUESTION_MARK, TOK_E, TOK_OP_COLON, TOK_E };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(EX_TERNARY, 3);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    // Pop false expr
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    expr->params[2] = top.expr_val;
    // Pop :
    stack_pop(expr_stack);
    // Pop true expr
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    expr->params[1] = top.expr_val;
    // Pop ?
    stack_pop(expr_stack);
    // Pop condition expr
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    expr->params[0] = top.expr_val;
    // Pop <
    stack_pop(expr_stack);

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_par(Stack *expr_stack) {
    TokType rule[] = { TOK_PREC_OPEN, TOK_LEFT_PAR, TOK_E, TOK_RIGHT_PAR };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

    Token top;
    // Pop )
    stack_pop(expr_stack);
    // Pop expr
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    // Pop (
    stack_pop(expr_stack);
    // Pop <
    stack_pop(expr_stack);

    // Push expr back
    if (!stack_push(expr_stack, top)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_literal(Stack *expr_stack, TokType lit_type, AstExprType expr_type) {
    TokType rule[] = { TOK_PREC_OPEN, lit_type };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(expr_type, 0);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    // Pop literal
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    // Pop <
    stack_pop(expr_stack);

    switch (expr_type) {
    case EX_STRING:
        expr->string_val = top.string_val; break;
    case EX_DOUBLE:
        expr->double_val = top.double_val; break;
    case EX_BOOL:
        expr->bool_val = top.type == TOK_KW_TRUE;
    default:
        break;
    }
    expr->val_known = true;

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_data_type(Stack *expr_stack, TokType data_type) {
    TokType rule[] = { TOK_PREC_OPEN, data_type };
    if (!stack_is_sequence_on_top(expr_stack, rule, RULE_SIZE(rule))) return SYNTACTIC_ERROR;

    AstExpression *expr = ast_expr_create(EX_DATA_TYPE, 0);
    if (expr_stack == NULL) return INTERNAL_ERROR;

    Token top;
    // Pop literal
    stack_top(expr_stack, &top);
    stack_pop(expr_stack);
    // Pop <
    stack_pop(expr_stack);

    switch (top.type) {
    case TOK_TYPE_NULL:
        expr->data_type = DT_NULL; break;
    case TOK_TYPE_NUM:
        expr->data_type = DT_NUM; break;
    case TOK_TYPE_STRING:
        expr->data_type = DT_STRING; break;
    case TOK_TYPE_BOOL:
        expr->data_type = DT_BOOL; break;
    default:
        return SYNTACTIC_ERROR; // Shouldn't happen
        break;
    }

    Token expr_tok = { .type = TOK_E, .expr_val = expr };
    if (!stack_push(expr_stack, expr_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_function_call(Stack *expr_stack, Lexer *lexer, String *id) {
    Token tok;
    do {
        if (lexer_get_token(lexer, &tok) != ERR_LEX_OK) return LEXICAL_ERROR;
    } while (tok.type == TOK_EOL);

    if (tok.type == TOK_RIGHT_PAR) {
        // No parameters
        AstExpression *fun_call = ast_expr_create(EX_FUN, 0);
        if (fun_call == NULL) return INTERNAL_ERROR;
        fun_call->string_val = id;
        Token fun_call_tok = { .type = TOK_E, .expr_val = fun_call };
        if (!stack_push(expr_stack, fun_call_tok)) return INTERNAL_ERROR;
        return OK;
    }
    lexer_unget_token(lexer, &tok);

    unsigned param_cnt = 0;

    while (1) {
        // Parse parameter
        AstExpression *expr;
        ErrorCode par_res = parse_expression(lexer, &expr);
        if (par_res != OK) return par_res;
        Token expr_token = { .type = TOK_E, .expr_val = expr };
        if (!stack_push(expr_stack, expr_token)) return INTERNAL_ERROR;
        param_cnt++;

        // Get next token
        do {
            if (lexer_get_token(lexer, &tok) != ERR_LEX_OK) return LEXICAL_ERROR;
        } while (tok.type == TOK_EOL);

        if (tok.type == TOK_RIGHT_PAR) {
            // End of function call
            break;
        }
        // More parameters, next has to be a comma
        if (tok.type != TOK_COMMA){
            token_free(&tok);
            return SYNTACTIC_ERROR;
        }
    }

    AstExpression *fun_call = ast_expr_create(EX_FUN, param_cnt);
    if (fun_call == NULL) return INTERNAL_ERROR;
    fun_call->string_val = id;

    // Adds params
    for (unsigned i = 1; i <= param_cnt; i++) {
        Token param_tok;
        stack_top(expr_stack, &param_tok);
        stack_pop(expr_stack);
        fun_call->params[param_cnt - i] = param_tok.expr_val;
    }

    Token fun_call_tok = { .type = TOK_E, .expr_val = fun_call };
    if (!stack_push(expr_stack, fun_call_tok)) return INTERNAL_ERROR;

    return OK;
}

ErrorCode reduce_buildtin_call(Stack *expr_stack, Lexer *lexer) {
    // We already covered the 'Ifj' keyword
    // Now we cover the dot
    Token tok;
    if (lexer_get_token(lexer, &tok) != ERR_LEX_OK) return LEXICAL_ERROR;
    if (tok.type != TOK_OP_DOT) {
        token_free(&tok);
        return SYNTACTIC_ERROR;
    }
    token_free(&tok);
    // Now the id
    Token id;
    do {
        if (lexer_get_token(lexer, &id) != ERR_LEX_OK) return LEXICAL_ERROR;
    } while (id.type == TOK_EOL);
    if (id.type != TOK_IDENTIFIER) {
        token_free(&id);
        return SYNTACTIC_ERROR;
    }
    // And (
    if (lexer_get_token(lexer, &tok) != ERR_LEX_OK) return LEXICAL_ERROR;
    if (tok.type != TOK_LEFT_PAR) {
        token_free(&tok);
        return SYNTACTIC_ERROR;
    }
    token_free(&tok);

    // This puts the function call to the top of the stack
    ErrorCode res = reduce_function_call(expr_stack, lexer, id.string_val);
    if (res != OK) {
        str_free(&id.string_val);
        return res;
    }

    // We have to change it's type to builtin
    stack_top(expr_stack, &tok);
    tok.expr_val->type = EX_BUILTIN_FUN;

    return OK;
}
