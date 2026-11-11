#include "numerix_expression_internal.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMERIX_EXPRESSION_MAX_SOURCE_LENGTH 4096U

static numerix_status lexer_error(
    numerix_error *error,
    size_t position,
    const char *message)
{
    char formatted[NUMERIX_ERROR_MESSAGE_CAPACITY];

    (void)snprintf(
        formatted,
        sizeof(formatted),
        "%s at character %lu",
        message,
        (unsigned long)position);

    return numerix_error_set(error, NUMERIX_STATUS_INVALID_INPUT, formatted);
}

static int is_identifier_start(char character)
{
    unsigned char value = (unsigned char)character;

    return isalpha(value) != 0 || character == '_';
}

static int is_identifier_part(char character)
{
    unsigned char value = (unsigned char)character;

    return isalnum(value) != 0 || character == '_';
}

static int is_digit(char character)
{
    return isdigit((unsigned char)character) != 0;
}

static numerix_status append_token(
    numerix_token *tokens,
    size_t capacity,
    size_t *count,
    numerix_token_type type,
    size_t position,
    size_t length,
    double number,
    numerix_error *error)
{
    numerix_token *token;

    if (*count >= capacity)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INTERNAL_ERROR,
            "expression token capacity exceeded");
    }

    token = &tokens[*count];
    token->type = type;
    token->position = position;
    token->length = length;
    token->number = number;
    ++(*count);

    return NUMERIX_STATUS_SUCCESS;
}

static numerix_status scan_number(
    const char *source,
    size_t *cursor,
    numerix_token *tokens,
    size_t capacity,
    size_t *token_count,
    numerix_error *error)
{
    size_t start = *cursor;
    size_t digit_count = 0;
    size_t exponent_digits = 0;
    char *end = NULL;
    double value;
    numerix_status status;

    while (is_digit(source[*cursor]))
    {
        ++digit_count;
        ++(*cursor);
    }

    if (source[*cursor] == '.')
    {
        ++(*cursor);

        while (is_digit(source[*cursor]))
        {
            ++digit_count;
            ++(*cursor);
        }
    }

    if (digit_count == 0)
    {
        return lexer_error(error, start, "invalid numeric literal");
    }

    if (source[*cursor] == 'e' || source[*cursor] == 'E')
    {
        ++(*cursor);

        if (source[*cursor] == '+' || source[*cursor] == '-')
        {
            ++(*cursor);
        }

        while (is_digit(source[*cursor]))
        {
            ++exponent_digits;
            ++(*cursor);
        }

        if (exponent_digits == 0)
        {
            return lexer_error(error, start, "malformed exponent in numeric literal");
        }
    }

    errno = 0;
    value = strtod(source + start, &end);

    if (end == source + start || (size_t)(end - (source + start)) != *cursor - start)
    {
        return lexer_error(error, start, "invalid numeric literal");
    }

    if (errno == ERANGE || !isfinite(value))
    {
        return lexer_error(error, start, "numeric literal is outside the finite double range");
    }

    status = append_token(
        tokens,
        capacity,
        token_count,
        NUMERIX_TOKEN_NUMBER,
        start,
        *cursor - start,
        value,
        error);

    return status;
}

numerix_status numerix_expression_lex(
    const char *source,
    numerix_token **tokens_out,
    size_t *token_count_out,
    numerix_error *error)
{
    numerix_token *tokens;
    size_t source_length;
    size_t cursor = 0;
    size_t token_count = 0;
    numerix_status status = NUMERIX_STATUS_SUCCESS;

    if (tokens_out == NULL || token_count_out == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "token output arguments are required");
    }

    *tokens_out = NULL;
    *token_count_out = 0;

    if (source == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_ARGUMENT,
            "expression source must not be NULL");
    }

    source_length = strlen(source);

    if (source_length > NUMERIX_EXPRESSION_MAX_SOURCE_LENGTH)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_INVALID_INPUT,
            "expression exceeds the maximum source length of 4096 characters");
    }

    tokens = numerix_allocate((source_length + 1) * sizeof(*tokens));

    if (tokens == NULL)
    {
        return numerix_error_set(
            error,
            NUMERIX_STATUS_ALLOCATION_FAILURE,
            "could not allocate expression tokens");
    }

    while (cursor < source_length)
    {
        char character = source[cursor];
        numerix_token_type type;
        size_t start;

        if (isspace((unsigned char)character) != 0)
        {
            ++cursor;
            continue;
        }

        if (is_digit(character) ||
            (character == '.' && is_digit(source[cursor + 1])))
        {
            status = scan_number(
                source,
                &cursor,
                tokens,
                source_length + 1,
                &token_count,
                error);

            if (status != NUMERIX_STATUS_SUCCESS)
            {
                numerix_deallocate(tokens);
                return status;
            }

            continue;
        }

        if (is_identifier_start(character))
        {
            start = cursor;
            ++cursor;

            while (cursor < source_length && is_identifier_part(source[cursor]))
            {
                ++cursor;
            }

            status = append_token(
                tokens,
                source_length + 1,
                &token_count,
                NUMERIX_TOKEN_IDENTIFIER,
                start,
                cursor - start,
                0.0,
                error);

            if (status != NUMERIX_STATUS_SUCCESS)
            {
                numerix_deallocate(tokens);
                return status;
            }

            continue;
        }

        switch (character)
        {
            case '+':
                type = NUMERIX_TOKEN_PLUS;
                break;
            case '-':
                type = NUMERIX_TOKEN_MINUS;
                break;
            case '*':
                type = NUMERIX_TOKEN_MULTIPLY;
                break;
            case '/':
                type = NUMERIX_TOKEN_DIVIDE;
                break;
            case '^':
                type = NUMERIX_TOKEN_POWER;
                break;
            case '(':
                type = NUMERIX_TOKEN_LEFT_PARENTHESIS;
                break;
            case ')':
                type = NUMERIX_TOKEN_RIGHT_PARENTHESIS;
                break;
            case ',':
                type = NUMERIX_TOKEN_COMMA;
                break;
            default:
                numerix_deallocate(tokens);
                return lexer_error(error, cursor, "invalid character");
        }

        status = append_token(
            tokens,
            source_length + 1,
            &token_count,
            type,
            cursor,
            1,
            0.0,
            error);

        if (status != NUMERIX_STATUS_SUCCESS)
        {
            numerix_deallocate(tokens);
            return status;
        }

        ++cursor;
    }

    status = append_token(
        tokens,
        source_length + 1,
        &token_count,
        NUMERIX_TOKEN_END,
        source_length,
        0,
        0.0,
        error);

    if (status != NUMERIX_STATUS_SUCCESS)
    {
        numerix_deallocate(tokens);
        return status;
    }

    *tokens_out = tokens;
    *token_count_out = token_count;
    numerix_error_clear(error);
    return NUMERIX_STATUS_SUCCESS;
}
