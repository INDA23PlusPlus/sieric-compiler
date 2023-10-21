#ifndef PARSER_TOKEN_H_
#define PARSER_TOKEN_H_

#include <stddef.h>

enum token_type {
#define T(t, v) t = v,
#include "__tokens.h"
#undef T
};

typedef struct token {
    const unsigned char *start;
    size_t sz;
    enum token_type type;
} token_t;

token_t *token_create(const unsigned char *start, size_t sz, enum token_type type);
const char *token_type_str(enum token_type);

#endif /* PARSER_TOKEN_H_ */
