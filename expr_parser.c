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
        break;
    }
}
Lexer *lex;
ErrorCode parse_expression(Lexer *lexer, AstExpression **out_expr) {
    Stack *expr_stack;
    Stack *op_stack;
    lex = lexer;

    expr_stack = stack_init(100);
    op_stack = stack_init(100);

    Token token;
    token.type = TOK_DOLLAR;
    stack_push(expr_stack, token);
    Token last_used_token = { .type = TOK_KW_CLASS }; // just some non-relevant initial value
    Token stack_token;

    while(1) {
        if(last_used_token.type != TOK_DOLLAR){
            if(lexer_get_token(lexer, &token) != ERR_LEX_OK){
                return LEXICAL_ERROR;
            } // read next token
        }
        if(token.type != TOK_DOLLAR){
            if(token.type == TOK_EOL && eol_possible(last_used_token)){
                continue;
            } // skip EOLs in expressions if they can be there
            
            else if(token.type == TOK_EOL && !eol_possible(last_used_token)){
                lexer_unget_token(lexer, token);
                token.type = TOK_DOLLAR;
            } // unget token if EOL is not allowed so parser can handle end of expression, set token to DOLLAR to finish parsing
        }
        if((token.type > TOK_OP_IS && token.type < TOK_LEFT_PAR) || (token.type > TOK_RIGHT_PAR && token.type < TOK_DOLLAR)){
            lexer_unget_token(lexer, token);
            token.type = TOK_DOLLAR;
        } // invalid token in expression

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

        switch (relation)
        {
        case '<':
            shift(expr_stack, op_stack, token);
            break;
        case '>':
            reduce(expr_stack, op_stack);
            break;
        case '=':
            stack_push(expr_stack, token);
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
    if(token.type <= TOK_OP_PLUS && token.type <= TOK_OP_IS){
        return true;
    } // EOL muze byt za vsemi operatory a za dalsim EOL
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
    stack_top(op_stack, &top_token);
    stack_find_type(expr_stack, op_stack, TOK_PREC_OPEN); // gets the whole handle for reduction (on op_stack is for example 'E | : | E | ? | E', from the top)
    stack_pop(expr_stack); // remove the '<' marker from expr_stack
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
            reduce_operand(expr_stack, op_stack, top_token);
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

ErrorCode reduce_binary(Stack *op_stack, Token *left, Token *right, Token *operator) {
    // Implementation of reduce for binary operators
    stack_top(op_stack, left); // left.type == TOK_E
    stack_pop(op_stack);
    stack_top(op_stack, operator); // op.type == TOK_OP_MULT
    stack_pop(op_stack);
    stack_top(op_stack, right); // right.type == TOK_E
    stack_pop(op_stack);
}

ErrorCode reduce_plus(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for plus operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);

    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_PLUS){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for addition
    AstExpression *add_expr = ast_expr_create(EX_ADD, 2);
    add_expr->params[0] = left.expr_val;
    add_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = add_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E + E -> E

}

ErrorCode reduce_minus(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for minus operator
} 

ErrorCode reduce_mult(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for multiplication operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);

    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_MULT){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for multiplication
    AstExpression *mult_expr = ast_expr_create(EX_MUL, 2);
    mult_expr->params[0] = left.expr_val;
    mult_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = mult_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E * E -> E

}

ErrorCode reduce_div(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for division operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);

    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_DIV){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for division
    AstExpression *div_expr = ast_expr_create(EX_DIV, 2);
    div_expr->params[0] = left.expr_val;
    div_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = div_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E / E -> E

}

ErrorCode reduce_greater(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for greater than operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_GREATER){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for greater than
    AstExpression *g_expr = ast_expr_create(EX_GREATER, 2);
    g_expr->params[0] = left.expr_val;
    g_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = g_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E > E -> E
}

ErrorCode reduce_less(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for less than operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_LESS){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for less than
    AstExpression *l_expr = ast_expr_create(EX_LESS, 2);
    l_expr->params[0] = left.expr_val;
    l_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = l_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E < E -> E
}

ErrorCode reduce_greater_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for greater than or equal operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_GREATER_EQ){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for greater than or equal
    AstExpression *ge_expr = ast_expr_create(EX_GREATER_EQ, 2);
    ge_expr->params[0] = left.expr_val;
    ge_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = ge_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E >= E -> E
}

ErrorCode reduce_less_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for less than or equal operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_LESS_EQ){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for less than or equal
    AstExpression *le_expr = ast_expr_create(EX_LESS_EQ, 2);
    le_expr->params[0] = left.expr_val;
    le_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = le_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E <= E -> E
}

ErrorCode reduce_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for equality operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_EQ){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for equality
    AstExpression *eq_expr = ast_expr_create(EX_EQ, 2);
    eq_expr->params[0] = left.expr_val;
    eq_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = eq_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E == E -> E
}

ErrorCode reduce_not_eq(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for not equal operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_NOT_EQ){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for not equal
    AstExpression *ne_expr = ast_expr_create(EX_NOT_EQ, 2);
    ne_expr->params[0] = left.expr_val;
    ne_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = ne_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E != E -> E
}

ErrorCode reduce_and(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for logical AND operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_AND){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for logical AND
    AstExpression *and_expr = ast_expr_create(EX_AND, 2);
    and_expr->params[0] = left.expr_val;
    and_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = and_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E && E -> E
}

ErrorCode reduce_or(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for logical OR operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_GREATER_EQ){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for logical OR
    AstExpression *or_expr = ast_expr_create(EX_OR, 2);
    or_expr->params[0] = left.expr_val;
    or_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = or_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E || E -> E
}

