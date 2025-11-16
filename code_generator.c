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

/// Generates code which checks if the value in a given var is the desired type
/// and exits with 26 if not
void generate_var_type_check(FILE *output, char *var, char *type) {
    unsigned expr_id = internal_names_cntr++;
    fprintf(output, "PUSHS %s\n"
                    "TYPES\n"
                    "PUSHS string@%s\n"
                    "JUMPIFEQS $type_check_valid%u\n"
                    "EXIT int@26\n"
                    "LABEL $type_check_valid%u\n" ,
                    var, type, expr_id, expr_id);
}
/// Generates code which checks if the value in a given var (assumes float) is an int and converts
/// to it and exits with 26 if not
void generate_var_int_check(FILE *output, char *var) {
    unsigned expr_id = internal_names_cntr++;
    fprintf(output, "PUSHS %s\n"
                    "ISINTS\n"
                    "PUSHS bool@true\n"
                    "JUMPIFEQS $int_check_valid%u\n"
                    "EXIT int@26\n"
                    "LABEL $int_check_valid%u\n",
                    var, expr_id, expr_id);
}
/// Generates code which checks if the value on the top of the stack is the desired type
/// and exits with 26 if not
/// Uses inter5
void generate_stack_type_check(FILE *output, char *type) {
    fprintf(output, "POPS GF@&&inter5\n");
    generate_var_type_check(output, "GF@&&inter5", type);
    fprintf(output, "PUSHS GF@&&inter5\n");
}

ErrorCode generate_compound_statement(FILE *output, AstBlock *st) {
    DEBUG_WRITE(output, "#v v v v v BLOCK v v v v v\n");
    for (AstStatement *cur = st->statements; cur->type != ST_END; cur = cur->next) {
        CG_ASSERT(generate_statement(output, cur) == OK);
        DEBUG_WRITE(output, "#------------\n");
        if (cur->type == ST_RETURN) {
            DEBUG_WRITE(output, "# SKIPPING DEAD CODE AFTER RETURN\n");
            break;
        }
    }
    DEBUG_WRITE(output, "#^ ^ ^ ^ ^ BLOCK ^ ^ ^ ^ ^\n");
    return OK;
}

/// Helper that returns true if an expression contains any variables
bool has_vars(AstExpression *ex) {
    if (ex->type == EX_ID || ex->type == EX_GLOBAL_ID || ex->type == EX_GETTER || ex->type == EX_FUN) {
        return true;
    }
    for (unsigned i = 0; i < ex->child_count; i++) {
        if (has_vars(ex->params[i])) return true;
    }
    return false;
}

ErrorCode generate_if_statement(FILE *output, AstIfStatement *st) {
    DataType cond_type = st->condition->assumed_type;
    if (cond_type == DT_NULL ||
        (cond_type == DT_BOOL && st->condition->val_known && !st->condition->bool_val)) {
        if (st->false_branch != NULL) {
            return generate_compound_statement(output, st->false_branch);
        }
        return OK;
    }
    if ((cond_type != DT_BOOL && cond_type != DT_UNKNOWN) ||
        (cond_type == DT_BOOL && st->condition->val_known && st->condition->bool_val)) {
        // If it is anything other than bool or null, it is always true
        return generate_compound_statement(output, st->true_branch);
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
    AstExpression *cond = st->condition;
    DataType cond_type = cond->assumed_type;
    if ((cond_type == DT_NULL) ||
        (cond_type == DT_BOOL && cond->val_known && !cond->bool_val && !has_vars(cond))) {
        // Null will never execute, so we don't have to generate anything
        return OK;
    }
    unsigned expr_id = internal_names_cntr++;
    if ((cond_type != DT_BOOL && cond_type != DT_UNKNOWN) ||
        (cond_type == DT_BOOL && cond->val_known && cond->bool_val)) {
        // If it is anything other than bool or null, it will be an infinite cycle
        fprintf(output, "LABEL $&&while_start%u\n", expr_id);
        generate_compound_statement(output, st->body);
        fprintf(output, "JUMP $&&while_start%u\n", expr_id);
        return OK;
    }
    fprintf(output, "LABEL $&&while_cond%u\n", expr_id);
    CG_ASSERT(generate_expression_evaluation(output, cond) == OK);
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
    CG_ASSERT(ex->params[1]->type == EX_DATA_TYPE);
    DataType expr_type = ex->params[0]->assumed_type;
    DataType checked_type = ex->params[1]->data_type;
    if (expr_type == checked_type) {
        // Types are the same, we can just push true
        fprintf(output, "PUSHS bool@true\n");
        return OK;
    }
    if (expr_type != DT_UNKNOWN) {
        // The type isn't uknown and it isn't the same, so we can push false
        fprintf(output, "PUSHS bool@false\n");
        return OK;
    }
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);

    fprintf(output, "TYPES\n");

    char *desired_type = NULL;
    switch (ex->params[1]->data_type) {
    case DT_NULL:
        desired_type = "nil"; break;
    case DT_NUM:
        desired_type = "float"; break;
    case DT_STRING:
        desired_type = "string"; break;
    case DT_BOOL:
        desired_type = "bool"; break;
    default:
        return INTERNAL_ERROR;
    }
    fprintf(output, "PUSHS string@%s\n"
                    "EQS\n", desired_type);

    return OK;
}

