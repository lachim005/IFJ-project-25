#include "token.h"


typedef struct stack {
    size_t capacity;
    size_t top;
    Token *items;
} Stack;

Stack *stack_init(size_t capacity); 


void stack_destroy(Stack *stack);

bool stack_empty(Stack *stack);

bool stack_push(Stack *stack, Token item);

bool stack_pop(Stack *stack, Token *out_item);

bool stack_top(Stack *stack, Token *out_item);

bool stack_find_type(Stack *stack, Stack *out_stack, TokType type);
