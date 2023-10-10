#ifndef PARSER_LEXER_H_
#define PARSER_LEXER_H_

#include <stdio.h>
#include <parser/token.h>

typedef struct lexer {
    const char *start, *end;
} lexer_t;

lexer_t *lexer_create(const char *, size_t sz);
void lexer_destroy(lexer_t *);

token_t *lexer_next(lexer_t *);

#endif // PARSER_LEXER_H_