ErrorCode generate_ternary_expr(FILE *output, AstExpression *ex) {
    unsigned expr_id = internal_names_cntr++;
    AstExpression *cond = ex->params[0];
    DataType cond_type = cond->assumed_type;
    if (cond_type == DT_NULL ||
        (cond_type == DT_BOOL && cond->val_known && !cond->bool_val)) {
        // We know it's false
        return generate_expression_evaluation(output, ex->params[2]);
    }
    if ((cond_type != DT_BOOL && cond_type != DT_UNKNOWN) ||
        (cond_type == DT_BOOL && cond->val_known && cond->bool_val)) {
        // We know it's true
        return generate_expression_evaluation(output, ex->params[1]);
    }

    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    if (cond_type != DT_BOOL) {
        // If it is not bool, we have to check the data type
        fprintf(output, "POPS GF@&&inter1\n");
        if (cond_type == DT_UNKNOWN) {
            // We only have to check for null if the type is unknown, otherwise we know it isn't
            fprintf(output, "PUSHS GF@&&inter1\n"
                            "PUSHS nil@nil\n"
                            "JUMPIFEQS $&&ternary_false%u\n",
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
                    "JUMPIFEQS $&&ternary_false%u\n"
                    "LABEL $&&ternary_true%u\n",
                    expr_id, expr_id);
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

/// Generates code for Ifj.str
ErrorCode generate_builtin_str(FILE *output, AstExpression *ex) {
    // Push parameter
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    unsigned expr_id = internal_names_cntr++;
    unsigned type = ex->params[0]->assumed_type;
    fprintf(output, "POPS GF@&&inter1\n");
    // Generate switch
    if (type == DT_UNKNOWN) {
        fprintf(output, "TYPE GF@&&inter2 GF@&&inter1\n"
                        "JUMPIFEQ $&&ifj_str_str%u GF@&&inter2 string@string\n"
                        "JUMPIFEQ $&&ifj_str_float%u GF@&&inter2 string@float\n"
                        "JUMPIFEQ $&&ifj_str_bool%u GF@&&inter2 string@bool\n"
                        "JUMPIFEQ $&&ifj_str_null%u GF@&&inter2 string@nil\n"
                        "EXIT int@26\n", // Shouldn't happen
                        expr_id, expr_id, expr_id, expr_id);
    }
    // Generate string branch
    if (type == DT_STRING || type == DT_UNKNOWN) {
        fprintf(output, "LABEL $&&ifj_str_str%u\n"
                        "PUSHS GF@&&inter1\n"
                        "JUMP $&&ifj_str_end%u\n",
                        expr_id, expr_id);
    }
    // Generate num branch
    if (type == DT_NUM || type == DT_UNKNOWN) {
        fprintf(output, "LABEL $&&ifj_str_float%u\n", expr_id);
        if (ex->params[0]->surely_int) {
            fprintf(output, "PUSHS GF@&&inter1\n"
                            "FLOAT2INTS\n"
                            "INT2STRS\n"
                            "JUMP $&&ifj_str_end%u\n",
                            expr_id);
        } else {
            fprintf(output, "PUSHS GF@&&inter1\n"
                            "ISINTS\n"
                            "PUSHS bool@true\n"
                            "JUMPIFEQS $&&ifj_str_float_int%u\n"
                            "PUSHS GF@&&inter1\n"
                            "FLOAT2STRS\n"
                            "JUMP $&&ifj_str_end%u\n"
                            "LABEL $&&ifj_str_float_int%u\n"
                            "PUSHS GF@&&inter1\n"
                            "FLOAT2INTS\n"
                            "INT2STRS\n"
                            "JUMP $&&ifj_str_end%u\n",
                            expr_id, expr_id, expr_id, expr_id);
        }
    }
    // Generate bool branch
    if (type == DT_BOOL || type == DT_UNKNOWN) {
        fprintf(output, "LABEL $&&ifj_str_bool%u\n"
                        "JUMPIFEQ $&&ifj_str_bool_true%u GF@&&inter1 bool@true\n"
                        "PUSHS string@false\n"
                        "JUMP $&&ifj_str_end%u\n"
                        "LABEL $&&ifj_str_bool_true%u\n"
                        "PUSHS string@true\n"
                        "JUMP $&&ifj_str_end%u\n",
                        expr_id, expr_id, expr_id, expr_id, expr_id);
    }
    // Generate null branch
    if (type == DT_NULL || type == DT_UNKNOWN) {
        fprintf(output, "LABEL $&&ifj_str_null%u\n"
                        "PUSHS string@null\n"
                        "JUMP $&&ifj_str_end%u\n",
                        expr_id, expr_id);
    }
    fprintf(output, "LABEL $&&ifj_str_end%u\n", expr_id);
    return OK;
}

/// Generates code for Ifj.write
ErrorCode generate_builtin_write(FILE *output, AstExpression *ex) {
    // Push parameter
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    unsigned type = ex->params[0]->assumed_type;
    unsigned expr_id = internal_names_cntr++;
    fprintf(output, "POPS GF@&&inter1\n");
    if (type == DT_UNKNOWN) {
        // We check if the expr is double and if so, we may convert it to an int int the next if
        fprintf(output, "PUSHS GF@&&inter1\n"
                        "TYPES\n"
                        "PUSHS string@float\n"
                        "JUMPIFNEQS $&&ifj_write_write%u\n",
                        expr_id);
    }
    if (type == DT_NUM || type == DT_UNKNOWN) {
        if (!ex->params[0]->surely_int) {
            fprintf(output, "PUSHS GF@&&inter1\n"
                            "ISINTS\n"
                            "PUSHS bool@false\n"
                            "JUMPIFEQS $&&ifj_write_write%u\n",\
                            expr_id);
        }
        fprintf(output, "FLOAT2INT GF@&&inter1 GF@&&inter1\n");
    }
    fprintf(output, "LABEL $&&ifj_write_write%u\n"
                    "WRITE GF@&&inter1\n"
                    "PUSHS nil@nil\n",
                    expr_id);
    return OK;
}

/// Generates code for Ifj.floor
ErrorCode generate_builtin_floor(FILE *output, AstExpression *ex) {
    // Push parameter
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    if (ex->params[0]->surely_int) {
        // We don't need to do anything if the argument is already an integer
        return OK;
    }
    if (ex->params[0]->assumed_type == DT_NUM) {
        fprintf(output, "FLOAT2INTS\n"
                        "INT2FLOATS\n");
        return OK;
    }
    unsigned expr_id = internal_names_cntr++;
    fprintf(output, "POPS GF@&&inter1\n"
                    "PUSHS GF@&&inter1\n"
                    "TYPES\n"
                    "PUSHS string@float\n"
                    "JUMPIFEQS $&&floor_float%u\n"
                    "EXIT int@26\n"
                    "LABEL $&&floor_float%u\n"
                    "PUSHS GF@&&inter1\n"
                    "FLOAT2INTS\n"
                    "INT2FLOATS\n",
                    expr_id, expr_id);
    return OK;
}

/// Generates code for Ifj.length
ErrorCode generate_builtin_length(FILE *output, AstExpression *ex) {
    AstExpression *param = ex->params[0];
    if (param->assumed_type == DT_STRING && param->val_known) {
        fprintf(output, "PUSHS float@%a\n", (double)strlen(param->string_val->val));
        return OK;
    }
    // Push parameter
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);

    fprintf(output, "POPS GF@&&inter1\n");
    if (param->assumed_type != DT_STRING) {
        generate_var_type_check(output, "GF@&&inter1", "string");
    }
    fprintf(output, "STRLEN GF@&&inter2 GF@&&inter1\n"
                    "PUSHS GF@&&inter2\n"
                    "INT2FLOATS\n");
    return OK;
}

/// Generates code for Ifj.substring
ErrorCode generate_builtin_substring(FILE *output, AstExpression *ex) {
    AstExpression *str = ex->params[0];
    AstExpression *start = ex->params[1];
    AstExpression *end = ex->params[2];
    // Type checks
    CG_ASSERT(generate_expression_evaluation(output, str) == OK);
    fprintf(output, "POPS GF@&&inter1\n");
    if (str->assumed_type == DT_UNKNOWN) {
        generate_var_type_check(output, "GF@&&inter1", "string");
    } else if (str->assumed_type != DT_STRING) {
        fprintf(output, "EXIT int@26\n"); // Shouldn't happen
    }

    CG_ASSERT(generate_expression_evaluation(output, start) == OK);
    fprintf(output, "POPS GF@&&inter2\n");
    if (start->assumed_type == DT_UNKNOWN) {
        generate_var_type_check(output, "GF@&&inter2", "float");
    } else if (start->assumed_type != DT_NUM) {
        fprintf(output, "EXIT int@26\n"); // Shouldn't happen
    }
    if (!start->surely_int) {
        generate_var_int_check(output, "GF@&&inter2");
    }
    fprintf(output, "FLOAT2INT GF@&&inter2 GF@&&inter2\n");

    CG_ASSERT(generate_expression_evaluation(output, end) == OK);
    fprintf(output, "POPS GF@&&inter3\n");
    if (end->assumed_type == DT_UNKNOWN) {
        generate_var_type_check(output, "GF@&&inter3", "float");
    } else if (end->assumed_type != DT_NUM) {
        fprintf(output, "EXIT int@26\n"); // Shouldn't happen
    }
    if (!start->surely_int) {
        generate_var_int_check(output, "GF@&&inter3");
    }
    fprintf(output, "FLOAT2INT GF@&&inter3 GF@&&inter3\n");

    unsigned expr_id = internal_names_cntr++;
    // Check boundaries
    fprintf(output, "STRLEN GF@&&inter4 GF@&&inter1\n"
                    "LT GF@&&inter5 GF@&&inter2 int@0\n" // i < 0
                    "JUMPIFEQ $&&substr_null%u GF@&&inter5 bool@true\n"
                    "GT GF@&&inter5 GF@&&inter3 GF@&&inter4\n" // j > len
                    "JUMPIFEQ $&&substr_null%u GF@&&inter5 bool@true\n"
                    "LT GF@&&inter5 GF@&&inter2 GF@&&inter4\n" // i >= len
                    "JUMPIFEQ $&&substr_null%u GF@&&inter5 bool@false\n"
                    "GT GF@&&inter5 GF@&&inter2 GF@&&inter3\n" // i > j
                    "JUMPIFEQ $&&substr_null%u GF@&&inter5 bool@true\n"
                    // We don't have to check j < 0 because i >= 0 and i <= j => j >= 0
                    "JUMP $&&substr_algo_start%u\n"
                    "LABEL $&&substr_null%u\n"
                    "PUSHS nil@nil\n"
                    "JUMP $&&substr_end%u\n"
                    "LABEL $&&substr_algo_start%u\n" ,
                    expr_id, expr_id, expr_id, expr_id, expr_id, expr_id, expr_id, expr_id);
    // Substring algorithm
    fprintf(output, "MOVE GF@&&inter5 string@\n"
                    "LABEL $&&substr_algo_iter%u\n"
                    "JUMPIFEQ $&&substr_loop_end%u GF@&&inter2 GF@&&inter3\n"
                    "GETCHAR GF@&&inter4 GF@&&inter1 GF@&&inter2\n"
                    "CONCAT GF@&&inter5 GF@&&inter5 GF@&&inter4\n"
                    "ADD GF@&&inter2 GF@&&inter2 int@1\n" // i = i + 1
                    "JUMP $&&substr_algo_iter%u\n"
                    "LABEL $&&substr_loop_end%u\n"
                    "PUSHS GF@&&inter5\n"
                    "LABEL $&&substr_end%u\n",
                    expr_id, expr_id, expr_id, expr_id, expr_id);
    return OK;
}

/// Generates code for Ifj.strcmp
ErrorCode generate_builtin_strcmp(FILE *output, AstExpression *ex) {
    AstExpression *str1 = ex->params[0];
    AstExpression *str2 = ex->params[1];
    // Type checks
    CG_ASSERT(generate_expression_evaluation(output, str1) == OK);
    fprintf(output, "POPS GF@&&inter1\n");
    if (str1->assumed_type == DT_UNKNOWN) {
        generate_var_type_check(output, "GF@&&inter1", "string");
    } else if (str1->assumed_type != DT_STRING) {
        fprintf(output, "EXIT int@26\n"); // Shouldn't happen
    }
    CG_ASSERT(generate_expression_evaluation(output, str2) == OK);
    fprintf(output, "POPS GF@&&inter2\n");
    if (str1->assumed_type == DT_UNKNOWN) {
        generate_var_type_check(output, "GF@&&inter2", "string");
    } else if (str1->assumed_type != DT_STRING) {
        fprintf(output, "EXIT int@26\n"); // Shouldn't happen
    }

    unsigned expr_id = internal_names_cntr++;
    // strcmp algorithm
    fprintf(output, "MOVE GF@&&inter3 int@0\n"
                    "STRLEN GF@&&inter4 GF@&&inter1\n"
                    "STRLEN GF@&&inter5 GF@&&inter2\n"
                    // Check if we have reached the end
                    "LABEL $&&strcmp_bound_check%u\n"
                    "JUMPIFNEQ $&&strcmp_neq1%u GF@&&inter3 GF@&&inter4\n"
                    // i == l1:
                    "JUMPIFNEQ $&&strcmp_less%u GF@&&inter3 GF@&&inter5\n" // i != l2
                    "JUMP $&&strcmp_eq%u\n" // i == l1, l2
                    // i != l1:
                    "LABEL $&&strcmp_neq1%u\n"
                    "JUMPIFEQ $&&strcmp_more%u GF@&&inter3 GF@&&inter5\n" // i == l2
                    // Compare next symbol
                    "STRI2INT GF@&&inter6 GF@&&inter1 GF@&&inter3\n"
                    "STRI2INT GF@&&inter7 GF@&&inter2 GF@&&inter3\n"
                    "ADD GF@&&inter3 GF@&&inter3 int@1\n"
                    "JUMPIFEQ $&&strcmp_bound_check%u GF@&&inter6 GF@&&inter7\n"
                    // Symbols are not the same
                    "LT GF@&&inter3 GF@&&inter6 GF@&&inter7\n"
                    "JUMPIFEQ $&&strcmp_less%u GF@&&inter3 bool@true\n"
                    // End
                    "LABEL $&&strcmp_more%u\n"
                    "PUSHS float@0x1p+0\n"
                    "JUMP $&&strcmp_end%u\n"
                    "LABEL $&&strcmp_less%u\n"
                    "PUSHS float@-0x1p+0\n"
                    "JUMP $&&strcmp_end%u\n"
                    "LABEL $&&strcmp_eq%u\n"
                    "PUSHS float@0x0p+0\n"
                    "LABEL $&&strcmp_end%u\n"
                    ,
                    expr_id, expr_id, expr_id, expr_id, expr_id, expr_id, expr_id, expr_id, expr_id,
                    expr_id, expr_id, expr_id, expr_id, expr_id);
    return OK;
}

ErrorCode generate_builtin_function_call(FILE *output, AstExpression *ex) {
    if (strcmp(ex->string_val->val, "write") == 0) {
        CG_ASSERT(ex->child_count == 1);
        generate_builtin_write(output, ex);
        return OK;
    }
    else if (strcmp(ex->string_val->val, "read_str") == 0) {
        CG_ASSERT(ex->child_count == 0);
        fprintf(output, "READ GF@&&inter1 string\n"
                        "PUSHS GF@&&inter1\n");
        return OK;
    }
    else if (strcmp(ex->string_val->val, "read_num") == 0) {
        CG_ASSERT(ex->child_count == 0);
        fprintf(output, "READ GF@&&inter1 float\n"
                        "PUSHS GF@&&inter1\n");
        return OK;
    }
    else if (strcmp(ex->string_val->val, "floor") == 0) {
        CG_ASSERT(ex->child_count == 1);
        return generate_builtin_floor(output, ex);
    }
    else if (strcmp(ex->string_val->val, "str") == 0) {
        CG_ASSERT(ex->child_count == 1);
        return generate_builtin_str(output, ex);
    }
    else if (strcmp(ex->string_val->val, "length") == 0) {
        CG_ASSERT(ex->child_count == 1);
        return generate_builtin_length(output, ex);
    }
    else if (strcmp(ex->string_val->val, "substring") == 0) {
        CG_ASSERT(ex->child_count == 3);
        return generate_builtin_substring(output, ex);
    }
    else if (strcmp(ex->string_val->val, "strcmp") == 0) {
        CG_ASSERT(ex->child_count == 2);
        return generate_builtin_strcmp(output, ex);
    }
    else if (strcmp(ex->string_val->val, "ord") == 0) {
        CG_ASSERT(ex->child_count == 2);
        CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
        CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
        unsigned expr_id = internal_names_cntr++;
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n");
        if (ex->params[0]->assumed_type == DT_UNKNOWN) {
            generate_var_type_check(output, "GF@&&inter1", "string");
        } else if (ex->params[0]->assumed_type != DT_STRING) {
            fprintf(output, "EXIT int@26\n"); // Shouldn't happen
            return OK;
        }
        if (ex->params[1]->assumed_type == DT_UNKNOWN) {
            generate_var_type_check(output, "GF@&&inter2", "float");
        } else if (ex->params[1]->assumed_type != DT_NUM) {
            fprintf(output, "EXIT int@26\n"); // Shouldn't happen
            return OK;
        }
        generate_var_int_check(output, "GF@&&inter2");
        fprintf(output, "FLOAT2INT GF@&&inter2 GF@&&inter2\n"
                        "STRLEN GF@&&inter3 GF@&&inter1\n" // Check bounds
                        "PUSHS GF@&&inter3\n"
                        "PUSHS GF@&&inter2\n"
                        "GTS\n"
                        "PUSHS bool@true\n"
                        "JUMPIFNEQS $&&ifj_ord_err%u\n"
                        "PUSHS GF@&&inter1\n" // Get value
                        "PUSHS GF@&&inter2\n"
                        "STRI2INTS\n"
                        "INT2FLOATS\n"
                        "JUMP $&&ifj_ord_end%u\n"
                        "LABEL $&&ifj_ord_err%u\n" // Error
                        "PUSHS float@0x0p+0\n"
                        "LABEL $&&ifj_ord_end%u\n",
                        expr_id, expr_id, expr_id, expr_id);
    }
    else if (strcmp(ex->string_val->val, "chr") == 0) {
        CG_ASSERT(ex->child_count == 1);
        CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);

        fprintf(output, "POPS GF@&&inter1\n");
        if (ex->params[0]->assumed_type == DT_UNKNOWN) {
            generate_var_type_check(output, "GF@&&inter1", "float");
        } else if (ex->params[0]->assumed_type != DT_NUM) {
            fprintf(output, "EXIT int@26\n"); // Shouldn't happen
            return OK;
        }
        generate_var_int_check(output, "GF@&&inter1");
        fprintf(output, "PUSHS GF@&&inter1\n"
                        "FLOAT2INTS\n"
                        "INT2CHARS\n");
    }
    else {
        return INTERNAL_ERROR;
    }
    return OK;
}

/// Generates code for applying an arithmetic operation to two expressions of known Num types
ErrorCode generate_arithmetic_with_known_type(FILE *output, AstExpression *ex, char *stack_op) {
    // Left side
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);

    // Right side
    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    fprintf(output, "%s\n", stack_op);
    return OK;
}

