#include "code_generator.h"
#include "ast.h"
#include "error.h"
#include "string.h"
#include "symtable.h"
#include <stdio.h>
#include <string.h>

#define CG_ASSERT(cond) \
do {\
    if (!(cond)) { return INTERNAL_ERROR; }\
} while(false)

#ifdef CG_DEBUG
#define DEBUG_WRITE(output, ...) fprintf(output, __VA_ARGS__)
#else
#define DEBUG_WRITE(output, ...)
#endif

// Used for unique names for compiler variables and labels
unsigned internal_names_cntr = 0;

ErrorCode generate_compound_statement(FILE *output, AstBlock *st) {
    for (AstStatement *cur = st->statements; cur->type != ST_END; cur = cur->next) {
        CG_ASSERT(generate_statement(output, cur) == OK);
    }
    return OK;
}

ErrorCode generate_if_statement(FILE *output, AstIfStatement *st) {
    DataType cond_type = st->condition->assumed_type;
    if (cond_type == DT_NULL) {
        if (st->false_branch != NULL) {
            generate_compound_statement(output, st->false_branch);
        }
        return OK;
    }
    if (cond_type != DT_BOOL && cond_type != DT_UNKNOWN) {
        // If it is anything other than bool or null, it is always true
        generate_compound_statement(output, st->true_branch);
        return OK;
    }
    CG_ASSERT(generate_expression_evaluation(output, st->condition) == OK);
    unsigned expr_id = internal_names_cntr++;
    if (cond_type != DT_BOOL) {
        // If it is not bool, we have to check the data type
        fprintf(output, "POPS GF@&&inter1\n");
        if (cond_type == DT_UNKNOWN) {
            // We only have to check for null if the type is unknown, otherwise we know it isn't
            fprintf(output, "PUSHS GF@&&inter1\n"
                            "PUSHS nil@nil\n"
                            "JUMPIFEQS $&&if_false_branch%u\n",
                            expr_id);
        }
        // Check if it is the bool datatype
        fprintf(output, "PUSHS GF@&&inter1\n"
                        "TYPES\n"
                        "PUSHS string@bool\n"
                        "JUMPIFNEQS $&&if_true_branch%u\n"
                        "PUSHS GF@&&inter1\n",
                        expr_id);
    }
    // Check if false
    fprintf(output, "PUSHS bool@false\n"
                    "JUMPIFEQS $&&if_false_branch%u\n"
                    "LABEL $&&if_true_branch%u\n",
                    expr_id, expr_id);
    generate_compound_statement(output, st->true_branch);
    fprintf(output, "JUMP $&&if_end%u\n"
                    "LABEL $&&if_false_branch%u\n",
                    expr_id, expr_id);
    if (st->false_branch != NULL) {
        generate_compound_statement(output, st->false_branch);
    }
    fprintf(output, "LABEL $&&if_end%u\n", expr_id);
    return OK;
}

ErrorCode generate_while_statement(FILE *output, AstWhileStatement *st) {
    DataType cond_type = st->condition->assumed_type;
    if (cond_type == DT_NULL) {
        // Null will never execute, so we don't have to generate anything
        return OK;
    }
    unsigned expr_id = internal_names_cntr++;
    if (cond_type != DT_BOOL && cond_type != DT_UNKNOWN) {
        // If it is anything other than bool or null, it will be an infinite cycle
        fprintf(output, "LABEL $&&while_start%u\n", expr_id);
        generate_compound_statement(output, st->body);
        fprintf(output, "JUMP $&&while_start%u\n", expr_id);
        return OK;
    }
    fprintf(output, "LABEL $&&while_cond%u\n", expr_id);
    CG_ASSERT(generate_expression_evaluation(output, st->condition) == OK);
    if (cond_type != DT_BOOL) {
        // If it is not bool, we have to check the data type
        fprintf(output, "POPS GF@&&inter1\n");
        if (cond_type == DT_UNKNOWN) {
            // We only have to check for null if the type is unknown, otherwise we know it isn't
            fprintf(output, "PUSHS GF@&&inter1\n"
                            "PUSHS nil@nil\n"
                            "JUMPIFEQS $&&while_end%u\n",
                            expr_id);
        }
        // Check if it is the bool datatype
        fprintf(output, "PUSHS GF@&&inter1\n"
                        "TYPES\n"
                        "PUSHS string@bool\n"
                        "JUMPIFNEQS $&&while_start%u\n"
                        "PUSHS GF@&&inter1\n",
                        expr_id);
    }
    // Checks if false
    fprintf(output, "PUSHS bool@false\n"
                    "JUMPIFEQS $&&while_end%u\n"
                    "LABEL $&&while_start%u\n",
                    expr_id, expr_id);
    generate_compound_statement(output, st->body);
    fprintf(output, "JUMP $&&while_cond%u\n"
                    "LABEL $&&while_end%u\n",
                    expr_id, expr_id);
    return OK;
}

