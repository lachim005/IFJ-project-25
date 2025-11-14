#include "ast.h"
#include "symtable.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/// Creates a new AST expression node
AstExpression *ast_expr_create(AstExprType type, size_t child_count) {
    AstExpression *expr = malloc(sizeof(AstExpression));
    if (expr == NULL) {
        return NULL;
    }
    expr->params = malloc(child_count * sizeof(AstExpression *));
    if (expr->params == NULL) {
        free(expr);
        return NULL;
    }
    expr->type = type;
    expr->child_count = child_count;
    expr->assumed_type = DT_UNKNOWN;
    return expr;
}

/// Initializes a new AST 
AstStatement *ast_statement_init() {
    AstStatement *statement = malloc(sizeof(AstStatement));
    if (statement == NULL) {
        return NULL;
    }
    statement->type = ST_ROOT;
    
    // Allocate next element
    statement->next = malloc(sizeof(AstStatement));
    if (statement->next == NULL) {
        free(statement);
        return NULL;
    }
    statement->next->type = ST_END;
    statement->next->next = NULL;
    
    return statement;
}

/// Creates a new AST block
AstBlock *ast_block_create() {
    AstBlock *block = malloc(sizeof(AstBlock));
    if (block == NULL) {
        return NULL;
    }
    
    // Allocate first statement as EMPTY
    block->statements = malloc(sizeof(AstStatement));
    if (block->statements == NULL) {
        free(block);
        return NULL;
    }
    block->statements->type = ST_END;
    block->statements->next = NULL;
    
    return block;
}

/// Adds a if statement to the AST
bool ast_add_if_statement(AstStatement *statement, AstExpression *condition) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create AstIfStatement structure
    AstIfStatement *if_st = malloc(sizeof(AstIfStatement));
    if (if_st == NULL) {
        return false;
    }

    // Create true branch block
    AstBlock *true_branch = ast_block_create();
    if (true_branch == NULL) {
        free(if_st);
        return false;
    }

    // Fill AstIfStatement structure
    if_st->condition = condition;
    if_st->true_branch = true_branch;
    if_st->false_branch = NULL;

    // Set statement type and union
    statement->type = ST_IF;
    statement->if_st = if_st;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            free(true_branch);
            free(if_st);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds an else branch to an existing if statement
bool ast_add_else_branch(AstStatement *if_statement) {
    // Check if statement is NULL or not an IF statement
    if (if_statement == NULL || if_statement->type != ST_IF) {
        return false;
    }

    // Check if if_st structure exists
    if (if_statement->if_st == NULL) {
        return false;
    }

    // Check if else branch already exists
    if (if_statement->if_st->false_branch != NULL) {
        return false;
    }

    // Create false branch block
    AstBlock *false_branch = ast_block_create();
    if (false_branch == NULL) {
        return false;
    }

    // Set the else branch
    if_statement->if_st->false_branch = false_branch;

    return true;
}

/// Adds a while statement to the AST
bool ast_add_while_statement(AstStatement *statement, AstExpression *condition) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create AstWhileStatement structure
    AstWhileStatement *while_st = malloc(sizeof(AstWhileStatement));
    if (while_st == NULL) {
        return false;
    }

    // Create body block
    AstBlock *body = ast_block_create();
    if (body == NULL) {
        free(while_st);
        return false;
    }

    // Fill AstWhileStatement structure
    while_st->condition = condition;
    while_st->body = body;

    // Set statement type and union
    statement->type = ST_WHILE;
    statement->while_st = while_st;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            free(body);
            free(while_st);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a return statement to the AST
bool ast_add_return_statement(AstStatement *statement, AstExpression *return_expr) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Set statement type and expression
    statement->type = ST_RETURN;
    statement->return_expr = return_expr;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a local variable to the AST
