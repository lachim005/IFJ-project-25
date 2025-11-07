#include "ast.h"
#include <stdlib.h>

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
    return expr;
}