ErrorCode generate_function_call(FILE *output, AstExpression *call) {
    for (unsigned i = 0; i < call->child_count; i++) {
        CG_ASSERT(generate_expression_evaluation(output, call->params[i]) == OK);
    }
    fprintf(output, "CALL $%s$%zu\n", call->string_val->val, call->child_count);
    return OK;
}

ErrorCode generate_and_expr(FILE *output, AstExpression *ex) {
    unsigned expr_id = internal_names_cntr++;
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    // Short circuit
    fprintf(output, "PUSHS bool@false\n"
                    "JUMPIFEQS $&&and_short%u\n", expr_id);

    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    fprintf(output, "JUMP $&&and_end%u\n"
                    "LABEL $&&and_short%u\n"
                    "PUSHS bool@false\n"
                    "LABEL $&&and_end%u\n",
                    expr_id, expr_id, expr_id);
    return OK;
}

ErrorCode generate_or_expr(FILE *output, AstExpression *ex) {
    unsigned expr_id = internal_names_cntr++;
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    // Short circuit
    fprintf(output, "PUSHS bool@true\n"
                    "JUMPIFEQS $&&or_short%u\n", expr_id);

    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    fprintf(output, "JUMP $&&or_end%u\n"
                    "LABEL $&&or_short%u\n"
                    "PUSHS bool@true\n"
                    "LABEL $&&or_end%u\n",
                    expr_id, expr_id, expr_id);
    return OK;
}

ErrorCode generate_is_expr(FILE *output, AstExpression *ex) {
    unsigned expr_id = internal_names_cntr++;
    CG_ASSERT(ex->params[1]->type == EX_DATA_TYPE);
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    if (ex->params[1]->data_type == DT_NUM) {
        // If the type we are checking is Num, it could be either a float or an int,
        // so we convert ints into floats
        // &&inter1 - holds the value the type of which we are checking
        // &&inter2 - holds the type
        fprintf(output, "POPS GF@&&inter1\n"
                        "TYPE GF@&&inter2 GF@&&inter1\n"
                        "JUMPIFNEQ $&&is_int_conv%u GF@&&inter2 string@float\n"
                        // The type is int, so we can push true and jump to the end
                        "PUSHS bool@true\n"
                        "JUMP $&&is_end%u\n"
                        "LABEL $&&is_int_conv%u\n"
                        "PUSHS GF@&&inter1\n",
                        expr_id, expr_id, expr_id);
    }

    fprintf(output, "TYPES\n");

    char *desired_type = NULL;
    switch (ex->params[1]->data_type) {
    case DT_NULL:
        desired_type = "nil"; break;
    case DT_NUM:
        desired_type = "int"; break;
    case DT_STRING:
        desired_type = "string"; break;
    case DT_BOOL:
        desired_type = "bool"; break;
    default:
        return INTERNAL_ERROR;
    }
    fprintf(output, "PUSHS string@%s\n"
                    "EQS\n", desired_type);

    if (ex->params[1]->data_type == DT_NUM) {
        // We have to add the label that we will jump to in case there is a float
        fprintf(output, "LABEL $&&is_end%u\n", expr_id);
    }
    return OK;
}

ErrorCode generate_ternary_expr(FILE *output, AstExpression *ex) {
    unsigned expr_id = internal_names_cntr++;

    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    fprintf(output, "PUSHS bool@false\n"
                    "JUMPIFEQS $&&ternary_false%u\n",
                    expr_id);
    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    fprintf(output, "JUMP $&&ternary_end%u\n"
                    "LABEL $&&ternary_false%u\n",
                    expr_id, expr_id);
    CG_ASSERT(generate_expression_evaluation(output, ex->params[2]) == OK);
    fprintf(output, "LABEL $&&ternary_end%u\n", expr_id);

    internal_names_cntr++;
    return OK;
}

ErrorCode convert_string(char *input, String **out) {
    String *str = str_init();
    CG_ASSERT(str != NULL);
    for (unsigned i = 0; input[i] != '\0'; i++) {
        unsigned char c = input[i];
        if (c > 32 && c != 35 && c != 92) {
            // Normal characters - we can print them as they are
            CG_ASSERT(str_append_char(str, c));
            continue;
        }
        // Others - print their escape sequence
        char buf[4];
        CG_ASSERT(snprintf(buf, 4, "%03d", c) <= 4);
        CG_ASSERT(str_append_char(str, '\\'));
        CG_ASSERT(str_append_string(str, buf));
    }
    *out = str;
    return OK;
}

