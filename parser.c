/*
 * parser.c
 * Implements parser
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 */

#include "parser.h"
#include "error.h"
#include "symtable.h"
#include "expr_parser.h"
#include <string.h>

#define RETURN_CODE(code, token) do { \
    token_free(&token); \
    return code; \
} while(0)

#define CHECK_TOKEN(lexer, token) do { \
    token_free(&token); \
    if(lexer_get_token(lexer, &token) != ERR_LEX_OK) { \
        return LEXICAL_ERROR; \
    } \
} while(0)

#define CHECK_TOKEN_SKIP_NEWLINE(lexer, token) do { \
    token_free(&token); \
    do { \
        if(lexer_get_token(lexer, &token) != ERR_LEX_OK) { \
            return LEXICAL_ERROR; \
        } \
    } while(token.type == TOK_EOL); \
} while(0)

/// Macro for adding getter to symbol table with redefinition check
#define ADD_GETTER(symtable, name, error_token) do { \
    SymtableItem *existing_item = NULL; \
    if (symtable_contains_getter(symtable, name, &existing_item)) { \
        if (existing_item != NULL && existing_item->is_defined) { \
            RETURN_CODE(SEM_REDEFINITION, error_token); \
        } \
        if (existing_item != NULL && !existing_item->is_defined) { \
            existing_item->is_defined = 1; \
            symtable_decrement_undefined_items_counter(symtable); \
        } \
    } else { \
        SymtableItem *new_item = symtable_add_getter(symtable, name, 1); \
        if (new_item == NULL) { \
            RETURN_CODE(INTERNAL_ERROR, error_token); \
        } \
    } \
} while(0)

/// Macro for adding setter to symbol table with redefinition check
#define ADD_SETTER(symtable, name, error_token) do { \
    SymtableItem *existing_item = NULL; \
    if (symtable_contains_setter(symtable, name, &existing_item)) { \
        if (existing_item != NULL && existing_item->is_defined) { \
            RETURN_CODE(SEM_REDEFINITION, error_token); \
        } \
        if (existing_item != NULL && !existing_item->is_defined) { \
            existing_item->is_defined = 1; \
            symtable_decrement_undefined_items_counter(symtable); \
        } \
    } else { \
        SymtableItem *new_item = symtable_add_setter(symtable, name, 1); \
        if (new_item == NULL) { \
            RETURN_CODE(INTERNAL_ERROR, error_token); \
        } \
    } \
} while(0)

/// Macro for adding function to symbol table with redefinition check
#define ADD_FUNCTION(symtable, name, pcount, error_token) do { \
    SymtableItem *existing_item = NULL; \
    if (symtable_contains_function(symtable, name, pcount, &existing_item)) { \
        if (existing_item != NULL && existing_item->is_defined) { \
            RETURN_CODE(SEM_REDEFINITION, error_token); \
        } \
        if (existing_item != NULL && !existing_item->is_defined) { \
            existing_item->is_defined = 1; \
            symtable_decrement_undefined_items_counter(symtable); \
        } \
    } else { \
        SymtableItem *new_item = symtable_add_function(symtable, name, pcount, 1); \
        if (new_item == NULL) { \
            RETURN_CODE(INTERNAL_ERROR, error_token); \
        } \
    } \
} while(0)

/// Macro for adding variable to symbol table with redefinition check
#define ADD_VARIABLE(symtable, name, error_token) do { \
    if (contains_var_at_current_scope(symtable, name)) { \
        RETURN_CODE(SEM_REDEFINITION, error_token); \
    } \
    SymtableItem *new_item = add_var_at_current_scope(symtable, name); \
    if (new_item == NULL) { \
        RETURN_CODE(INTERNAL_ERROR, error_token); \
    } \
    new_item->is_defined = 1; \
} while(0)

/// Macro for adding global variable to symbol table with redefinition check
#define ADD_GLOBAL_VARIABLE(symtable, name, error_token) do { \
    SymtableItem *existing_item = symtable_find(symtable, name); \
    if (existing_item != NULL) { \
        if (existing_item->is_defined) { \
            RETURN_CODE(SEM_REDEFINITION, error_token); \
        } \
        if (!existing_item->is_defined) { \
            existing_item->is_defined = 1; \
            symtable_decrement_undefined_items_counter(symtable); \
        } \
    } else { \
        SymtableItem *new_item = symtable_insert(symtable, name); \
        if (new_item == NULL) { \
            RETURN_CODE(INTERNAL_ERROR, error_token); \
        } \
        new_item->is_defined = 1; \
    } \
} while(0)