bool ast_add_local_var(AstStatement *statement, char *name, AstExpression *expression) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create variable name as String
    String *var_name = str_init();
    if (var_name == NULL) {
        return false;
    }
    if (!str_append_string(var_name, name)) {
        str_free(&var_name);
        return false;
    }

    // Create AstVariable structure
    AstVariable *local_var = malloc(sizeof(AstVariable));
    if (local_var == NULL) {
        str_free(&var_name);
        return false;
    }

    // Fill AstVariable structure
    local_var->name = var_name;
    local_var->expression = expression;

    // Set statement type and union
    statement->type = ST_LOCAL_VAR;
    statement->local_var = local_var;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            str_free(&var_name);
            free(local_var);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a global variable to the AST
bool ast_add_global_var(AstStatement *statement, char *name, AstExpression *expression) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create variable name as String
    String *var_name = str_init();
    if (var_name == NULL) {
        return false;
    }
    if (!str_append_string(var_name, name)) {
        str_free(&var_name);
        return false;
    }

    // Create AstVariable structure
    AstVariable *global_var = malloc(sizeof(AstVariable));
    if (global_var == NULL) {
        str_free(&var_name);
        return false;
    }

    // Fill AstVariable structure
    global_var->name = var_name;
    global_var->expression = expression;

    // Set statement type and union
    statement->type = ST_GLOBAL_VAR;
    statement->global_var = global_var;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            str_free(&var_name);
            free(global_var);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a getter to the AST
bool ast_add_getter(AstStatement *statement, char *name, Symtable *symtable) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create getter name as String
    String *getter_name = str_init();
    if (getter_name == NULL) {
        return false;
    }
    if (!str_append_string(getter_name, name)) {
        str_free(&getter_name);
        return false;
    }
    if (!str_append_char(getter_name, '!')) {
        str_free(&getter_name);
        return false;
    }

    // Create AstGetter structure
    AstGetter *getter = malloc(sizeof(AstGetter));
    if (getter == NULL) {
        str_free(&getter_name);
        return false;
    }

    // Create body block
    AstBlock *body = ast_block_create();
    if (body == NULL) {
        str_free(&getter_name);
        free(getter);
        return false;
    }

    // Fill AstGetter structure
    getter->name = getter_name;
    getter->body = body;
    getter->symtable = symtable;

    // Set statement type and union
    statement->type = ST_GETTER;
    statement->getter = getter;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            str_free(&getter_name);
            free(body);
            free(getter);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a setter to the AST
bool ast_add_setter(AstStatement *statement, char *name, char *param_name, Symtable *symtable) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create setter name as String
    String *setter_name = str_init();
    if (setter_name == NULL) {
        return false;
    }
    if (!str_append_string(setter_name, name)) {
        str_free(&setter_name);
        return false;
    }
    if (!str_append_char(setter_name, '*')) {
        str_free(&setter_name);
        return false;
    }

    // Create parameter name as String
    String *setter_param_name = str_init();
    if (setter_param_name == NULL) {
        str_free(&setter_name);
        return false;
    }
    if (!str_append_string(setter_param_name, param_name)) {
        str_free(&setter_param_name);
        str_free(&setter_name);
        return false;
    }

    // Create AstSetter structure
    AstSetter *setter = malloc(sizeof(AstSetter));
    if (setter == NULL) {
        str_free(&setter_param_name);
        str_free(&setter_name);
        return false;
    }

    // Create body block
    AstBlock *body = ast_block_create();
    if (body == NULL) {
        str_free(&setter_param_name);
        str_free(&setter_name);
        free(setter);
        return false;
    }

    // Fill AstSetter structure
    setter->name = setter_name;
    setter->param_name = setter_param_name;
    setter->body = body;
    setter->symtable = symtable;

    // Set statement type and union
    statement->type = ST_SETTER;
    statement->setter = setter;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            str_free(&setter_param_name);
            str_free(&setter_name);
            free(body);
            free(setter);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a function to the AST program