ErrorCode generate_builtin_function_call(FILE *output, AstExpression *ex) {
    // Push parameters
    for (unsigned par = 0; par < ex->child_count; par++) {
        CG_ASSERT(generate_expression_evaluation(output, ex->params[par]) == OK);
    }
    if (strcmp(ex->string_val->val, "write") == 0) {
        CG_ASSERT(ex->child_count == 1);
        fprintf(output, "POPS GF@&&inter1\n"
                        "WRITE GF@&&inter1\n"
                        "PUSHS nil@nil\n");
    }
    else if (strcmp(ex->string_val->val, "read_str") == 0) {
        CG_ASSERT(ex->child_count == 0);
        fprintf(output, "READ GF@&&inter1 string\n"
                        "PUSHS GF@&&inter1\n");
    }
    else if (strcmp(ex->string_val->val, "read_num") == 0) {
        CG_ASSERT(ex->child_count == 0);
        unsigned expr_id = internal_names_cntr++;
        fprintf(output, "READ GF@&&inter1 float\n"
                        "PUSHS GF@&&inter1\n"
                        "PUSHS GF@&&inter1\n"
                        // Convert to int if required
                        "ISINTS\n"
                        "PUSHS bool@false\n"
                        "JUMPIFEQS $&&read_num_end%u\n"
                        "FLOAT2INTS\n"
                        "LABEL $&&read_num_end%u\n",
                        expr_id, expr_id);
    }
    else if (strcmp(ex->string_val->val, "floor") == 0) {
        CG_ASSERT(ex->child_count == 1);
        fprintf(output, "FLOAT2INTS\n");
    }
    else if (strcmp(ex->string_val->val, "length") == 0) {
        CG_ASSERT(ex->child_count == 1);
        fprintf(output, "POPS GF@&&inter1\n"
                        "STRLEN GF@&&inter2 GF@&&inter1\n"
                        "PUSHS GF@&&inter2\n");
    }
    else if (strcmp(ex->string_val->val, "substring") == 0) {
        CG_ASSERT(ex->child_count == 3);
        // TODO:
        fprintf(output, "POPS GF@&&inter3\n"
                        "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "PUSHS string@TODO\n");
    }
    else if (strcmp(ex->string_val->val, "strcmp") == 0) {
        CG_ASSERT(ex->child_count == 2);
        // TODO:
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "PUSHS int@0\n");
    }
    else if (strcmp(ex->string_val->val, "ord") == 0) {
        CG_ASSERT(ex->child_count == 2);
        unsigned expr_id = internal_names_cntr++;
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "ISINT GF@&&inter3 GF@&&inter2\n"
                        "JUMPIFEQ $&&ifj_ord_int_checked%u GF@&&inter3 bool@true\n"
                        "EXIT int@26\n"
                        "LABEL $&&ifj_ord_int_checked%u\n"
                        "STRLEN GF@&&inter3 GF@&&inter1\n"
                        "PUSHS GF@&&inter3\n"
                        "PUSHS GF@&&inter2\n"
                        "GTS\n"
                        "PUSHS bool@true\n"
                        "JUMPIFNEQS $&&ifj_ord_err%u\n"
                        "PUSHS GF@&&inter1\n"
                        "PUSHS GF@&&inter2\n"
                        "STRI2INTS\n"
                        "JUMP $&&ifj_ord_end%u\n"
                        "LABEL $&&ifj_ord_err%u\n"
                        "PUSHS int@0\n"
                        "LABEL $&&ifj_ord_end%u\n",
                        expr_id, expr_id, expr_id, expr_id, expr_id, expr_id);
    }
    else if (strcmp(ex->string_val->val, "chr") == 0) {
        CG_ASSERT(ex->child_count == 1);
        unsigned expr_id = internal_names_cntr++;
        fprintf(output, "POPS GF@&&inter1\n"
                        "ISINT GF@&&inter2 GF@&&inter1\n"
                        "JUMPIFEQ $&&ifj_chr_int_checked%u GF@&&inter2 bool@true\n"
                        "EXIT int@26\n"
                        "LABEL $&&ifj_chr_int_checked%u\n"
                        "PUSHS GF@&&inter1\n"
                        "INT2CHARS\n",
                        expr_id, expr_id);
    }
    else {
        return INTERNAL_ERROR;
    }
    return OK;
}