ErrorCode generate_add_expression(FILE *output, AstExpression *ex) {
    DataType left_type = ex->params[0]->assumed_type;
    DataType right_type = ex->params[1]->assumed_type;
    // Known data types
    if (left_type == DT_NUM && right_type == DT_NUM) {
        return generate_arithmetic_with_known_type(output, ex, "ADDS");
    }
    if (left_type == DT_STRING && right_type == DT_STRING) {
        CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
        CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "CONCAT GF@&&inter3 GF@&&inter1 GF@&&inter2\n"
                        "PUSHS GF@&&inter3\n");
        return OK;
    }
    if (left_type != DT_UNKNOWN && right_type != DT_UNKNOWN) {
        // Should be caught during semantic checks
        fprintf(output, "EXIT int@26\n");
        return OK;
    }
    unsigned expr_id = internal_names_cntr++;
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    // Unknown data types
    if (left_type == DT_STRING || right_type == DT_STRING) {
        // One is string -> check if other is string
        unsigned unknown = left_type == DT_UNKNOWN ? 1 : 2;
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "PUSHS GF@&&inter%u\n" // check type of unknown
                        "TYPES\n"
                        "PUSHS string@string\n"
                        "JUMPIFEQS $&&cat%u\n"
                        "EXIT int@26\n"
                        "LABEL $&&cat%u\n"
                        "CONCAT GF@&&inter3 GF@&&inter1 GF@&&inter2\n"
                        "PUSHS GF@&&inter3\n",
                        unknown, expr_id, expr_id);
        return OK;
    }
    if (left_type == DT_NUM || right_type == DT_NUM) {
        // One is num -> check if other is num
        unsigned unknown = left_type == DT_UNKNOWN ? 1 : 2;
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "PUSHS GF@&&inter%u\n" // check if unknown is num
                        "TYPES\n"
                        "PUSHS string@float\n"
                        "JUMPIFEQS $&&add_both_float%u\n"
                        "EXIT int@26\n"
                        "LABEL $&&add_both_float%u\n"
                        "ADD GF@&&inter3 GF@&&inter1 GF@&&inter2\n"
                        "PUSHS GF@&&inter3\n",
                        unknown, expr_id, expr_id);
        return OK;
    }

    // We know nothing
    // TODO: Rework

    // Pop values
    fprintf(output, "POPS GF@&&inter2\n"
                    "POPS GF@&&inter1\n");
    // Choose the path by the first operand type
    fprintf(output, "PUSHS GF@&&inter1\n" // string test
                    "TYPES\n"
                    "PUSHS string@string\n"
                    "JUMPIFEQS $&&add_string_val%u\n"
                    "PUSHS GF@&&inter1\n" // int test
                    "TYPES\n"
                    "PUSHS string@int\n"
                    "JUMPIFEQS $&&add_int_val%u\n"
                    "PUSHS GF@&&inter1\n" // float test
                    "TYPES\n"
                    "PUSHS string@float\n"
                    "JUMPIFEQS $&&add_float_val%u\n"
                    "EXIT int@26\n",
                    expr_id, expr_id, expr_id);
    // First is string
    fprintf(output, "LABEL $&&add_string_val%u\n"
                    "PUSHS GF@&&inter2\n" // check if second is string
                    "TYPES\n"
                    "PUSHS string@string\n"
                    "JUMPIFNEQS $&&add_type_err%u\n"
                    "CONCAT GF@&&inter3 GF@&&inter1 GF@&&inter2\n" // concat
                    "PUSHS GF@&&inter3\n"
                    "JUMP $&&add_end%u\n",
                    expr_id, expr_id, expr_id);
    // First is int
    fprintf(output, "LABEL $&&add_int_val%u\n"
                    "PUSHS GF@&&inter2\n" // check if second is int
                    "TYPES\n"
                    "PUSHS string@int\n"
                    "JUMPIFEQS $&&add_add%u\n" // it is - we add them
                    "PUSHS GF@&&inter2\n" // check if float
                    "TYPES\n"
                    "PUSHS string@float\n"
                    "JUMPIFNEQS $&&add_type_err%u\n" // it isn't - type error
                    "INT2FLOAT GF@&&inter1 GF@&&inter1\n" // convert first to float and add them
                    "JUMP $&&add_add%u\n",
                    expr_id, expr_id, expr_id, expr_id);
    // First is float
    fprintf(output, "LABEL $&&add_float_val%u\n"
                    "PUSHS GF@&&inter2\n" // check if second is float
                    "TYPES\n"
                    "PUSHS string@float\n"
                    "JUMPIFEQS $&&add_add%u\n" // it is - we add them
                    "PUSHS GF@&&inter2\n" // check if int
                    "TYPES\n"
                    "PUSHS string@int\n"
                    "JUMPIFNEQS $&&add_type_err%u\n" // it isn't - type error
                    "INT2FLOAT GF@&&inter2 GF@&&inter2\n" // convert second to float and add them
                    "JUMP $&&add_add%u\n",
                    expr_id, expr_id, expr_id, expr_id);
    // End
    fprintf(output, "LABEL $&&add_type_err%u\n"
                    "EXIT int@26\n"
                    "LABEL $&&add_add%u\n"
                    "ADD GF@&&inter3 GF@&&inter1 GF@&&inter2\n"
                    "PUSHS GF@&&inter3\n"
                    "LABEL $&&add_end%u\n",
                    expr_id, expr_id, expr_id);
    return OK;
}