/// Macro for checking variable expression - checks if variable exists locally or as setter globally
#define CHECK_VARIABLE_EXPRESSION(localtable, globaltable, name, error_token) do { \
    SymtableItem *local_var = NULL; \
    if (!find_local_var(localtable, name, &local_var)) { \
        RETURN_CODE(INTERNAL_ERROR, error_token); \
    } \
    if (local_var == NULL) { \
        SymtableItem *setter_item = NULL; \
        if (!symtable_contains_setter(globaltable, name, &setter_item)) { \
            SymtableItem *new_setter = symtable_add_setter(globaltable, name, 0); \
            if (new_setter == NULL) { \
                RETURN_CODE(INTERNAL_ERROR, error_token); \
            } \
            symtable_increment_undefined_items_counter(globaltable); \
        } \
    } \
} while(0)


/// Adds all builtin functions to the symbol table
/// Format: #function_name$param_count
/// Returns true if all functions were successfully added, false otherwise
bool add_builtin_functions(Symtable *symtab) {
    // I/O functions
    if (symtable_insert(symtab, "#read_str$0") == NULL) return false;   // Ifj.read_str() -> String | Null
    if (symtable_insert(symtab, "#read_num$0") == NULL) return false;   // Ifj.read_num() -> Num | Null
    if (symtable_insert(symtab, "#write$1") == NULL) return false;      // Ifj.write(term) -> Null

    // Conversion functions
    if (symtable_insert(symtab, "#floor$1") == NULL) return false;      // Ifj.floor(Num) -> Num
    if (symtable_insert(symtab, "#str$1") == NULL) return false;        // Ifj.str(term) -> String

    // String functions
    if (symtable_insert(symtab, "#length$1") == NULL) return false;     // Ifj.length(String) -> Num
    if (symtable_insert(symtab, "#substring$3") == NULL) return false;  // Ifj.substring(String, Num, Num) -> String | Null
    if (symtable_insert(symtab, "#strcmp$2") == NULL) return false;     // Ifj.strcmp(String, String) -> Num
    if (symtable_insert(symtab, "#ord$2") == NULL) return false;        // Ifj.ord(String, Num) -> Num
    if (symtable_insert(symtab, "#chr$1") == NULL) return false;        // Ifj.chr(Num) -> String

    return true;
}

/// Checks prologue of the program
/// Rule: import "ifj25" for Ifj
ErrorCode check_prologue(Lexer *lexer) {
    Token token;
    token.type = TOK_KW_IFJ;

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_KW_IMPORT)
        RETURN_CODE(SYNTACTIC_ERROR, token);

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_LIT_STRING || strcmp(token.string_val->val, "ifj25"))
        RETURN_CODE(SYNTACTIC_ERROR, token);

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_KW_FOR)
        RETURN_CODE(SYNTACTIC_ERROR, token);

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_KW_IFJ)
        RETURN_CODE(SYNTACTIC_ERROR, token);

    RETURN_CODE(OK, token);
}

/// Checks class Program { ... }
ErrorCode check_class_program(Lexer *lexer, Symtable *symtable) {
    Token token;
    token.type = TOK_KW_CLASS;

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_KW_CLASS)
        RETURN_CODE(SYNTACTIC_ERROR, token);

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_IDENTIFIER || strcmp(token.string_val->val, "Program"))
        RETURN_CODE(SYNTACTIC_ERROR, token);

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_BRACE)
        RETURN_CODE(SYNTACTIC_ERROR, token);

    ErrorCode ec = check_class_body(lexer, symtable);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_RIGHT_BRACE)
        RETURN_CODE(SYNTACTIC_ERROR, token);

    RETURN_CODE(OK, token);
}

/// Checks the body of a class
ErrorCode check_class_body(Lexer *lexer, Symtable *symtable) {
    Token token;
    token.type = TOK_IDENTIFIER;
    
    while (true) {
        CHECK_TOKEN_SKIP_NEWLINE(lexer, token);

        // If we encounter a right brace, end of class body
        if (token.type == TOK_RIGHT_BRACE) {
            // Return token back for parent function
            if (!lexer_unget_token(lexer, token)) {
                token_free(&token);
                return INTERNAL_ERROR;
            }
            return OK;
        }

        ErrorCode ec;
        switch (token.type) {
            case TOK_KW_STATIC:
                ec = check_statics(lexer, symtable);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_GLOBAL_VAR:
                ec = check_global_var(lexer, symtable, token.string_val);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            default:
                RETURN_CODE(SYNTACTIC_ERROR, token);
        }
    }
}