bool ast_add_function(AstStatement *statement, char *name, size_t param_count, Symtable *symtable, String **param_names) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Create function name as String
    String *func_name = str_init();
    if (func_name == NULL) {
        return false;
    }
    if (!str_append_string(func_name, name)) {
        str_free(&func_name);
        return false;
    }

    // Create AstFunction structure
    AstFunction *function = malloc(sizeof(AstFunction));
    if (function == NULL) {
        str_free(&func_name);
        return false;
    }

    // Create function body block
    AstBlock *body = ast_block_create();
    if (body == NULL) {
        str_free(&func_name);
        free(function);
        return false;
    }

    // Fill AstFunction structure
    function->name = func_name;
    function->param_count = param_count;
    function->param_names = param_names;
    function->body = body;
    function->symtable = symtable;

    // Set statement type and union
    statement->type = ST_FUNCTION;
    statement->function = function;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            str_free(&func_name);
            free(body);
            free(function);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a setter call to the AST
bool ast_add_setter_call(AstStatement *statement, char *name, AstExpression *expression) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }  
    // Create setter call name as String
    String *setter_name = str_init();
    if (setter_name == NULL) {
        return false;
    }

    if (!str_append_string(setter_name, name)) {
        str_free(&setter_name);
        return false;
    } 

    // Create AstVariable structure for setter call
    AstVariable *setter_call = malloc(sizeof(AstVariable));
    if (setter_call == NULL) {
        str_free(&setter_name);
        return false;
    }

    // Fill AstVariable structure
    setter_call->name = setter_name;
    setter_call->expression = expression;

    // Set statement type and union
    statement->type = ST_SETTER_CALL;
    statement->setter_call = setter_call;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            str_free(&setter_name);
            free(setter_call);
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds a block to the AST
bool ast_add_block(AstStatement *statement) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Set statement type and union
    statement->type = ST_BLOCK;
    statement->block = ast_block_create();

    if (statement->block == NULL) {
        return false;
    }

    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Adds an inline expression to the AST
bool ast_add_inline_expression(AstStatement *statement, AstExpression *expression) {
    // Check if statement is NULL or not EMPTY
    if (statement == NULL || statement->type != ST_END) {
        return false;
    }

    // Set statement type and union
    statement->type = ST_EXPRESSION;
    statement->expression = expression;
    
    // Allocate next element if not already allocated
    if (statement->next == NULL) {
        statement->next = malloc(sizeof(AstStatement));
        if (statement->next == NULL) {
            return false;
        }
        statement->next->type = ST_END;
        statement->next->next = NULL;
    }

    return true;
}

/// Frees an AST expression recursively
void ast_expr_free(AstExpression *expr) {
    if (expr == NULL) {
        return;
    }

    // Free all child expressions
    for (size_t i = 0; i < expr->child_count; i++) {
        ast_expr_free(expr->params[i]);
    }
    free(expr->params);

    // Free string value if its a string expression
    if (expr->type == EX_STRING || expr->type == EX_ID || expr->type == EX_GLOBAL_ID ||
        expr->type == EX_FUN || expr->type == EX_BUILTIN_FUN || expr->type == EX_GETTER) {
        str_free(&expr->string_val);
    }

    free(expr);
}