ErrorCode generate_expression_evaluation(FILE *output, AstExpression *st) {
    String *str; // Used for string literals
    switch (st->type) {
    case EX_ID:
        fprintf(output, "PUSHS LF@%s\n", st->string_val->val);
        return OK;
    case EX_GLOBAL_ID:
        fprintf(output, "PUSHS GF@%s\n", st->string_val->val);
        return OK;
    case EX_GETTER:
        fprintf(output, "CALL $%s$0\n", st->string_val->val);
        return OK;
    case EX_FUN:
        return generate_function_call(output, st);
    case EX_INT:
        fprintf(output, "PUSHS int@%d\n", st->int_val);
        return OK;
    case EX_DOUBLE:
        fprintf(output, "PUSHS float@%a\n", st->double_val);
        return OK;
    case EX_BOOL:
        fprintf(output, "PUSHS bool@%s\n", st->bool_val ? "true" : "false");
        return OK;
    case EX_NULL:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_TERNARY:
        return generate_ternary_expr(output, st);
    case EX_NOT:
        CG_ASSERT(generate_expression_evaluation(output, st->params[0]) == OK);
        fprintf(output, "NOTS\n");
        return OK;
    case EX_IS:
        return generate_is_expr(output, st);
    case EX_STRING:
        CG_ASSERT(convert_string(st->string_val->val, &str) == OK);
        fprintf(output, "PUSHS string@%s\n", str->val);
        str_free(&str);
        return OK;
    case EX_NEGATE:
        fprintf(output, "PUSHS int@0\n");
        CG_ASSERT(generate_expression_evaluation(output, st->params[0]) == OK);
        fprintf(output, "SUBS\n");
        return OK;
    case EX_DATA_TYPE:
        return INTERNAL_ERROR;
    case EX_BUILTIN_FUN:
        CG_ASSERT(generate_builtin_function_call(output, st) == OK);
        return OK;
    case EX_AND:
        return generate_and_expr(output, st);
    case EX_OR:
        return generate_or_expr(output, st);
    default:
        break;
    }

    // The rest are just binary operators
    // TODO: type check
    CG_ASSERT(generate_expression_evaluation(output, st->params[0]) == OK);
    CG_ASSERT(generate_expression_evaluation(output, st->params[1]) == OK);
    switch (st->type) {
    case EX_ADD:
        fprintf(stdout, "ADDS\n"); break;
    case EX_SUB:
        fprintf(stdout, "SUBS\n"); break;
    case EX_MUL:
        fprintf(stdout, "MULS\n"); break;
    case EX_DIV:
        fprintf(stdout, "DIVS\n"); break;
    case EX_GREATER:
        fprintf(stdout, "GTS\n"); break;
    case EX_LESS:
        fprintf(stdout, "LTS\n"); break;
    case EX_GREATER_EQ:
        fprintf(stdout, "LTS\nNOTS\n"); break;
    case EX_LESS_EQ:
        fprintf(stdout, "GTS\nNOTS\n"); break;
    case EX_EQ:
        fprintf(stdout, "EQS\n"); break;
    case EX_NOT_EQ:
        fprintf(stdout, "EQS\nNOTS\n"); break;
    default:
        return INTERNAL_ERROR;
    }
    return OK;
}

ErrorCode generate_var_assignment(FILE *output, char *scope, AstVariable *st) {
    if (st->expression == NULL) return OK;
    CG_ASSERT(generate_expression_evaluation(output, st->expression) == OK);
    fprintf(output, "POPS %s@%s\n", scope, st->name->val);
    return OK;
}

ErrorCode generate_setter_assignment(FILE *output, AstVariable *st) {
    CG_ASSERT(generate_expression_evaluation(output, st->expression) == OK);
    fprintf(output, "CALL $%s*$1\n"
                    "POPS GF@&&inter1\n", st->name->val);
    return OK;
}

ErrorCode generate_return_statement(FILE *output, AstExpression *expr) {
    if (expr != NULL) {
        CG_ASSERT(generate_expression_evaluation(output, expr) == OK);
    } else {
        fprintf(output, "PUSHS nil@nil\n");
    }
    fprintf(output, "POPFRAME\nRETURN\n");
    return OK;
}

ErrorCode generate_statement(FILE *output, AstStatement *st) {
    switch (st->type) {
    case ST_BLOCK:
        return generate_compound_statement(output, st->block);
    case ST_IF:
        return generate_if_statement(output, st->if_st);
    case ST_WHILE:
        return generate_while_statement(output, st->while_st);
    case ST_RETURN:
        return generate_return_statement(output, st->return_expr);
    case ST_LOCAL_VAR:
        return generate_var_assignment(output, "LF", st->local_var);
    case ST_GLOBAL_VAR:
        return generate_var_assignment(output, "GF", st->global_var);
    case ST_SETTER_CALL:
        return generate_setter_assignment(output, st->setter_call);
    case ST_EXPRESSION:
        CG_ASSERT(generate_expression_evaluation(output, st->expression) == OK);
        fprintf(output, "POPS GF@&&inter1\n");
        return OK;
    default:
        // return INTERNAL_ERROR;
        return OK;
    }
    return OK;
}