/// Checks the global variable declaration
ErrorCode check_global_var(Lexer *lexer, Symtable *symtable, String *var_name) {
    Token token;
    token.type = TOK_GLOBAL_VAR;
    
    // Add variable to symbol table with redefinition check
    ADD_GLOBAL_VARIABLE(symtable, var_name->val, token);
    
    CHECK_TOKEN(lexer, token);
    if (token.type == TOK_OP_ASSIGN) {
        // Parse assignment expression
        AstExpression *expr;
        ErrorCode ec = parse_expression(lexer, &expr);
        if (ec != OK) {
            RETURN_CODE(ec, token);
        }
        CHECK_TOKEN(lexer, token);
    }

    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    RETURN_CODE(OK, token);
}

/// Checks statics (static functions and variables)
ErrorCode check_statics(Lexer *lexer, Symtable *symtable) {
    Token identifier;
    identifier.type = TOK_IDENTIFIER;

    CHECK_TOKEN(lexer, identifier);
    if (identifier.type != TOK_IDENTIFIER)
        RETURN_CODE(SYNTACTIC_ERROR, identifier);

    Symtable *new_symtable = symtable_init();
    if (new_symtable == NULL) {
        RETURN_CODE(INTERNAL_ERROR, identifier);
    }

    Token token;
    token.type = TOK_OP_ASSIGN;
    CHECK_TOKEN(lexer, token);
    
    // Setter: static identifier = (val) { ... }
    if (token.type == TOK_OP_ASSIGN) {
        ErrorCode ec = checks_setter(lexer, symtable, new_symtable, identifier);
        if (ec != OK) {
            token_free(&identifier);
            symtable_free(new_symtable);
            RETURN_CODE(ec, token);
        }
    }
    // Getter: static identifier { ... }
    else if (token.type == TOK_LEFT_BRACE) {
        ErrorCode ec = check_getter(lexer, symtable, new_symtable, identifier);
        if (ec != OK) {
            token_free(&identifier);
            symtable_free(new_symtable);
            RETURN_CODE(ec, token);
        }
    }
    // Function: static identifier(...) { ... }
    else if (token.type == TOK_LEFT_PAR) {
        ErrorCode ec = check_function(lexer, symtable, new_symtable, identifier);
        if (ec != OK) {
            token_free(&identifier);
            symtable_free(new_symtable);
            RETURN_CODE(ec, token);
        }
    }
    else {
        // Unexpected token after identifier
        token_free(&identifier);
        symtable_free(new_symtable);
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }
    
    token_free(&identifier);

    RETURN_CODE(OK, token);
}

