#ifndef NUMERIX_PHASE14_JSON_H
#define NUMERIX_PHASE14_JSON_H

/* Small, allocation-free JSON token reader for the generated Phase 14 inputs. */
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef enum p14_json_kind {
    P14_JSON_OBJECT,
    P14_JSON_ARRAY,
    P14_JSON_STRING,
    P14_JSON_PRIMITIVE
} p14_json_kind;

typedef struct p14_json_token {
    size_t start;
    size_t end;
    size_t next;
    size_t first_child;
    p14_json_kind kind;
} p14_json_token;

typedef struct p14_json_doc {
    const char *text;
    p14_json_token *tokens;
    size_t count;
    size_t capacity;
} p14_json_doc;

static void p14_json_skip_space(const char **cursor)
{
    while (**cursor != '\0' && isspace((unsigned char)**cursor)) {
        ++(*cursor);
    }
}

static size_t p14_json_parse_value(p14_json_doc *doc, const char **cursor)
{
    const char *p;
    size_t token_index;
    char closing;

    p14_json_skip_space(cursor);
    p = *cursor;
    if (*p == '\0' || doc->count >= doc->capacity) {
        return (size_t)-1;
    }

    token_index = doc->count++;
    doc->tokens[token_index].start = (size_t)(p - doc->text);
    doc->tokens[token_index].first_child = (size_t)-1;

    if (*p == '{' || *p == '[') {
        int object = (*p == '{');
        closing = object ? '}' : ']';
        doc->tokens[token_index].kind = object ? P14_JSON_OBJECT : P14_JSON_ARRAY;
        ++p;
        p14_json_skip_space(&p);
        while (*p != closing && *p != '\0') {
            size_t child;
            if (object) {
                size_t key;
                if (*p != '"') return (size_t)-1;
                key = p14_json_parse_value(doc, &p);
                if (key == (size_t)-1) return key;
                if (doc->tokens[token_index].first_child == (size_t)-1) {
                    doc->tokens[token_index].first_child = key;
                }
                p14_json_skip_space(&p);
                if (*p++ != ':') return (size_t)-1;
                child = p14_json_parse_value(doc, &p);
                if (child == (size_t)-1) return child;
            } else {
                child = p14_json_parse_value(doc, &p);
                if (child == (size_t)-1) return child;
                if (doc->tokens[token_index].first_child == (size_t)-1) {
                    doc->tokens[token_index].first_child = child;
                }
            }
            p14_json_skip_space(&p);
            if (*p == ',') {
                ++p;
                p14_json_skip_space(&p);
            } else if (*p != closing) {
                return (size_t)-1;
            }
        }
        if (*p++ != closing) return (size_t)-1;
        *cursor = p;
    } else if (*p == '"') {
        doc->tokens[token_index].kind = P14_JSON_STRING;
        ++p;
        while (*p != '\0') {
            if (*p == '\\') {
                if (p[1] == '\0') return (size_t)-1;
                p += 2;
            } else if (*p == '"') {
                ++p;
                break;
            } else {
                ++p;
            }
        }
        if (p[-1] != '"') return (size_t)-1;
        *cursor = p;
    } else {
        doc->tokens[token_index].kind = P14_JSON_PRIMITIVE;
        while (*p != '\0' && *p != ',' && *p != ']' && *p != '}' &&
               !isspace((unsigned char)*p)) {
            ++p;
        }
        if (p == doc->text + doc->tokens[token_index].start) return (size_t)-1;
        *cursor = p;
    }

    doc->tokens[token_index].end = (size_t)(*cursor - doc->text);
    doc->tokens[token_index].next = doc->count;
    return token_index;
}

static int p14_json_parse(p14_json_doc *doc, const char *text)
{
    const char *cursor = text;
    size_t len;
    size_t root;

    if (doc == NULL || text == NULL) return 0;
    len = strlen(text);
    doc->text = text;
    doc->tokens = (p14_json_token *)calloc(len / 2 + 8, sizeof(*doc->tokens));
    if (doc->tokens == NULL) return 0;
    doc->count = 0;
    doc->capacity = len / 2 + 8;
    root = p14_json_parse_value(doc, &cursor);
    p14_json_skip_space(&cursor);
    if (root != 0 || cursor == NULL || *cursor != '\0') {
        free(doc->tokens);
        doc->tokens = NULL;
        doc->count = 0;
        return 0;
    }
    return 1;
}

