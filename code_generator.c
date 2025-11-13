#include "code_generator.h"
#include "ast.h"
#include "error.h"
#include "symtable.h"
#include <stdio.h>

#define CG_ASSERT(cond) \
do {\
    if (!(cond)) { return INTERNAL_ERROR; }\
} while(false)

#ifdef CG_DEBUG
#define DEBUG_WRITE(output, ...) fprintf(output, __VA_ARGS__)
#else
#define DEBUG_WRITE(output, ...)
#endif

ErrorCode generate_compound_statement(FILE *output, AstBlock *st) {
    for (AstStatement *cur = st->statements; cur->type != ST_END; cur = cur->next) {
        CG_ASSERT(generate_statement(output, cur) == OK);
    }
    return OK;
}

ErrorCode generate_function_call(FILE *output, AstExpression *call) {
    for (unsigned i = 0; i < call->child_count; i++) {
        CG_ASSERT(generate_expression_evaluation(output, call->params[i]) == OK);
    }
    fprintf(output, "CALL $%s$%zu\n", call->string_val->val, call->child_count);
    return OK;
}

ErrorCode generate_expression_evaluation(FILE *output, AstExpression *st) {
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
        fprintf(output, "PUSHS float@%lf\n", st->double_val);
        return OK;
    case EX_BOOL:
        fprintf(output, "PUSHS bool@%s\n", st->bool_val ? "true" : "false");
        return OK;
    case EX_NULL:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_TERNARY:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_NOT:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_IS:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_STRING:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_NEGATE:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_DATA_TYPE:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_BUILTIN_FUN:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_AND:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    case EX_OR:
        fprintf(output, "PUSHS nil@nil\n");
        return OK;
    default:
        break;
    }

    // The rest are just binary operators
    // TODO: type check
    generate_expression_evaluation(output, st->params[0]);
    generate_expression_evaluation(output, st->params[1]);
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
    CG_ASSERT(generate_expression_evaluation(output, st->expression) == OK);
    fprintf(output, "POPS %s@%s\n", scope, st->name->val);
    return OK;
}

ErrorCode generate_setter_assignment(FILE *output, AstVariable *st) {
    CG_ASSERT(generate_expression_evaluation(output, st->expression) == OK);
    fprintf(output, "CALL $%s*$1\n", st->name->val);
    return OK;
}

ErrorCode generate_return_statement(FILE *output, AstExpression *expr) {
    CG_ASSERT(generate_expression_evaluation(output, expr) == OK);
    fprintf(output, "POPFRAME\nRETURN\n");
    return OK;
}

ErrorCode generate_statement(FILE *output, AstStatement *st) {
    switch (st->type) {
    case ST_BLOCK:
        return generate_compound_statement(output, st->block);
    case ST_IF:
        break;
    case ST_WHILE:
        break;
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
        fprintf(output, "POPS GF@&&void\n");
        return OK;
    default:
        // return INTERNAL_ERROR;
        return OK;
    }
    return OK;
}

void declare_global_var(SymtableItem *item, void *par) {
    if (item->type != SYM_GLOBAL_VAR) return;

    fprintf((FILE*)par, "DEFVAR GF@%s\n", item->key);
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
    fprintf(output, "DEFVAR GF@&&void\n");
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