/// Checks Setter: static identifier = (val) { ... }
ErrorCode checks_setter(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier) {
    // Enters new scope for the setter
    enter_scope(localtable);

    // Add setter to global symbol table with automatic redefinition check
    ADD_SETTER(globaltable, identifier.string_val->val, identifier);

    Token token;
    token.type = TOK_OP_ASSIGN;

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_IDENTIFIER) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Insert parameter into local symbol table
    if (add_var_at_current_scope(localtable, token.string_val->val) == NULL) {
        RETURN_CODE(INTERNAL_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_RIGHT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Check setter body
    ErrorCode ec = check_body(lexer, globaltable, localtable);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_RIGHT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Exits the scope for the setter
    exit_scope(localtable);

    RETURN_CODE(OK, token);
}

/// Checks Getter: static identifier { ... }
ErrorCode check_getter(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier) {
    // Enters new scope for the getter
    enter_scope(localtable);

    // Add getter to global symbol table with automatic redefinition check
    ADD_GETTER(globaltable, identifier.string_val->val, identifier);

    Token token;
    token.type = TOK_LEFT_BRACE;

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Check getter body
    ErrorCode ec = check_body(lexer, globaltable, localtable);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_RIGHT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Exits the scope for the getter
    exit_scope(localtable);

    RETURN_CODE(OK, token);
}

/// Checks Function: static identifier(...) { ... }
ErrorCode check_function(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier) {
    // Enters new scope for the function
    enter_scope(localtable);

    Token token;
    token.type = TOK_LEFT_PAR;

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Parse function parameters
    int param_count = 0;
    while (true) {
        CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
        if (token.type != TOK_IDENTIFIER && token.type != TOK_RIGHT_PAR) {
            RETURN_CODE(SYNTACTIC_ERROR, token);
        }

        if (token.type == TOK_RIGHT_PAR) {
            break;
        }

        // Insert parameter into local symbol table
        if (add_var_at_current_scope(localtable, token.string_val->val) == NULL) {
            RETURN_CODE(INTERNAL_ERROR, token);
        }
        param_count++;

        CHECK_TOKEN(lexer, token);
        if (token.type == TOK_RIGHT_PAR) {
            break;
        }

        if (token.type != TOK_COMMA) {
            RETURN_CODE(SYNTACTIC_ERROR, token);
        }
    }

    // Add function to global symbol table
    ADD_FUNCTION(globaltable, identifier.string_val->val, param_count, token);

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Check function body
    ErrorCode ec = check_body(lexer, globaltable, localtable);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_RIGHT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Exits the scope for the function
    exit_scope(localtable);

    RETURN_CODE(OK, token);
}

/// Checks body
ErrorCode check_body(Lexer *lexer, Symtable *globaltable, Symtable *localtable) {
    Token token;
    token.type = TOK_IDENTIFIER;

    while (true) {
        CHECK_TOKEN_SKIP_NEWLINE(lexer, token);

        // If we encounter a right brace, end of function body
        if (token.type == TOK_RIGHT_BRACE) {
            // Return token back for parent function
            if (!lexer_unget_token(lexer, token)) {
                token_free(&token);
                return INTERNAL_ERROR;
            }
            return OK;
        }

        ErrorCode ec;
        switch (token.type) {
            case TOK_GLOBAL_VAR:
                ec = check_global_var(lexer, globaltable, token.string_val);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_KW_VAR:
                ec = check_local_var(lexer, globaltable, localtable);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_IDENTIFIER:
                ec = check_assignment_or_function_call(lexer, globaltable, localtable, token);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_KW_IF:
                ec = check_if_statement(lexer, globaltable, localtable);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_KW_WHILE:
                ec = check_while_statement(lexer, globaltable, localtable);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_KW_RETURN:
                ec = check_return_statement(lexer, globaltable, localtable);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
                break;

            case TOK_LEFT_BRACE:
                // New scope
                enter_scope(localtable);

                ec = check_body(lexer, globaltable, localtable);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }

                CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
                if (token.type != TOK_RIGHT_BRACE) {
                    RETURN_CODE(SYNTACTIC_ERROR, token);
                }

                CHECK_TOKEN(lexer, token);
                if (token.type != TOK_EOL) {
                    RETURN_CODE(SYNTACTIC_ERROR, token);
                }

                // Exit scope
                exit_scope(localtable);

                break;

            default:
                ec = parse_expression(lexer, NULL);
                if (ec != OK) {
                    RETURN_CODE(ec, token);
                }
        }
    }
}

/// Checks local variable declaration
ErrorCode check_local_var(Lexer *lexer, Symtable *globaltable, Symtable *localtable) {
    Token identifier;
    identifier.type = TOK_IDENTIFIER;

    CHECK_TOKEN(lexer, identifier);
    if (identifier.type != TOK_IDENTIFIER) {
        RETURN_CODE(SYNTACTIC_ERROR, identifier);
    }

    // Check if is assigned
    Token token;
    token.type = TOK_OP_ASSIGN;

    CHECK_TOKEN(lexer, token);
    if (token.type == TOK_OP_ASSIGN) {
        // Parse assignment expression
        AstExpression *expr;
        ErrorCode ec = parse_expression(lexer, &expr);
        if (ec != OK) {
            RETURN_CODE(ec, token);
        }

        CHECK_TOKEN(lexer, token);
    }

    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Add variable to symbol table with redefinition check
    ADD_VARIABLE(localtable, identifier.string_val->val, identifier);

    RETURN_CODE(OK, token);
}

/// Checks assignment or function call
ErrorCode check_assignment_or_function_call(Lexer *lexer, Symtable *globaltable, Symtable *localtable, Token identifier) {
    Token token;
    token.type = TOK_OP_ASSIGN;

    CHECK_TOKEN(lexer, token);
    if (token.type == TOK_OP_ASSIGN) {
        CHECK_VARIABLE_EXPRESSION(localtable, globaltable, identifier.string_val->val, identifier);
        // Find variable in symbol tables
        // Parse assignment expression
        AstExpression *expr;
        ErrorCode ec = parse_expression(lexer, &expr);
        if (ec != OK) {
            RETURN_CODE(ec, token);
        }

        CHECK_TOKEN(lexer, token);
    } else if (token.type == TOK_LEFT_PAR) {
        // Function call
        // Return 2 tokens back for expression parser
        if (!lexer_unget_token(lexer, token)) {
            token_free(&token);
            return INTERNAL_ERROR;
        }

        if (!lexer_unget_token(lexer, identifier)) {
            return INTERNAL_ERROR;
        }

        // Parse function call expression
        AstExpression *expr;
        ErrorCode ec = parse_expression(lexer, &expr);
        if (ec != OK) {
            RETURN_CODE(ec, token);
        }

        CHECK_TOKEN(lexer, token);
    }

    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    RETURN_CODE(OK, token);
}