/// Generates code for string iteration. Expects the string in inter1,
/// the count in inter2 and will put the result into inter3
void generate_string_iteration(FILE *output) {
    unsigned expr_id = internal_names_cntr++;
    fprintf(output, "MOVE GF@&&inter3 string@\n"
                    "LABEL $&&str_iter_cond%u\n"
                    "PUSHS GF@&&inter2\n"
                    "PUSHS float@0x0p+0\n"
                    "GTS\n"
                    "PUSHS bool@false\n"
                    "JUMPIFEQS $&&str_iter_end%u\n"
                    "SUB GF@&&inter2 GF@&&inter2 float@0x1p+0\n"
                    "CONCAT GF@&&inter3 GF@&&inter3 GF@&&inter1\n"
                    "JUMP $&&str_iter_cond%u\n"
                    "LABEL $&&str_iter_end%u\n",
                    expr_id, expr_id, expr_id, expr_id);
}

ErrorCode generate_mul_expression(FILE *output, AstExpression *ex) {
    DataType left_type = ex->params[0]->assumed_type;
    DataType right_type = ex->params[1]->assumed_type;
    // Known data types
    if (left_type == DT_NUM && right_type == DT_NUM) {
        return generate_arithmetic_with_known_type(output, ex, "MULS");
    }
    unsigned expr_id = internal_names_cntr++;
    if (left_type == DT_STRING && right_type == DT_NUM) {
        CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
        CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
        fprintf(output, "POPS GF@&&inter2\n"
                        "POPS GF@&&inter1\n"
                        "ISINT GF@&&inter3 GF@&&inter2\n" // We check if the number is an integer
                        "JUMPIFEQ $&&str_iter_is_integer%u GF@&&inter3 bool@true\n"
                        "EXIT int@26\n"
                        "LABEL $&&str_iter_is_integer%u\n",
                        expr_id, expr_id);
        generate_string_iteration(output);
        fprintf(output, "PUSHS GF@&&inter3\n");
        return OK;
    }
    if (left_type != DT_UNKNOWN && right_type != DT_UNKNOWN) {
        // Should be caught during semantic checks
        fprintf(output, "EXIT int@26\n");
        return OK;
    }
    // Unknown data types
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    fprintf(output, "POPS GF@&&inter2\n"
                    "POPS GF@&&inter1\n");
    if (left_type == DT_UNKNOWN) {
        // Switch by data type
        fprintf(output, "PUSHS GF@&&inter1\n"
                        "TYPES\n"
                        "PUSHS string@string\n"
                        "JUMPIFEQS $&&mul_string_val%u\n"
                        "PUSHS GF@&&inter1\n"
                        "TYPES\n"
                        "PUSHS string@float\n"
                        "JUMPIFEQS $&&mul_float_val%u\n"
                        "EXIT int@26\n",
                        expr_id, expr_id);
    }
    if (left_type == DT_STRING || left_type == DT_UNKNOWN) {
        if (left_type == DT_UNKNOWN) {
            // We only need the label if left is unknown, otherwise this is the default path
            fprintf(output, "LABEL $&&mul_string_val%u\n", expr_id);
        }
        if (right_type == DT_UNKNOWN) {
            // Check right type
            fprintf(output, "PUSHS GF@&&inter2\n"
                            "TYPES\n"
                            "PUSHS string@float\n"
                            "JUMPIFEQS $&&mul_string_float%u\n"
                            "EXIT int@26\n"
                            "LABEL $&&mul_string_float%u\n"
                            "PUSHS GF@&&inter2\n"
                            "ISINTS\n"
                            "PUSHS bool@true\n"
                            "JUMPIFEQS $&&mul_string_int%u\n"
                            "EXIT int@26\n"
                            "LABEL $&&mul_string_int%u\n",
                            expr_id, expr_id, expr_id, expr_id);
        }
        generate_string_iteration(output);
        fprintf(output, "PUSHS GF@&&inter3\n"
                        "JUMP $&&mul_end%u\n", expr_id);
    }
    if (left_type == DT_NUM || left_type == DT_UNKNOWN) {
        if (left_type == DT_UNKNOWN) {
            // We only need the label if left is unknown, otherwise this is the default path
            fprintf(output, "LABEL $&&mul_float_val%u\n", expr_id);
        }
        if (right_type == DT_UNKNOWN) {
            // Check right type
            fprintf(output, "PUSHS GF@&&inter2\n"
                            "TYPES\n"
                            "PUSHS string@float\n"
                            "JUMPIFEQS $&&mul_mul%u\n"
                            "PUSHS GF@&&inter2\n"
                            "TYPES\n"
                            "PUSHS string@int\n"
                            "JUMPIFNEQS $&&mul_type_errror%u\n"
                            "INT2FLOAT GF@&&inter2 GF@&&inter2\n",
                            expr_id, expr_id);
        } else if (right_type == DT_NUM) {
            // Nothing
        } else {
            // Should get caught by semantic analysis
            fprintf(output, "EXIT int@26\n");
        }
        fprintf(output, "JUMP $&&mul_mul%u\n", expr_id);
    }
    fprintf(output, "LABEL $&&mul_type_errror%u\n"
                    "EXIT int@26\n"
                    "LABEL $&&mul_mul%u\n"
                    "MUL GF@&&inter3 GF@&&inter1 GF@&&inter2\n"
                    "PUSHS GF@&&inter3\n"
                    "LABEL $&&mul_end%u\n",
                    expr_id, expr_id, expr_id);


    return OK;
}

