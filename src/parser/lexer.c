#include <parser/lexer.h>

#include <stdlib.h>
#include <string.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static token_t *lexer_read_identifier(lexer_t *l);
static token_t *lexer_read_constant(lexer_t *l);

lexer_t *lexer_new(const unsigned char *buf, size_t sz) {
    lexer_t *lexer = malloc(sizeof(lexer_t));
    lexer->start = buf;
    lexer->end = buf + sz;
    memset(&lexer->token, 0, 2*sizeof(token_t));
    return lexer;
}

void lexer_free(lexer_t *l) {
    free(l);
}

int lexer_peek(lexer_t *l, size_t off) {
    if(l->start + off < l->end) return l->start[off];
    else return -1;
}

/* UB to call multiple times */
token_t *lexer_unget(lexer_t *l) {
    return memcpy(&l->token, &l->prev, sizeof(token_t));
}

token_t *lexer_next(lexer_t *l) {
    memcpy(&l->prev, &l->token, sizeof(token_t));
    for(; l->start < l->end; l->start++) {
        unsigned char c = *l->start;
        if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_')
            return lexer_read_identifier(l);
        if('0' <= c && c <= '9')
            return lexer_read_constant(l);

        switch(c) {

/* HACK: really dumb macro to increment l->start */
#define T(t, sz) l->start += sz, token_init(&l->token, l->start - sz, sz, t)
        case ';': case '{': case '}': case '(': case ')': case ',': case '^':
        case '+': case '-': case '*': case '/': case '%': case '~':
            return T(c, 1);

        case '&':
            if(lexer_peek(l, 1) == '&') return T(TLAND_OP, 2);
            else return T(c, 1);

        case '|':
            if(lexer_peek(l, 1) == '|') return T(TLOR_OP, 2);
            else return T(c, 1);

        case '<':
            if(lexer_peek(l, 1) == '=') return T(TLEQ_OP, 2);
            else return T(c, 1);

        case '>':
            if(lexer_peek(l, 1) == '=') return T(TGEQ_OP, 2);
            else return T(c, 1);

        case '!':
            if(lexer_peek(l, 1) == '=') return T(TNEQ_OP, 2);
            else return T(c, 1);

        case '=':
            if(lexer_peek(l, 1) == '=') return T(TEQ_OP, 2);
            else return T(c, 1);

        case ' ': case '\n': case '\t': case '\v': case '\f':
            break;

        default: return T(TUNKNOWN, 1);
#undef T
        }
    }
    return token_init(&l->token, l->start, 0, TEOF);
}

static const struct {
    unsigned char str[7];
    enum token_type type;
} keywords[] = {
{"let", TLET},
{"if", TIF},
{"else", TELSE},
{"fn", TFN},
{"return", TRETURN},
};

static token_t *lexer_read_identifier(lexer_t *l) {
    size_t sz = 1;
    const unsigned char *orig = l->start++;
    for(; l->start < l->end &&
         (('a' <= *l->start && *l->start <= 'z')
       || ('A' <= *l->start && *l->start <= 'Z')
       || ('0' <= *l->start && *l->start <= '9')
       || (*l->start == '_')); l->start++, sz++);

    /* check if identifier is actually a keyword */
    for(size_t i = 0; i < sizeof keywords / sizeof *keywords; i++)
        if(!memcmp(keywords[i].str, orig, MIN(sz, sizeof keywords[i].str)))
            return token_init(&l->token, orig, sz, keywords[i].type);

    return token_init(&l->token, orig, sz, TIDENTIFIER);
}

static token_t *lexer_read_constant(lexer_t *l) {
    size_t sz = 1;
    const unsigned char *orig = l->start++;
    for(; l->start < l->end
       && ('0' <= *l->start && *l->start <= '9'); l->start++, sz++);

    return token_init(&l->token, orig, sz, TCONSTANT);
}
