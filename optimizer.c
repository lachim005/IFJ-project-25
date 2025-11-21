/*
 * optimizer.c
 * Implements optimization for ast
 *
 * IFJ project 2025
 * FIT VUT
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 */

#include "optimizer.h"
#include "ast.h"
#include "error.h"
#include "symtable.h"
#include "string.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

void update_symtable_value(SymtableItem *item, AstExpression *expr) {
    if (item == NULL || expr == NULL) {
        return;
    }

    if (item->data_type_known && item->data_type == DT_STRING) {
        str_free(&item->string_val);
    }

    item->data_type_known = true;
    item->data_type = expr->assumed_type;

    switch (expr->assumed_type) {
        case DT_NUM:
            item->double_val = expr->double_val;
            break;
        case DT_BOOL:
            item->bool_val = expr->bool_val;
            break;
        case DT_STRING:
            item->string_val = str_init();
            if (item->string_val != NULL && expr->string_val != NULL) {
                str_append_string(item->string_val, expr->string_val->val);
            }
            break;
        case DT_NULL:
            break;
        default:
            item->data_type_known = false;
            break;
    }
}

void clear_symtable_item_value(SymtableItem *it, void* par) {
    // Par is unused here
    (void)par;

    if (it->data_type_known == false) return;
    it->data_type_known = false;
    if (it->data_type == DT_STRING) {
        str_free(&it->string_val);
    }
    it->data_type = DT_UNKNOWN;
}

void clear_symtable_values(Symtable *st) {
    symtable_foreach(st, clear_symtable_item_value, NULL);
}

