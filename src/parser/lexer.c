#include <parser/lexer.h>

#include <stdlib.h>
#include <string.h>

/* #define LEXER_DEBUG */

static token_t *lexer_read_identifier(lexer_t *l);
static token_t *lexer_read_constant(lexer_t *l);

lexer_t *lexer_new(const unsigned char *buf, size_t sz) {
    lexer_t *lexer = malloc(sizeof(lexer_t));
    lexer->start = buf;
    lexer->end = buf + sz;
    lexer->unget = 0;
    memset(&lexer->token, 0, sizeof(token_t));
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
#ifdef LEXER_DEBUG
    printf("\033[31mlexer_unget\033[m: %s\n", token_type_str(l->token.type));
#endif
    l->unget = 1;
    return &l->token;
}

token_t *lexer_next(lexer_t *l) {
    if(l->unget) {
        l->unget = 0;
        return &l->token;
    }
    for(; l->start < l->end; l->start++) {
        unsigned char c = *l->start;
        if(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_')
            return lexer_read_identifier(l);
        if('0' <= c && c <= '9')
            return lexer_read_constant(l);

        switch(c) {

/* HACK: really dumb macro to increment l->start */
#ifdef LEXER_DEBUG
#define T(t, sz) printf("\033[31mlexer_next\033[m: %s\n", token_type_str(t)), l->start += sz, token_init(&l->token, l->start - sz, sz, t)
#else
#define T(t, sz) l->start += sz, token_init(&l->token, l->start - sz, sz, t)
#endif
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
    size_t len;
    enum token_type type;
} keywords[] = {
#define T(str, tk) {(str), sizeof(str)-1, (tk)},
T("let", TLET)
T("if", TIF)
T("else", TELSE)
T("fn", TFN)
T("return", TRETURN)
#undef T
};

#ifdef LEXER_DEBUG
#define T(tk, type) printf("\033[31mlexer_next\033[m: %s\n", token_type_str(type)), token_init(tk, orig, sz, type)
#else
#define T(tk, type) token_init(tk, orig, sz, type)
#endif
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
        if(sz == keywords[i].len && !memcmp(keywords[i].str, orig, sz))
            return T(&l->token, keywords[i].type);

    return T(&l->token, TIDENTIFIER);
}

static token_t *lexer_read_constant(lexer_t *l) {
    size_t sz = 1;
    const unsigned char *orig = l->start++;
    for(; l->start < l->end
       && ('0' <= *l->start && *l->start <= '9'); l->start++, sz++);

    return T(&l->token, TCONSTANT);
}
#undef T
