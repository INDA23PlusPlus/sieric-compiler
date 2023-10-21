#ifndef PARSER_PARSER_H_
#define PARSER_PARSER_H_

#include <parser/lexer.h>
#include <parser/ast.h>

typedef struct parser {
    lexer_t *lexer;
    ast_node_t *root;
} parser_t;

parser_t *parser_new(lexer_t *);
ast_node_t *parser_parse(parser_t *);
void parser_free(parser_t *);

void ast_print(ast_node_t *);

#endif /* PARSER_PARSER_H_ */