static void p14_json_destroy(p14_json_doc *doc)
{
    if (doc != NULL) {
        free(doc->tokens);
        doc->tokens = NULL;
        doc->count = 0;
        doc->capacity = 0;
    }
}

static int p14_json_string_equals(const p14_json_doc *doc, size_t token,
                                  const char *value)
{
    size_t length;
    if (token >= doc->count || doc->tokens[token].kind != P14_JSON_STRING) return 0;
    length = doc->tokens[token].end - doc->tokens[token].start - 2;
    return strlen(value) == length &&
        memcmp(doc->text + doc->tokens[token].start + 1, value, length) == 0;
}

static size_t p14_json_object_get(const p14_json_doc *doc, size_t object,
                                  const char *key)
{
    size_t item;
    if (object >= doc->count || doc->tokens[object].kind != P14_JSON_OBJECT) {
        return (size_t)-1;
    }
    item = doc->tokens[object].first_child;
    while (item != (size_t)-1 && item < doc->count &&
           doc->tokens[item].start < doc->tokens[object].end - 1) {
        size_t value = doc->tokens[item].next;
        if (value >= doc->count) return (size_t)-1;
        if (p14_json_string_equals(doc, item, key)) return value;
        item = doc->tokens[value].next;
    }
    return (size_t)-1;
}

static size_t p14_json_array_get(const p14_json_doc *doc, size_t array,
                                 size_t index)
{
    size_t item;
    if (array >= doc->count || doc->tokens[array].kind != P14_JSON_ARRAY) {
        return (size_t)-1;
    }
    item = doc->tokens[array].first_child;
    while (item != (size_t)-1 && item < doc->count &&
           doc->tokens[item].start < doc->tokens[array].end - 1 && index != 0) {
        item = doc->tokens[item].next;
        --index;
    }
    if (item == (size_t)-1 || item >= doc->count ||
        doc->tokens[item].start >= doc->tokens[array].end - 1) return (size_t)-1;
    return item;
}

static double p14_json_number(const p14_json_doc *doc, size_t token,
                              double fallback)
{
    char buffer[96];
    size_t n;
    char *end;
    double result;
    if (token >= doc->count || doc->tokens[token].kind == P14_JSON_OBJECT ||
        doc->tokens[token].kind == P14_JSON_ARRAY) return fallback;
    n = doc->tokens[token].end - doc->tokens[token].start;
    if (doc->tokens[token].kind == P14_JSON_STRING) {
        if (n < 2 || n - 2 >= sizeof(buffer)) return fallback;
        memcpy(buffer, doc->text + doc->tokens[token].start + 1, n - 2);
        buffer[n - 2] = '\0';
    } else {
        if (n >= sizeof(buffer)) return fallback;
        memcpy(buffer, doc->text + doc->tokens[token].start, n);
        buffer[n] = '\0';
    }
    errno = 0;
    result = strtod(buffer, &end);
    return errno == 0 && end != buffer && *end == '\0' ? result : fallback;
}

static int p14_json_array_to_doubles(const p14_json_doc *doc, size_t array,
                                    double *values, size_t capacity,
                                    size_t *written)
{
    size_t item;
    size_t count = 0;
    if (array >= doc->count || doc->tokens[array].kind != P14_JSON_ARRAY) return 0;
    item = doc->tokens[array].first_child;
    while (item != (size_t)-1 && item < doc->count &&
           doc->tokens[item].start < doc->tokens[array].end - 1) {
        if (doc->tokens[item].kind != P14_JSON_PRIMITIVE || count >= capacity) return 0;
        values[count++] = p14_json_number(doc, item, 0.0);
        item = doc->tokens[item].next;
    }
    if (written != NULL) *written = count;
    return 1;
}

#endif
