#ifndef PARSER_TOKEN_H_
#define PARSER_TOKEN_H_

#include <stddef.h>

enum token_type {
#define T(t, v) t = v,
#include "__tokens.h"
#undef T
};

typedef struct token {
    const char *start;
    size_t sz;
    enum token_type type;
} token_t;

token_t *token_create(const char *, size_t, enum token_type);
void token_destroy(token_t *);

const char *token_type_str(enum token_type);

#endif // PARSER_TOKEN_H_
