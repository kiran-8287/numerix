#ifndef NUMERIX_EXPRESSION_INTERNAL_H
#define NUMERIX_EXPRESSION_INTERNAL_H

#include "numerix.h"

typedef enum numerix_token_type {
    NUMERIX_TOKEN_END = 0,
    NUMERIX_TOKEN_NUMBER,
    NUMERIX_TOKEN_IDENTIFIER,
    NUMERIX_TOKEN_PLUS,
    NUMERIX_TOKEN_MINUS,
    NUMERIX_TOKEN_MULTIPLY,
    NUMERIX_TOKEN_DIVIDE,
    NUMERIX_TOKEN_POWER,
    NUMERIX_TOKEN_LEFT_PARENTHESIS,
    NUMERIX_TOKEN_RIGHT_PARENTHESIS,
    NUMERIX_TOKEN_COMMA
} numerix_token_type;

typedef struct numerix_token {
    numerix_token_type type;
    size_t position;
    size_t length;
    double number;
} numerix_token;

typedef enum numerix_expression_node_type {
    NUMERIX_NODE_NUMBER = 0,
    NUMERIX_NODE_CONSTANT,
    NUMERIX_NODE_VARIABLE,
    NUMERIX_NODE_UNARY_PLUS,
    NUMERIX_NODE_UNARY_MINUS,
    NUMERIX_NODE_ADD,
    NUMERIX_NODE_SUBTRACT,
    NUMERIX_NODE_MULTIPLY,
    NUMERIX_NODE_DIVIDE,
    NUMERIX_NODE_POWER,
    NUMERIX_NODE_FUNCTION
} numerix_expression_node_type;

typedef enum numerix_expression_function {
    NUMERIX_FUNCTION_SIN = 0,
    NUMERIX_FUNCTION_COS,
    NUMERIX_FUNCTION_TAN,
    NUMERIX_FUNCTION_ASIN,
    NUMERIX_FUNCTION_ACOS,
    NUMERIX_FUNCTION_ATAN,
    NUMERIX_FUNCTION_SINH,
    NUMERIX_FUNCTION_COSH,
    NUMERIX_FUNCTION_TANH,
    NUMERIX_FUNCTION_SQRT,
    NUMERIX_FUNCTION_ABS,
    NUMERIX_FUNCTION_EXP,
    NUMERIX_FUNCTION_LOG,
    NUMERIX_FUNCTION_LOG10,
    NUMERIX_FUNCTION_FLOOR,
    NUMERIX_FUNCTION_CEIL,
    NUMERIX_FUNCTION_POW,
    NUMERIX_FUNCTION_MIN,
    NUMERIX_FUNCTION_MAX
} numerix_expression_function;

typedef struct numerix_expression_node {
    numerix_expression_node_type type;
    numerix_expression_function function;
    double number;
    size_t position;
    size_t depth;
    struct numerix_expression_node *left;
    struct numerix_expression_node *right;
} numerix_expression_node;

struct numerix_expression {
    numerix_expression_node *root;
};

numerix_status numerix_expression_lex(
    const char *source,
    numerix_token **tokens,
    size_t *token_count,
    numerix_error *error);

numerix_status numerix_expression_parse(
    const char *source,
    const numerix_token *tokens,
    size_t token_count,
    numerix_expression_node **root,
    numerix_error *error);

void numerix_expression_node_destroy(numerix_expression_node *node);

numerix_status numerix_expression_evaluate_node(
    const numerix_expression_node *node,
    double x,
    double *value,
    numerix_error *error);

#endif
