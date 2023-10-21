#ifndef PARSER_LEXER_H_
#define PARSER_LEXER_H_

#include <stdio.h>
#include <parser/token.h>

typedef struct lexer {
    const unsigned char *start, *end;
    token_t *token;
} lexer_t;

lexer_t *lexer_new(const unsigned char *, size_t);
void lexer_free(lexer_t *);

token_t *lexer_next(lexer_t *);
int lexer_peek(lexer_t *, size_t);

#endif // PARSER_LEXER_H_