/// Frees an AST statement recursively
void ast_statement_free(AstStatement *statement) {
    if (statement == NULL) {
        return;
    }

    // Free next statement in the list
    ast_statement_free(statement->next);

    // Free based on statement type
    switch (statement->type) {
        case ST_BLOCK:
            ast_block_free(statement->block);
            break;

        case ST_IF:
            if (statement->if_st != NULL) {
                ast_expr_free(statement->if_st->condition);
                ast_block_free(statement->if_st->true_branch);
                ast_block_free(statement->if_st->false_branch);
                free(statement->if_st);
            }
            break;

        case ST_WHILE:
            if (statement->while_st != NULL) {
                ast_expr_free(statement->while_st->condition);
                ast_block_free(statement->while_st->body);
                free(statement->while_st);
            }
            break;

        case ST_EXPRESSION:
            ast_expr_free(statement->expression);
            break;

        case ST_RETURN:
            ast_expr_free(statement->return_expr);
            break;

        case ST_LOCAL_VAR:
            if (statement->local_var != NULL) {
                str_free(&statement->local_var->name);
                ast_expr_free(statement->local_var->expression);
                free(statement->local_var);
            }
            break;

        case ST_GLOBAL_VAR:
            if (statement->global_var != NULL) {
                str_free(&statement->global_var->name);
                ast_expr_free(statement->global_var->expression);
                free(statement->global_var);
            }
            break;

        case ST_SETTER_CALL:
            if (statement->setter_call != NULL) {
                str_free(&statement->setter_call->name);
                ast_expr_free(statement->setter_call->expression);
                free(statement->setter_call);
            }
            break;

        case ST_FUNCTION:
            if (statement->function != NULL) {
                str_free(&statement->function->name);
                
                // Free parameter names
                if (statement->function->param_names != NULL) {
                    for (size_t i = 0; i < statement->function->param_count; i++) {
                        str_free(&statement->function->param_names[i]);
                    }
                    free(statement->function->param_names);
                }
                
                ast_block_free(statement->function->body);
                symtable_free(statement->function->symtable);
                free(statement->function);
            }
            break;

        case ST_GETTER:
            if (statement->getter != NULL) {
                str_free(&statement->getter->name);
                ast_block_free(statement->getter->body);
                symtable_free(statement->getter->symtable);
                free(statement->getter);
            }
            break;

        case ST_SETTER:
            if (statement->setter != NULL) {
                str_free(&statement->setter->name);
                str_free(&statement->setter->param_name);
                ast_block_free(statement->setter->body);
                symtable_free(statement->setter->symtable);
                free(statement->setter);
            }
            break;

        case ST_ROOT:
            // Root statement doesn't have additional data to free
            break;

        case ST_END:
            // Empty statement doesn't have additional data to free
            break;
        }

    free(statement);
}

/// Frees an AST block recursively
void ast_block_free(AstBlock *block) {
    if (block == NULL) {
        return;
    }

    ast_statement_free(block->statements);
    free(block);
}

/// Frees the entire AST starting from the root statement
void ast_free(AstStatement *root) {
    ast_statement_free(root);
}

// Helper function for printing with indentation
static void ast_print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

// Forward declarations
static void ast_print_block(AstBlock *block, int indent);
static void ast_print_statement(AstStatement *statement, int indent);
static void ast_print_expression(AstExpression *expr);

// Helper function for printing block
static void ast_print_block(AstBlock *block, int indent) {
    if (block == NULL) {
        return;
    }
    ast_print_statement(block->statements, indent);
}

// Print an expression tree recursively (simple readable form)
static void ast_print_expression(AstExpression *expr) {
    if (expr == NULL) {
        printf("(null expression)\n");
        return;
    }

    switch (expr->type) {
        case EX_ID:
            // For AST printing replace identifier text with the generic label
            printf("EXPRESSION\n");
            break;
        case EX_GLOBAL_ID:
            // For AST printing replace global identifier text with the generic label
            printf("EXPRESSION\n");
            break;
        case EX_STRING:
            // For AST printing replace string literal text with the generic label
            printf("EXPRESSION\n");
            break;
        case EX_INT:
            printf("%d\n", expr->int_val);
            break;
        case EX_DOUBLE:
            printf("%f\n", expr->double_val);
            break;
        case EX_BOOL:
            printf("%s\n", expr->bool_val ? "true" : "false");
            break;
        case EX_NULL:
            printf("null\n");
            break;
        default:
            // For non-leaf nodes (operators, function calls, etc.) don't print labels,
            // just recurse into children so only leaf values appear in output.
            break;
    }

    for (size_t i = 0; i < expr->child_count; i++) {
        ast_print_expression(expr->params[i]);
    }
}

