#include <parser/parser.h>
#include <stdlib.h>
#include <string.h>

static ast_node_t *parser_parse_fn_defn(parser_t *);

static void ast_free(ast_node_t *);
static void ast_print_internal(ast_node_t *root, size_t n);

/***** parser functions *****/

parser_t *parser_new(lexer_t *lexer) {
    parser_t *parser = malloc(sizeof(parser_t));
    parser->lexer = lexer;
    parser->root = NULL;
    return parser;
}

ast_node_t *parser_parse(parser_t *p) {
    ast_node_tu_t *tu = malloc(sizeof(ast_node_tu_t));
    tu->hdr.type = AST_TU;
    tu->functions = vec_new_free(1, (vec_free_t)ast_free);

    ast_node_t *fn;
    while((fn = parser_parse_fn_defn(p))) vec_push(tu->functions, fn);

    return (ast_node_t *)tu;
}

void parser_free(parser_t *p) {
    if(p->root) ast_free(p->root);
    free(p);
}

/***** AST node parsers *****/

static ast_node_t *parser_parse_fn_defn(parser_t *p) {
    token_t *token = lexer_next(p->lexer);
    switch(token->type) {
    default:
        fprintf(stderr, "Invalid token in file root\n");
        __attribute__((fallthrough));
    case TEOF: return NULL;

    case TFN: {
        lexer_next(p->lexer);
        if(token->type != TIDENTIFIER) {
            fprintf(stderr, "Invalid function name\n");
            return NULL;
        }

        ast_node_fn_defn_t *node = malloc(sizeof(ast_node_fn_defn_t));
        node->hdr.type = AST_FN_DEFN;
        node->name_sz = token->sz;
        node->name = strndup((const char *)token->start, token->sz);
        node->arguments = vec_new(1);
        node->body = vec_new(1);
        return (void *)node;
    }
    }
}

/***** AST functions *****/

/* TODO */
static void ast_free(ast_node_t *node) {
    printf("freeing node type %d at %p\n", node->type, node);
}

void ast_print(ast_node_t *root) {
    ast_print_internal(root, 0);
}

static void ast_print_internal(ast_node_t *root, size_t n) {
    char pad[128] = {' '};
    if(n > sizeof pad) return;
    pad[n] = '\0';

    switch(root->type) {
    case AST_TU: {
        ast_node_tu_t *tu = (void *)root;
        printf("%sTU\n", pad);
        for(size_t i = 0; i < tu->functions->sz; ++i)
            ast_print_internal(vec_get(tu->functions, i), n+1);
        break;
    }

    case AST_FN_DEFN: {
        ast_node_fn_defn_t *fn = (void *)root;
        printf("%sFunction[%.*s]\n", pad, (int)fn->name_sz, fn->name);
        break;
    }

    /* TODO */
    default:
        printf("%swhatever\n", pad);
    }
}