ErrorCode generate_binary_operator_with_floats(FILE *output, AstExpression *ex, char *stack_op) {
    DataType left_type = ex->params[0]->assumed_type;
    DataType right_type = ex->params[1]->assumed_type;

    // Left side
    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    if (left_type == DT_UNKNOWN) {
        // Check if left is float or int and convert to float
        generate_stack_type_check(output, "float");
    }
    CG_ASSERT(left_type == DT_UNKNOWN || left_type == DT_NUM);

    // Right side
    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);
    if (right_type == DT_UNKNOWN) {
        // Check if right is float or int and convert to float
        generate_stack_type_check(output, "float");
    }
    CG_ASSERT(right_type == DT_UNKNOWN ||right_type == DT_NUM);

    fprintf(output, "%s\n", stack_op);

    return OK;
}

ErrorCode generate_equals_expression(FILE *output, AstExpression *ex) {
    DataType left_type = ex->params[0]->assumed_type;
    DataType right_type = ex->params[1]->assumed_type;
    // We set all ints to doubles to possibly skip subexpressions evaluations
    // Both types known and not the same -> false
    if (left_type != right_type && left_type != DT_UNKNOWN && right_type != DT_UNKNOWN) {
        fprintf(output, "PUSHS bool@false\n");
        return OK;
    }
    // We reset them again to convert them if needed
    left_type = ex->params[0]->assumed_type;
    right_type = ex->params[1]->assumed_type;

    CG_ASSERT(generate_expression_evaluation(output, ex->params[0]) == OK);
    CG_ASSERT(generate_expression_evaluation(output, ex->params[1]) == OK);

    if (left_type == right_type && left_type != DT_UNKNOWN && right_type != DT_UNKNOWN) {
        fprintf(output, "EQS\n");
        return OK;
    }

    unsigned expr_id = internal_names_cntr++;

    // We don't know their types
    fprintf(output, "POPS GF@&&inter2\n"
                    "POPS GF@&&inter1\n");

    // Push left type
    // This structure saves one instruction. Worth it
    if (left_type == DT_NUM) { fprintf(output, "PUSHS string@float\n"); }
    else if (left_type == DT_STRING) { fprintf(output, "PUSHS string@string\n"); }
    else if (left_type == DT_BOOL) { fprintf(output, "PUSHS string@bool\n"); }
    else if (left_type == DT_NULL) { fprintf(output, "PUSHS string@nil\n"); }
    else {
        fprintf(output, "PUSHS GF@&&inter1\n"
                        "TYPES\n");
    }
    // Push right type
    if (right_type == DT_NUM) { fprintf(output, "PUSHS string@float\n"); }
    else if (right_type == DT_STRING) { fprintf(output, "PUSHS string@string\n"); }
    else if (right_type == DT_BOOL) { fprintf(output, "PUSHS string@bool\n"); }
    else if (right_type == DT_NULL) { fprintf(output, "PUSHS string@nil\n"); }
    else {
        fprintf(output, "PUSHS GF@&&inter2\n"
                        "TYPES\n");
    }
    fprintf(output, "JUMPIFNEQS $&&eq_false%u\n" // Compare types
                    "PUSHS GF@&&inter1\n" // Types are the same, compare values
                    "PUSHS GF@&&inter2\n"
                    "EQS\n"
                    "JUMP $&&eq_end%u\n"
                    "LABEL $&&eq_false%u\n"
                    "PUSHS bool@false\n"
                    "LABEL $&&eq_end%u\n",
                    expr_id, expr_id, expr_id, expr_id);

    return OK;
}