// Helper function for printing statement
static void ast_print_statement(AstStatement *statement, int indent) {
    if (statement == NULL) {
        return;
    }

    ast_print_indent(indent);

    switch (statement->type) {
        case ST_ROOT:
            printf("ROOT\n");
            break;

        case ST_BLOCK:
            printf("BLOCK\n");
            if (statement->block != NULL) {
                ast_print_block(statement->block, indent + 1);
            }
            break;

        case ST_FUNCTION:
            if (statement->function != NULL) {
                printf("FUNCTION: %s (params: %zu)\n", 
                       statement->function->name ? statement->function->name->val : "?",
                       statement->function->param_count);
                if (statement->function->body != NULL) {
                    ast_print_block(statement->function->body, indent + 1);
                }
            } else {
                printf("FUNCTION: (null)\n");
            }
            break;

        case ST_GETTER:
            if (statement->getter != NULL) {
                printf("GETTER: %s\n", 
                       statement->getter->name ? statement->getter->name->val : "?");
                if (statement->getter->body != NULL) {
                    ast_print_block(statement->getter->body, indent + 1);
                }
            } else {
                printf("GETTER: (null)\n");
            }
            break;

        case ST_SETTER:
            if (statement->setter != NULL) {
                printf("SETTER: %s (param: %s)\n", 
                       statement->setter->name ? statement->setter->name->val : "?",
                       statement->setter->param_name ? statement->setter->param_name->val : "?");
                if (statement->setter->body != NULL) {
                    ast_print_block(statement->setter->body, indent + 1);
                }
            } else {
                printf("SETTER: (null)\n");
            }
            break;

        case ST_IF:
            if (statement->if_st != NULL) {
                printf("IF\n");
                ast_print_indent(indent + 1);
                printf("TRUE_BRANCH:\n");
                if (statement->if_st->true_branch != NULL) {
                    ast_print_block(statement->if_st->true_branch, indent + 2);
                }
                if (statement->if_st->false_branch != NULL) {
                    ast_print_indent(indent + 1);
                    printf("FALSE_BRANCH:\n");
                    ast_print_block(statement->if_st->false_branch, indent + 2);
                }
            } else {
                printf("IF: (null)\n");
            }
            break;

        case ST_WHILE:
            if (statement->while_st != NULL) {
                printf("WHILE\n");
                if (statement->while_st->body != NULL) {
                    ast_print_block(statement->while_st->body, indent + 1);
                }
            } else {
                printf("WHILE: (null)\n");
            }
            break;

        case ST_RETURN:
            printf("RETURN\n");
            break;

        case ST_SETTER_CALL:
            if (statement->setter_call != NULL) {
                if (statement->setter_call->expression != NULL) {
                    // print only the expression (no labels or additional text)
                    ast_print_expression(statement->setter_call->expression);
                }
            }
            break;

        case ST_LOCAL_VAR:
            if (statement->local_var != NULL) {
                printf("LOCAL_VAR: %s\n", 
                       statement->local_var->name ? statement->local_var->name->val : "?");
            } else {
                printf("LOCAL_VAR: (null)\n");
            }
            break;

        case ST_GLOBAL_VAR:
            if (statement->global_var != NULL) {
                printf("GLOBAL_VAR: %s\n", 
                       statement->global_var->name ? statement->global_var->name->val : "?");
            } else {
                printf("GLOBAL_VAR: (null)\n");
            }
            break;

        case ST_EXPRESSION:
            if (statement->expression != NULL) {
                ast_print_expression(statement->expression);
            }
            break;

        case ST_END:
            printf("END\n");
            break;

        default:
            printf("UNKNOWN_STATEMENT\n");
            break;
    }

    // Print next statement at same level (vedle, ne vnořené)
    if (statement->next != NULL) {
        ast_print_statement(statement->next, indent);
    }
}

/// Prints the entire AST tree to stdout with indentation
void ast_print(AstStatement *root) {
    printf("\n=== AST TREE ===\n");
    ast_print_statement(root, 0);
    printf("=== END AST ===\n\n");
}

