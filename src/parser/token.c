#include <parser/token.h>
#include <stdlib.h>

static const struct tokentypestr {
#define T(t, v) char str##t[sizeof(#t)];
#include <parser/__tokens.h>
#undef T
} tokentypestr = {
#define T(t, v) #t,
#include <parser/__tokens.h>
#undef T
};

static const unsigned short tokentypeidx[] = {
#define T(t, v) [(v)] = offsetof(struct tokentypestr, str##t),
#include <parser/__tokens.h>
#undef T
};

token_t *token_create(const char *start, size_t sz, enum token_type type) {
    token_t *token = malloc(sizeof(token_t));

    token->start = start;
    token->sz = sz;
    token->type = type;

    return token;
}

void token_destroy(token_t *token) {
    free(token);
}

const char *token_type_str(enum token_type type) {
    if((size_t)type >= sizeof tokentypeidx / sizeof *tokentypeidx) type = 0;
    return (char *)&tokentypestr + tokentypeidx[type];
}
