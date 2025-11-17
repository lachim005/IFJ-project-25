/*
 * stack.c
 * Implements a stack data structure for tokens
 *
 * IFJ project 2025
 * FIT VUT
 *
 * Authors:
 * Vojtěch Borýsek (xborysv00)
 */
#include <stdio.h>
#include <stdlib.h>
#include "stack.h"
#include "token.h"

Stack *stack_init(size_t capacity) {
    Stack *stack = malloc(sizeof(Stack));
    if (stack == NULL) {
        return NULL;
    }
    stack->capacity = capacity;
    stack->top = 0;
    stack->items = malloc(capacity * sizeof(Token));
    if (stack->items == NULL) {
        free(stack);
        return NULL;
    }
    return stack;
}

void stack_destroy(Stack *stack) {
    for (unsigned i = 0; i < stack->top; i++) {
        token_free(stack->items + i);
    }
    free(stack->items);
    free(stack);
}

bool stack_empty(Stack *stack) {
    return stack->top == 0;
}

/**
 * Pushes an item onto the stack
 * If the stack is full, it will be resized to double its current capacity
 * Returns true if successful, false if memory allocation fails
 */
bool stack_push(Stack *stack, Token item) {
    if (stack->top >= stack->capacity) {
        size_t new_capacity = stack->capacity * 2;
        Token *new_items = realloc(stack->items, new_capacity * sizeof(Token));
        if (new_items == NULL) {
            return false;
        }
        stack->items = new_items;
        stack->capacity = new_capacity;
    }

    stack->items[stack->top] = item;
    stack->top++;

    return true;
}

/**
 * Pops the top item from the stack and stores it in out_item
 * Returns true if successful, false if the stack is empty
 */
bool stack_pop(Stack *stack) {
    if (stack_empty(stack)) {
        return false;
    }
    stack->top--;
    return true;
}

/**
 * Retrieves the top item of the stack without removing it
 * Stores the item in out_item
 * Returns true if successful, false if the stack is empty
 */
bool stack_top(Stack *stack, Token *out_item) {
    if (stack_empty(stack)) {
        return false;
    }

    *out_item = stack->items[stack->top-1];
    return true;
}

/**
 * Searches for a specific token type in stack
 * It pops items from the original stack and pushes them onto out_stack until it finds the type
 * If found, it returns true and the searched type is on the top of the stack, otherwise false
 */
bool stack_find_type(Stack *stack, Stack *out_stack, TokType type) {
    for (size_t i = stack->top; i > 0; i--) {
        Token temp;
        stack_top(stack, &temp);
        if (temp.type != type) {
            stack_pop(stack);
            stack_push(out_stack, temp);
        }
        else if (temp.type == type) {
            return true;
        }
    }
    return false;
}

/**
 * Searches for the topmost terminal in the stack
 * It pops items from the original stack and pushes them onto out_stack until it finds a terminal
 * If found, it returns true and the terminal is on the top of the stack, otherwise false
 */
bool stack_find_term(Stack *stack, Stack *out_stack) {
    for (size_t i = stack->top; i > 0; i--) {
        Token temp;
        stack_top(stack, &temp);
        if (temp.type == TOK_E || temp.type == TOK_PREC_OPEN) {
            stack_pop(stack);
            stack_push(out_stack, temp);
        }
        else {
            return true; // found terminal, currently on top of the stack
        }
    }
    return false;
}

/**
 * Pushes all items from src stack to dst stack
 * Returns true if successful, false otherwise
 */
bool push_whole_stack(Stack *src, Stack *dst) {
    for (size_t i = src->top; i > 0; i--) {
        if(stack_empty(src)){
            return true;
        }
        Token temp;
        stack_top(src, &temp);
        stack_push(dst, temp);
        stack_pop(src);
    }
    return false;
}

bool stack_is_sequence_on_top(Stack *stack, TokType rule[], size_t rule_size) {
    if (rule_size > stack->top) return false;
    for (size_t i = 1; i <= rule_size; i++) {
        unsigned stack_index = stack->top - i;
        if (stack->items[stack_index].type != rule[rule_size - i]) return false;
    }
    return true;
}