ErrorCode push_known_value(FILE *output, AstExpression *ex) {
    String *converted_string;
    switch (ex->assumed_type) {
    case DT_NULL:
        fprintf(output, "PUSHS nil@nil\n");
        break;
    case DT_NUM:
        fprintf(output, "PUSHS float@%a\n", ex->double_val);
        break;
    case DT_STRING:
        CG_ASSERT(convert_string(ex->string_val->val, &converted_string) == OK);
        fprintf(output, "PUSHS string@%s\n", converted_string->val);
        str_free(&converted_string);
        break;
    case DT_BOOL:
        fprintf(output, "PUSHS bool@%s\n", ex->bool_val ? "true" : "false");
        break;
    case DT_UNKNOWN:
    case DT_TYPE:
        return INTERNAL_ERROR;
    }
    return OK;
}

ErrorCode generate_expression_evaluation(FILE *output, AstExpression *st) {
    if (st->val_known) {
        ErrorCode ec = push_known_value(output, st);
        if (ec == OK) return OK;
    }
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
        fprintf(output, "PUSHS float@0x0p+0\n");
        CG_ASSERT(generate_expression_evaluation(output, st->params[0]) == OK);
        fprintf(output, "SUBS\n");
        return OK;
    case EX_DATA_TYPE:
        fprintf(output, "EXIT int@26\n");
        return OK;
    case EX_BUILTIN_FUN:
        CG_ASSERT(generate_builtin_function_call(output, st) == OK);
        return OK;
    case EX_AND:
        return generate_and_expr(output, st);
    case EX_OR:
        return generate_or_expr(output, st);
    case EX_ADD:
        return generate_add_expression(output, st);
    case EX_MUL:
        return generate_mul_expression(output, st);
    case EX_SUB:
        return generate_binary_operator_with_floats(output, st, "SUBS");
    case EX_DIV:
        return generate_binary_operator_with_floats(output, st, "DIVS");
    case EX_GREATER:
        return generate_binary_operator_with_floats(output, st, "GTS");
    case EX_LESS:
        return generate_binary_operator_with_floats(output, st, "LTS");
    case EX_GREATER_EQ:
        return generate_binary_operator_with_floats(output, st, "LTS\nNOTS");
    case EX_LESS_EQ:
        return generate_binary_operator_with_floats(output, st, "GTS\nNOTS");
    case EX_EQ:
        return generate_equals_expression(output, st);
    case EX_NOT_EQ:
        CG_ASSERT(generate_equals_expression(output, st) == OK);
        fprintf(stdout, "NOTS\n");
        return OK;
    default:
        return INTERNAL_ERROR;
    }
    return INTERNAL_ERROR;
}

