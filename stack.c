#include <stdio.h>
#include <stdlib.h>
#include "stack.h"

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
bool stack_pop(Stack *stack, Token *out_item) {
    if (stack_empty(stack)) {
        return false;
    }

    stack->top--;
    *out_item = stack->items[stack->top];

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

    *out_item = stack->items[stack->top];
    return true;
}

/**
 * Searches for a specific token type in stack
 * It pops items from the original stack and pushes them onto out_stack until it finds the type
 * If found, it returns true and the searched type is on the top of the stack, otherwise false
 */
bool stack_find_type(Stack *stack, Stack *out_stack, TokType type) {
    for (size_t i = stack->top; i > 0; i--) {
        if (stack->items[i - 1].type != type) {
            Token temp;
            stack_pop(stack, &temp);
            stack_push(out_stack, temp);
        }
        else if (stack->items[i - 1].type == type) {
            return true;
        }
    }
    return false;
}