ErrorCode optimize_expression(AstExpression *expr, Symtable *globaltable, Symtable *localtable) {
    if (expr == NULL) {
        return INTERNAL_ERROR;
    }

    // First optimize all child expressions
    for (size_t i = 0; i < expr->child_count; i++) {
        ErrorCode ec = optimize_expression(expr->params[i], globaltable, localtable);
        if (ec != OK) {
            return ec;
        }
    }

    // Try to evaluate the expression if all operands are known
    bool all_known = true;
    for (size_t i = 0; i < expr->child_count; i++) {
        if (!expr->params[i]->val_known) {
            all_known = false;
            break;
        }
    }

    if (!all_known) {
        return OK;
    }

    // Evaluate based on expression type
    switch (expr->type) {
        // Binary arithmetic operations
        case EX_ADD:
            if (expr->child_count == 2) {
                AstExpression *left = expr->params[0];
                AstExpression *right = expr->params[1];
                
                if (left->assumed_type == DT_NUM && right->assumed_type == DT_NUM) {
                    expr->val_known = true;
                    expr->assumed_type = DT_NUM;
                    expr->double_val = left->double_val + right->double_val;
                } else if (left->assumed_type == DT_STRING && right->assumed_type == DT_STRING) {
                    expr->val_known = true;
                    if (expr->assumed_type == DT_STRING && expr->string_val != NULL) {
                        str_free(&expr->string_val);
                    }
                    expr->assumed_type = DT_STRING;
                    expr->string_val = str_init();
                    if (expr->string_val != NULL) {
                        str_append_string(expr->string_val, left->string_val->val);
                        str_append_string(expr->string_val, right->string_val->val);
                    }
                }
            }
            break;

        case EX_SUB:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_NUM;
                expr->double_val = expr->params[0]->double_val - expr->params[1]->double_val;
            }
            break;

        case EX_MUL:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_NUM;
                expr->double_val = expr->params[0]->double_val * expr->params[1]->double_val;
            } else if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_STRING && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                if (expr->assumed_type == DT_STRING && expr->string_val != NULL) {
                    str_free(&expr->string_val);
                }
                expr->assumed_type = DT_STRING;
                expr->string_val = str_init();
                if (expr->string_val != NULL) {
                    int repeat_count = (int)expr->params[1]->double_val;
                    for (int i = 0; i < repeat_count; i++) {
                        str_append_string(expr->string_val, expr->params[0]->string_val->val);
                    }
                }
            }
            break;

        case EX_DIV:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM && expr->params[1]->double_val != 0.0) {
                expr->val_known = true;
                expr->assumed_type = DT_NUM;
                expr->double_val = expr->params[0]->double_val / expr->params[1]->double_val;
            }
            break;

        // Unary operations
        case EX_NEGATE:
            if (expr->child_count == 1 && expr->params[0]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_NUM;
                expr->double_val = -expr->params[0]->double_val;
            }
            break;

        case EX_NOT:
            if (expr->child_count == 1 && expr->params[0]->assumed_type == DT_BOOL) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = !expr->params[0]->bool_val;
            }
            break;

        // Comparison operations
        case EX_GREATER:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = expr->params[0]->double_val > expr->params[1]->double_val;
            }
            break;

        case EX_LESS:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = expr->params[0]->double_val < expr->params[1]->double_val;
            }
            break;

        case EX_GREATER_EQ:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = expr->params[0]->double_val >= expr->params[1]->double_val;
            }
            break;

        case EX_LESS_EQ:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_NUM && expr->params[1]->assumed_type == DT_NUM) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = expr->params[0]->double_val <= expr->params[1]->double_val;
            }
            break;

        case EX_EQ:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == expr->params[1]->assumed_type) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                
                if (expr->params[0]->assumed_type == DT_NUM) {
                    expr->bool_val = expr->params[0]->double_val == expr->params[1]->double_val;
                } else if (expr->params[0]->assumed_type == DT_BOOL) {
                    expr->bool_val = expr->params[0]->bool_val == expr->params[1]->bool_val;
                } else if (expr->params[0]->assumed_type == DT_STRING) {
                    expr->bool_val = strcmp(expr->params[0]->string_val->val, expr->params[1]->string_val->val) == 0;
                } else if (expr->params[0]->assumed_type == DT_NULL) {
                    expr->bool_val = true;
                }
            }
            break;

        case EX_NOT_EQ:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == expr->params[1]->assumed_type) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                
                if (expr->params[0]->assumed_type == DT_NUM) {
                    expr->bool_val = expr->params[0]->double_val != expr->params[1]->double_val;
                } else if (expr->params[0]->assumed_type == DT_BOOL) {
                    expr->bool_val = expr->params[0]->bool_val != expr->params[1]->bool_val;
                } else if (expr->params[0]->assumed_type == DT_STRING) {
                    expr->bool_val = strcmp(expr->params[0]->string_val->val, expr->params[1]->string_val->val) != 0;
                } else if (expr->params[0]->assumed_type == DT_NULL) {
                    expr->bool_val = false;
                }
            }
            break;

        // Logical operations
        case EX_AND:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_BOOL && expr->params[1]->assumed_type == DT_BOOL) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = expr->params[0]->bool_val && expr->params[1]->bool_val;
            }
            break;

        case EX_OR:
            if (expr->child_count == 2 && expr->params[0]->assumed_type == DT_BOOL && expr->params[1]->assumed_type == DT_BOOL) {
                expr->val_known = true;
                expr->assumed_type = DT_BOOL;
                expr->bool_val = expr->params[0]->bool_val || expr->params[1]->bool_val;
            }
            break;

        // Ternary operator
        case EX_TERNARY:
            if (expr->child_count == 3 && expr->params[0]->assumed_type == DT_BOOL) {
                // Pick the branch based on condition
                if (expr->params[0]->bool_val) {
                    // Copy true branch result
                    expr->val_known = expr->params[1]->val_known;
                    expr->assumed_type = expr->params[1]->assumed_type;
                    if (expr->params[1]->assumed_type == DT_NUM) {
                        expr->double_val = expr->params[1]->double_val;
                    } else if (expr->params[1]->assumed_type == DT_BOOL) {
                        expr->bool_val = expr->params[1]->bool_val;
                    } else if (expr->params[1]->assumed_type == DT_STRING) {
                        if (expr->assumed_type == DT_STRING && expr->string_val != NULL) {
                            str_free(&expr->string_val);
                        }
                        expr->string_val = str_init();
                        if (expr->string_val != NULL) {
                            str_append_string(expr->string_val, expr->params[1]->string_val->val);
                        }
                    }
                } else {
                    // Copy false branch result
                    expr->val_known = expr->params[2]->val_known;
                    expr->assumed_type = expr->params[2]->assumed_type;
                    if (expr->params[2]->assumed_type == DT_NUM) {
                        expr->double_val = expr->params[2]->double_val;
                    } else if (expr->params[2]->assumed_type == DT_BOOL) {
                        expr->bool_val = expr->params[2]->bool_val;
                    } else if (expr->params[2]->assumed_type == DT_STRING) {
                        if (expr->assumed_type == DT_STRING && expr->string_val != NULL) {
                            str_free(&expr->string_val);
                        }
                        expr->string_val = str_init();
                        if (expr->string_val != NULL) {
                            str_append_string(expr->string_val, expr->params[2]->string_val->val);
                        }
                    }
                }
            }
            break;

        // Variable lookup
        case EX_ID:
            if (expr->string_val != NULL) {
                SymtableItem *item = NULL;
                if ((item = symtable_find(localtable, expr->string_val->val)) != NULL) {
                    // We always have to set the data type to unknown if it is unknown
                    // in the symtable because it could have been reset due to side effects
                    if (item->data_type == DT_UNKNOWN) {
                        expr->assumed_type = DT_UNKNOWN;
                        break;
                    }
                    if (item->data_type_known) {
                        expr->val_known = true;
                        expr->assumed_type = item->data_type;
                        
                        str_free(&expr->string_val);
                        
                        switch (item->data_type) {
                            case DT_NUM:
                                expr->type = EX_DOUBLE;
                                expr->double_val = item->double_val;
                                break;
                            case DT_BOOL:
                                expr->type = EX_BOOL;
                                expr->bool_val = item->bool_val;
                                break;
                            case DT_STRING:
                                expr->type = EX_STRING;
                                expr->string_val = str_init();
                                if (expr->string_val != NULL && item->string_val != NULL) {
                                    str_append_string(expr->string_val, item->string_val->val);
                                }
                                break;
                            case DT_NULL:
                                expr->type = EX_NULL;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            break;

        case EX_GLOBAL_ID:
            if (expr->string_val != NULL) {
                SymtableItem *item = NULL;
                if (symtable_contains_global_var(globaltable, expr->string_val->val, &item) && item != NULL) {
                    // We always have to set the data type to unknown if it is unknown
                    // in the symtable because it could have been reset due to side effects
                    if (item->data_type == DT_UNKNOWN) {
                        expr->assumed_type = DT_UNKNOWN;
                        break;
                    }
                    if (item->data_type_known) {
                        expr->val_known = true;
                        expr->assumed_type = item->data_type;
                        
                        str_free(&expr->string_val);
                        
                        switch (item->data_type) {
                            case DT_NUM:
                                expr->type = EX_DOUBLE;
                                expr->double_val = item->double_val;
                                break;
                            case DT_BOOL:
                                expr->type = EX_BOOL;
                                expr->bool_val = item->bool_val;
                                break;
                            case DT_STRING:
                                expr->type = EX_STRING;
                                expr->string_val = str_init();
                                if (expr->string_val != NULL && item->string_val != NULL) {
                                    str_append_string(expr->string_val, item->string_val->val);
                                }
                                break;
                            case DT_NULL:
                                expr->type = EX_NULL;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            break;
        case EX_FUN:
        case EX_GETTER:
            // Functions and getters could have side effects on global variables,
            // so we have to clear their values
            clear_symtable_values(globaltable);
            break;
        case EX_BUILTIN_FUN:
            if (strcmp(expr->string_val->val, "floor") == 0) {
                if (expr->params[0]->assumed_type != DT_NUM) return SEM_TYPE_COMPAT;
                str_free(&expr->string_val);
                expr->val_known = true;
                expr->assumed_type = DT_NUM;
                expr->surely_int = true;
                expr->double_val = floor(expr->params[0]->double_val);
                break;
            }
            if (strcmp(expr->string_val->val, "length") == 0) {
                if (expr->params[0]->assumed_type != DT_STRING) return SEM_TYPE_COMPAT;
                str_free(&expr->string_val);
                expr->assumed_type = DT_NUM;
                expr->val_known = true;
                expr->double_val = (double)expr->params[0]->string_val->length;
                break;
            }
            if (strcmp(expr->string_val->val, "substring") == 0) {
                if (expr->params[0]->assumed_type != DT_STRING) return SEM_TYPE_COMPAT;
                if (expr->params[1]->assumed_type != DT_NUM) return SEM_TYPE_COMPAT;
                if (!expr->params[1]->surely_int) break;
                if (expr->params[2]->assumed_type != DT_NUM) return SEM_TYPE_COMPAT;
                if (!expr->params[2]->surely_int) break;
                int start = (int)expr->params[1]->double_val;
                int end = (int)expr->params[2]->double_val;
                char *str = expr->params[0]->string_val->val;
                int len = expr->params[0]->string_val->length;
                if (start < 0 || end < 0 || start > end || start >= len || end > len) {
                    expr->assumed_type = DT_NULL;
                    break;
                }
                String *res = str_init();
                if (res == NULL) return INTERNAL_ERROR;
                for (int i = start; i < end; i++) {
                    if (!str_append_char(res, str[i])) {
                        return INTERNAL_ERROR;
                        str_free(&res);
                    }
                }
                // Clear function name
                str_free(&expr->string_val);
                expr->string_val = res;

                expr->val_known = true;
                expr->assumed_type = DT_STRING;
                break;
            }
            if (strcmp(expr->string_val->val, "strcmp") == 0) {
                if (expr->params[0]->assumed_type != DT_STRING) return SEM_TYPE_COMPAT;
                if (expr->params[1]->assumed_type != DT_STRING) return SEM_TYPE_COMPAT;
                str_free(&expr->string_val);
                expr->assumed_type = DT_NUM;
                expr->val_known = true;
                int res = strcmp(expr->params[0]->string_val->val, expr->params[1]->string_val->val);
                expr->double_val = res > 0 ? 1 :
                                   res < 0 ? -1 :
                                   0;
                break;
            }
            if (strcmp(expr->string_val->val, "ord") == 0) {
                if (expr->params[0]->assumed_type != DT_STRING) return SEM_TYPE_COMPAT;
                if (expr->params[1]->assumed_type != DT_NUM) return SEM_TYPE_COMPAT;
                if (!expr->params[1]->surely_int) break;
                char *str = expr->params[0]->string_val->val;
                int index = (int)expr->params[1]->double_val;
                str_free(&expr->string_val);
                expr->val_known = true;
                expr->assumed_type = DT_NUM;
                if (index >= (int)strlen(str) || index < 0) {
                    // Out of bound index
                    expr->double_val = 0.0;
                } else {
                    expr->double_val = (double)(unsigned char)str[index];
                }
                break;
            }
            if (strcmp(expr->string_val->val, "chr") == 0) {
                if (expr->params[0]->assumed_type != DT_NUM) return SEM_TYPE_COMPAT;
                if (!expr->params[0]->surely_int) break;
                int num = (int)expr->params[0]->double_val;
                if (num < 0 || num > 255) break;
                expr->val_known = true;
                expr->assumed_type = DT_STRING;
                // Clear function name
                str_clear(expr->string_val);
                if (!str_append_char(expr->string_val, (char)num)) return INTERNAL_ERROR;
                break;
            }
            break;
        default:
            break;
    }

    if (expr->val_known && expr->assumed_type == DT_NUM) {
        // We check if the known value is an integer
        expr->surely_int = ceil(expr->double_val) == expr->double_val;
    }

    return OK;
}

ErrorCode optimize_block(AstBlock *block, Symtable *globaltable, Symtable *localtable) {
    if (block == NULL) {
        return OK;
    }

    AstStatement *stmt = block->statements;
    while (stmt != NULL && stmt->type != ST_END) {
        ErrorCode ec = optimize_statement(stmt, globaltable, localtable);
        if (ec != OK) {
            return ec;
        }
        stmt = stmt->next;
    }

    return OK;
}

ErrorCode optimize_root(AstStatement *statement, Symtable *globaltable, Symtable *localtable) {
    if (statement == NULL) {
        return OK;
    }

    AstStatement *stmt = statement;
    while (stmt != NULL && stmt->type != ST_END) {
        ErrorCode ec = optimize_statement(stmt, globaltable, localtable);
        if (ec != OK) {
            return ec;
        }
        stmt = stmt->next;
    }

    return OK;
}

ErrorCode optimize_statement(AstStatement *statement, Symtable *globaltable, Symtable *localtable) {
    if (statement == NULL) {
        return OK;
    }

    ErrorCode ec = OK;
    AstExpression *cond;

    switch (statement->type) {
        case ST_LOCAL_VAR:
            if (statement->local_var != NULL && statement->local_var->expression != NULL) {
                ec = optimize_expression(statement->local_var->expression, globaltable, localtable);
                if (ec != OK) return ec;

                // Update symtable with known value
                if (statement->local_var->expression->val_known) {
                    SymtableItem *item = NULL;
                    if ((item = symtable_find(localtable, statement->local_var->name->val)) != NULL) {
                        update_symtable_value(item, statement->local_var->expression);
                    }
                }
            }
            break;

        case ST_GLOBAL_VAR:
            if (statement->global_var != NULL && statement->global_var->expression != NULL) {
                ec = optimize_expression(statement->global_var->expression, globaltable, localtable);
                if (ec != OK) return ec;

                // Update symtable with known value
                if (statement->global_var->expression->val_known) {
                    SymtableItem *item = NULL;
                    if (symtable_contains_global_var(globaltable, statement->global_var->name->val, &item) && item != NULL) {
                        update_symtable_value(item, statement->global_var->expression);
                    }
                }
            }
            break;

        case ST_SETTER_CALL:
            if (statement->setter_call != NULL && statement->setter_call->expression != NULL) {
                ec = optimize_expression(statement->setter_call->expression, globaltable, localtable);
            }
            // Setter could have side effects on global variables, so we have to clear their values
            clear_symtable_values(globaltable);
            break;

        case ST_RETURN:
            if (statement->return_expr != NULL) {
                ec = optimize_expression(statement->return_expr, globaltable, localtable);
            }
            break;

        case ST_IF:
            if (statement->if_st == NULL) break;

            cond = statement->if_st->condition;
            ec = optimize_expression(cond, globaltable, localtable);
            if (ec != OK) return ec;

            if (cond->assumed_type == DT_UNKNOWN || !cond->val_known || cond->bool_val)
            {
                // We only optimize this branch if it can execute
                ec = optimize_block(statement->if_st->true_branch, globaltable, localtable);
                if (ec != OK) return ec;
                if (cond->assumed_type == DT_UNKNOWN || !cond->val_known) {
                    // If we aren't sure that this branch will execute, we have to clear
                    // symtable values because it could have side effects
                    clear_symtable_values(localtable);
                    clear_symtable_values(globaltable);
                } else {
                    // We now that this branch will execute for sure, so we are done here
                    break;
                }
            }

            // Optimize else-if branches
            for (size_t i = 0; i < statement->if_st->else_if_count; i++) {
                if (statement->if_st->else_if_branches[i] != NULL) {
                    ec = optimize_expression(statement->if_st->else_if_branches[i]->condition, globaltable, localtable);
                    if (ec != OK) return ec;

                    ec = optimize_block(statement->if_st->else_if_branches[i]->body, globaltable, localtable);
                    if (ec != OK) return ec;

                    clear_symtable_values(localtable);
                    clear_symtable_values(globaltable);
                }
            }

            // Optimize false branch
            if (statement->if_st->false_branch != NULL &&
                (cond->assumed_type == DT_UNKNOWN || !cond->val_known || !cond->bool_val)) {
                // We only optimize this branch if it can execute
                ec = optimize_block(statement->if_st->false_branch, globaltable, localtable);
                if (cond->assumed_type == DT_UNKNOWN || !cond->val_known) {
                    // If we aren't sure that this branch will execute, we have to clear
                    // symtable values because it could have side effects
                    clear_symtable_values(localtable);
                    clear_symtable_values(globaltable);
                }
            }
            break;

        case ST_WHILE:
            if (statement->while_st == NULL) break;

            clear_symtable_values(localtable);
            clear_symtable_values(globaltable);
            cond = statement->while_st->condition;

            ec = optimize_expression(cond, globaltable, localtable);
            if (ec != OK) return ec;
            ec = optimize_block(statement->while_st->body, globaltable, localtable);
            if (ec != OK) return ec;

            clear_symtable_values(localtable);
            clear_symtable_values(globaltable);

            break;
            
        case ST_BLOCK:
            if (statement->block != NULL) {
                ec = optimize_block(statement->block, globaltable, localtable);
            }
            break;

        case ST_FUNCTION:
            if (statement->function != NULL && statement->function->body != NULL) {
                ec = optimize_block(statement->function->body, globaltable, statement->function->symtable);
                clear_symtable_values(globaltable);
            }
            break;

        case ST_GETTER:
            if (statement->getter != NULL && statement->getter->body != NULL) {
                ec = optimize_block(statement->getter->body, globaltable, statement->getter->symtable);
                clear_symtable_values(globaltable);
            }
            break;

        case ST_SETTER:
            if (statement->setter != NULL && statement->setter->body != NULL) {
                ec = optimize_block(statement->setter->body, globaltable, statement->setter->symtable);
                clear_symtable_values(globaltable);
            }
            break;

        case ST_EXPRESSION:
            if (statement->expression != NULL) {
                ec = optimize_expression(statement->expression, globaltable, localtable);
            }
            break;

        case ST_ROOT:
            ec = optimize_root(statement->next, globaltable, localtable);
            break;
        case ST_END:
            break;
    }

    return ec;
}

ErrorCode optimize_ast(AstStatement *root, Symtable *globaltable) {
    if (root == NULL) {
        return INTERNAL_ERROR;
    }

    if (globaltable == NULL) {
        return INTERNAL_ERROR;
    }

    ErrorCode ec = optimize_statement(root, globaltable, NULL);
    return ec;
}