/// Checks if statement
ErrorCode check_if_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable) {
    Token token;
    token.type = TOK_KW_IF;

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Parse condition expression
    AstExpression *expr;
    ErrorCode ec = parse_expression(lexer, &expr);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_RIGHT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Enter new scope for if body
    enter_scope(localtable);

    // Check if body
    ec = check_body(lexer, globaltable, localtable);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    // Exit scope for if body
    exit_scope(localtable);

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_RIGHT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Optional else part
    CHECK_TOKEN(lexer, token);
    if (token.type == TOK_KW_ELSE) {
        CHECK_TOKEN(lexer, token);
        if (token.type != TOK_LEFT_BRACE) {
            RETURN_CODE(SYNTACTIC_ERROR, token);
        }

        // Enter new scope for else body
        enter_scope(localtable);

        // Check else body
        ec = check_body(lexer, globaltable, localtable);
        if (ec != OK) {
            RETURN_CODE(ec, token);
        }

        // Exit scope for else body
        exit_scope(localtable);

        CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
        if (token.type != TOK_RIGHT_BRACE) {
            RETURN_CODE(SYNTACTIC_ERROR, token);
        }

        CHECK_TOKEN(lexer, token);
    }

    // Return token back for parent function
    if (!lexer_unget_token(lexer, token)) {
        token_free(&token);
        return INTERNAL_ERROR;
    }

    RETURN_CODE(OK, token);
}

/// Checks while statement
ErrorCode check_while_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable) {
    Token token;
    token.type = TOK_KW_WHILE;

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Parse condition expression
    AstExpression *expr;
    ErrorCode ec = parse_expression(lexer, &expr);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_RIGHT_PAR) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_LEFT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Enter new scope for while body
    enter_scope(localtable);

    // Check while body
    ec = check_body(lexer, globaltable, localtable);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    // Exit scope for while body
    exit_scope(localtable);

    CHECK_TOKEN_SKIP_NEWLINE(lexer, token);
    if (token.type != TOK_RIGHT_BRACE) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    // Return token back for parent function
    if (!lexer_unget_token(lexer, token)) {
        token_free(&token);
        return INTERNAL_ERROR;
    }

    RETURN_CODE(OK, token);
}

/// Checks return statement
ErrorCode check_return_statement(Lexer *lexer, Symtable *globaltable, Symtable *localtable) {
    Token token;
    token.type = TOK_KW_RETURN;

    // Parse return expression
    AstExpression *expr;
    ErrorCode ec = parse_expression(lexer, &expr);
    if (ec != OK) {
        RETURN_CODE(ec, token);
    }

    CHECK_TOKEN(lexer, token);
    if (token.type != TOK_EOL) {
        RETURN_CODE(SYNTACTIC_ERROR, token);
    }

    RETURN_CODE(OK, token);
}

ErrorCode parse() {
    Lexer lexer_structure;
    Lexer *lexer = &lexer_structure;
    if (!lexer_init(lexer, stdin)){
        return INTERNAL_ERROR;
    }

    Symtable *symtable = symtable_init();
    if (symtable == NULL) {
        lexer_free(lexer);
        return INTERNAL_ERROR;
    }

    // Add all builtin functions
    if (!add_builtin_functions(symtable)) {
        symtable_free(symtable);
        lexer_free(lexer);
        return INTERNAL_ERROR;
    }

    if (check_prologue(lexer) != OK) {
        symtable_free(symtable);
        lexer_free(lexer);
        return SYNTACTIC_ERROR;
    }

    if (check_class_program(lexer, symtable) != OK) {
        symtable_free(symtable);
        lexer_free(lexer);
        return SYNTACTIC_ERROR;
    }

    if (symtable_get_undefined_items_count(symtable) > 0) {
        return SEM_UNDEFINED;
    }

    symtable_free(symtable);
    lexer_free(lexer);

    return OK;
}

int main() {
    parse();
    return 0;
}