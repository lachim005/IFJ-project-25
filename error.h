/* error.h
 * Return codes used by the IFJ25 compiler
 *
 * Authors:
 * Tomáš Hanák (xhanakt00)
 * 
 * Values and meanings (as specified):
 *  0  - successful translation
 *  1  - lexical analysis error
 *  2  - syntactic analysis error
 *  3  - semantic error: use of undefined function/variable
 *  4  - semantic error: redefinition of function/variable
 *  5  - static semantic error: wrong number of arguments or bad built-in parameter type
 *  6  - static semantic type compatibility error (arith, string, relational)
 * 10  - other semantic errors
 * 99  - internal compiler error (e.g. memory allocation failure)
 */

#ifndef _ERROR_H
#define _ERROR_H

typedef enum error_code {
    OK = 0,
    LEXICAL_ERROR = 1,
    SYNTACTIC_ERROR = 2,

    /// semantic errors
    SEM_UNDEFINED = 3,   /// use of undefined function or variable
    SEM_REDEFINITION = 4,/// redefinition of function or variable
    SEM_BAD_PARAMS = 5,  /// unexpected number/type of args for function or built-in param
    SEM_TYPE_COMPAT = 6, /// type compatibility error in expressions
    SEM_OTHER = 10,      /// other semantic errors

    /// internal/compiler errors
    INTERNAL_ERROR = 99
} ErrorCode;

#endif // !_ERROR_H