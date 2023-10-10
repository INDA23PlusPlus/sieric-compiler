#include <parser/lexer.h>

#include <stdlib.h>

lexer_t *lexer_create(const char *buf, size_t sz) {
    lexer_t *lexer = malloc(sizeof(lexer_t));
    lexer->start = buf;
    lexer->end = buf + sz;
    return lexer;
}

void lexer_destroy(lexer_t *l) {
    free(l);
}

token_t *lexer_next(lexer_t *l) {
    if(l->start++ > l->end) return token_create(NULL, 0, TEOF);
    return token_create(l->start, 1, TUNKNOWN);
}