void declare_global_var(SymtableItem *item, void *par) {
    if (item->type != SYM_GLOBAL_VAR) return;

    fprintf((FILE*)par, "DEFVAR GF@%s\n"
                        "MOVE GF@%s nil@nil\n",
                        item->key, item->key);
}

void declare_local_var(SymtableItem *item, void *par) {
    if (item->type != SYM_VAR) return;

    fprintf((FILE*)par, "DEFVAR LF@%s\n", item->key);
}

ErrorCode store_function_parameters(FILE *output, AstFunction *fun) {
    DEBUG_WRITE(output, "\n# Store parameters into variables\n");
    for (size_t i = fun->param_count; i > 0; i--) {
        // We find the var in the symtable to get the name with the correct scope id
        SymtableItem *var;
        CG_ASSERT(find_local_var(fun->symtable, fun->param_names[i-1]->val, &var));
        CG_ASSERT(var != NULL);
        fprintf(output, "POPS LF@%s\n", var->key);
    }
    return OK;
}

ErrorCode define_function(FILE *output, AstFunction *fun) {
    // Function label
    DEBUG_WRITE(output, "\n\n################\n""# DEFINITION OF FUNCTION '%s' with %zu parameters\n################\n", fun->name->val, fun->param_count);
    fprintf(output, "LABEL $%s$%zu\n", fun->name->val, fun->param_count);

    fprintf(output, "CREATEFRAME\n"
                    "PUSHFRAME\n");

    // Declare local variables
    DEBUG_WRITE(output, "\n# Local variables\n");
    symtable_foreach(fun->symtable, declare_local_var, output);

    // Store arguments into local variables
    CG_ASSERT(store_function_parameters(output, fun) == OK);

    // Generate code for statements
    DEBUG_WRITE(output, "\n# Function body\n");
    for (AstStatement *cur = fun->body->statements; cur->type != ST_END; cur = cur->next) {
        CG_ASSERT(generate_statement(output, cur) == OK);
        DEBUG_WRITE(output, "#---------\n");
    }

    // Default return
    DEBUG_WRITE(output, "\n# Default return value\n");
    fprintf(output, "POPFRAME\n"
                    "PUSHS nil@nil\n"
                    "RETURN\n");

    return OK;
}

void write_runtime(FILE *output) {
    DEBUG_WRITE(output, "\n\n################\n# RUNTIME\n################\n");

    // Main call
    fprintf(output, "CALL $main$0\n"
                    "EXIT int@0\n");
}

ErrorCode generate_code(FILE *output, AstStatement *root, Symtable *global_symtable) {
    // IFJcode Header
    fprintf(output, ".IFJcode25\n");

    // Declare global variables
    DEBUG_WRITE(output, "\n\n# GLOBAL VARS DECLARATION\n");
    // Declares some compiler variables
    fprintf(output, "DEFVAR GF@&&inter1\n"
                    "DEFVAR GF@&&inter2\n"
                    "DEFVAR GF@&&inter3\n");
    symtable_foreach(global_symtable, declare_global_var, output);

    // Runtime
    write_runtime(output);

    // Defines functions
    for (AstStatement *cur = root->next; cur->type != ST_END; cur = cur->next) {
        // We define some variables that we need to convert getters and setters into functions
        // and generate the code for them the same way
        AstGetter *g;
        AstSetter *s;
        AstFunction f;
        String *setter_par;
        switch (cur->type) {
            case ST_FUNCTION:
                CG_ASSERT(define_function(output, cur->function) == OK);
                break;
            case ST_GETTER:
                // Converting getter into a function
                g = cur->getter;
                f.name = g->name;
                f.param_count = 0;
                f.param_names = NULL;
                f.body = g->body;
                f.symtable = g->symtable;
                CG_ASSERT(define_function(output, &f) == OK);
                break;
            case ST_SETTER:
                // Converting setter into a function
                s = cur->setter;
                f.name = s->name;
                f.param_count = 1;
                setter_par = s->param_name;
                f.param_names = &setter_par;
                f.body = s->body;
                f.symtable = s->symtable;
                CG_ASSERT(define_function(output, &f) == OK);
                break;
            default:
                return INTERNAL_ERROR;
        }
    }

    return OK;
}
