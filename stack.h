/*
 * stack.h
 * Defines a stack data structure for tokens
 *
 * IFJ project 2025
 * FIT VUT
 *
 * Authors:
 * Vojtěch Borýsek (xborysv00)
 */
#include "token.h"

#define STACK_INITIAL_CAPACITY 32


typedef struct stack {
    size_t capacity;
    size_t top;
    Token *items;
} Stack;

Stack *stack_init();

void stack_destroy(Stack *stack);

bool stack_empty(Stack *stack);

/**
 * Pushes an item onto the stack
 * If the stack is full, it will be resized to double its current capacity
 * Returns true if successful, false if memory allocation fails
 */
bool stack_push(Stack *stack, Token item);

/**
 * Pops the top item from the stack and stores it in out_item
 * Returns true if successful, false if the stack is empty
 */
bool stack_pop(Stack *stack);

/**
 * Retrieves the top item of the stack without removing it
 * Stores the item in out_item
 * Returns true if successful, false if the stack is empty
 */
bool stack_top(Stack *stack, Token *out_item);

/**
 * Searches for a specific token type in stack
 * It pops items from the original stack and pushes them onto out_stack until it finds the type
 * If found, it returns true and the searched type is on the top of the stack, otherwise false
 */
bool stack_find_type(Stack *stack, Stack *out_stack, TokType type);

/**
 * Searches for the topmost terminal in the stack
 * It pops items from the original stack and pushes them onto out_stack until it finds a terminal
 * If found, it returns true and the terminal is on the top of the stack, otherwise false
 */
bool stack_find_term(Stack *stack, Stack *out_stack);

/**
 * Pushes all items from src stack to dst stack
 * Returns true if successful, false otherwise
 */
bool push_whole_stack(Stack *src, Stack *dst);

bool stack_is_sequence_on_top(Stack *stack, TokType rule[], size_t rule_size);
