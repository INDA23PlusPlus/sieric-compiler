#ifndef PARSER_PARSER_H_
#define PARSER_PARSER_H_

#include <parser/lexer.h>
#include <parser/ast.h>

typedef struct parser {
    lexer_t *lexer;
    ast_node_t *root;
    int error;
} parser_t;

parser_t *parser_new(lexer_t *);
ast_node_tu_t *parser_parse(parser_t *);
void parser_free(parser_t *);

int parser_eat(parser_t *, enum token_type);

#endif /* PARSER_PARSER_H_ */