ErrorCode reduce_operand(Stack *expr_stack, Stack *op_stack, Token top_token) {
    // Implementation of reduce for operands
    stack_pop(op_stack); // remove the operand from op_stack
    AstExpression *expr = NULL;
    if (top_token.type == TOK_IDENTIFIER || top_token.type == TOK_GLOBAL_VAR) {
        // Handle identifier or global variable, TODO: FUNCTIONS, rekursive calls
        String *id = top_token.string_val;
        expr = ast_expr_create(EX_ID, 0);
    } else if (top_token.type == TOK_LIT_INT) {
        // Handle integer literal
        expr = ast_expr_create(EX_INT, 0);
        expr->int_val = top_token.int_val;
    } else if (top_token.type == TOK_LIT_DOUBLE) {
        // Handle double literal
        expr = ast_expr_create(EX_DOUBLE, 0);
        expr->double_val = top_token.double_val;
    } else if (top_token.type == TOK_LIT_STRING) {
        // Handle string literal
        expr = ast_expr_create(EX_STRING, 0);
        expr->string_val = top_token.string_val;
    }
    else {
        return SYNTACTIC_ERROR; // unexpected token type
    }

    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = expr;
    E_token.pos_line = top_token.pos_line; 
    E_token.pos_char = top_token.pos_char;
    stack_push(expr_stack, E_token); // push the new expression node 
}

ErrorCode reduce_parentheses(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for parentheses
    Token inner;
    Token temp;
    stack_top(op_stack, &temp); // should be TOK_LEFT_PAR
    if(temp.type != TOK_LEFT_PAR){
        return SYNTACTIC_ERROR;
    }
    stack_pop(op_stack); // remove '(' from op_stack

    // Get the inner expression
    stack_top(op_stack, &inner);
    stack_pop(op_stack); // remove inner expression from op_stack
    if(inner.type != TOK_E){
        return SYNTACTIC_ERROR;
    }
    stack_push(expr_stack, inner); // push the new expression node, it stays the same without '()' (E) -> E

    stack_top(op_stack, &temp); // should be TOK_RIGHT_PAR
    if(temp.type != TOK_RIGHT_PAR){
        return SYNTACTIC_ERROR;
    }
}

ErrorCode reduce_is(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for 'is' operator
    Token right;
    Token left;
    Token op;
    // gets the whole handle for reduction
    reduce_binary(op_stack, &left, &right, &op);
    if(left.type != TOK_E || right.type != TOK_E || op.type != TOK_OP_IS){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for is
    AstExpression *is_expr = ast_expr_create(EX_IS, 2);
    is_expr->params[0] = left.expr_val;
    is_expr->params[1] = right.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = is_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E is E -> E
}

ErrorCode reduce_question_mark(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for question mark operator
    return SYNTACTIC_ERROR;
}

ErrorCode reduce_colon(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for colon operator
    Token false_expr;
    Token true_expr;
    Token cond;
    Token question;
    Token colon;
    // gets the whole handle for reduction
    reduce_ternary(op_stack, &cond, &true_expr, &false_expr, &question, &colon);
    if(cond.type != TOK_E || true_expr.type != TOK_E || false_expr.type != TOK_E ||
       question.type != TOK_OP_QUESTION_MARK || colon.type != TOK_OP_COLON){
        return SYNTACTIC_ERROR;
    }

    // create new AST node for ternary operator
    AstExpression *ternary_expr = ast_expr_create(EX_TERNARY, 3);
    ternary_expr->params[0] = cond.expr_val;
    ternary_expr->params[1] = true_expr.expr_val;
    ternary_expr->params[2] = false_expr.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = ternary_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces E ? E : E -> E
}

ErrorCode reduce_ternary(Stack *op_stack, 
    Token *cond, Token *true_expr, Token *false_expr, Token *question, Token *colon) {
    // Implementation of reduce for ternary operator
    stack_top(op_stack, cond); // cond.type == TOK_E
    stack_pop(op_stack);
    stack_top(op_stack, question); // question.type == TOK_OP_QUESTION_MARK
    stack_pop(op_stack);
    stack_top(op_stack, true_expr); // true_expr.type == TOK_E
    stack_pop(op_stack);
    stack_top(op_stack, colon); // colon.type == TOK_OP_COLON
    stack_pop(op_stack);
    stack_top(op_stack, false_expr); // false_expr.type == TOK_E
    stack_pop(op_stack);
}

ErrorCode reduce_not(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for logical NOT operator
    Token operand;
    Token operator;
    // gets the whole handle for reduction
    reduce_unary(op_stack, &operand, &operator);
    if(operand.type != TOK_E || operator.type != TOK_OP_NOT){
        return SYNTACTIC_ERROR;
    }
    // create new AST node for logical NOT
    AstExpression *not_expr = ast_expr_create(EX_NOT, 1);
    not_expr->params[0] = operand.expr_val;
    Token E_token;
    E_token.type = TOK_E;
    E_token.expr_val = not_expr;

    stack_push(expr_stack, E_token); // push the new expression node, reduces !E -> E
}

ErrorCode reduce_unary(Stack *op_stack, Token *operand, Token *operator) {
    // Implementation of reduce for unary operators
    stack_top(op_stack, operator); // operator.type == TOK_OP_NOT
    stack_pop(op_stack);
    stack_top(op_stack, operand); // operand.type == TOK_E
    stack_pop(op_stack);
}

ErrorCode reduce_dollar(Stack *expr_stack, Stack *op_stack) {
    // Implementation of reduce for dollar sign (end of expression)
    return SYNTACTIC_ERROR;
}