ErrorCode generate_var_assignment(FILE *output, char *scope, AstVariable *st) {
    if (st->expression == NULL) return OK;
    if (st->expression->val_known) {
        AstExpression *expr = st->expression;
        String *str;
        // We can assign directly without pushing and poping
        fprintf(output, "MOVE %s@%s ", scope, st->name->val);
        switch (expr->assumed_type) {
        case DT_NUM:
            fprintf(output, "float@%a", expr->double_val);
            break;
        case DT_STRING:
            convert_string(expr->string_val->val, &str);
            fprintf(output, "string@%s", str->val);
            str_free(&str);
            break;
        case DT_BOOL:
            fprintf(output, "bool@%s", expr->bool_val ? "true" : "false");
            break;
        default:
            fprintf(output, "nil@nil");
            break;
        }
        fprintf(output, "\n");

        return OK;
    }
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
        if (st->local_var->expression == NULL) {
            fprintf(output, "MOVE LF@%s nil@nil\n", st->local_var->name->val);
            return OK;
        } else {
            return generate_var_assignment(output, "LF", st->local_var);
        }
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
    generate_compound_statement(output, fun->body);

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
                    "DEFVAR GF@&&inter3\n"
                    "DEFVAR GF@&&inter4\n"
                    "DEFVAR GF@&&inter5\n"
                    "DEFVAR GF@&&inter6\n"
                    "DEFVAR GF@&&inter7\n");
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
