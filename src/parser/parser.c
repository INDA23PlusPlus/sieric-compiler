#include <parser/parser.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static ast_node_fn_defn_t *parser_parse_fn_defn(parser_t *);
static ast_node_stmt_decl_t *parser_parse_stmt_decl(parser_t *);
static ast_node_stmt_expr_t *parser_parse_stmt_expr(parser_t *);
static ast_node_stmt_if_t *parser_parse_stmt_if(parser_t *);
static ast_node_stmt_ret_t *parser_parse_stmt_ret(parser_t *);
static ast_node_stmt_block_t *parser_parse_stmt_block(parser_t *);
static ast_node_t *parser_parse_expr(parser_t *);
static int parser_parse_block(parser_t *, vec_t *);

static ast_node_ident_t *ast_node_ident_new(token_t *);
static ast_node_const_t *ast_node_const_new(token_t *);

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

    ast_node_fn_defn_t *fn;
    while((fn = parser_parse_fn_defn(p))) vec_push(tu->functions, fn);

    return (ast_node_t *)tu;
}

void parser_free(parser_t *p) {
    if(p->root) ast_free(p->root);
    free(p);
}

int parser_eat(parser_t *p, enum token_type type) {
    token_t *token = lexer_next(p->lexer);
    if(token->type != type) {
        fprintf(stderr, "[Error] Expected token '%s' but got '%s'\n",
                token_type_str(type), token_type_str(token->type));
        return p->error = 1;
    }
    return 0;
}

/***** AST node parsers *****/

static ast_node_fn_defn_t *parser_parse_fn_defn(parser_t *p) {
    token_t *token = lexer_next(p->lexer);

    switch(token->type) {
    default:
        /* HACK: reuse error message from parser_eat */
        lexer_unget(p->lexer);
        parser_eat(p, TFN);
        __attribute__((fallthrough));
    case TEOF: return NULL;
    case TFN: break;
    }

    if(parser_eat(p, TIDENTIFIER)) return NULL;

    ast_node_fn_defn_t *node = malloc(sizeof(ast_node_fn_defn_t));
    node->hdr.type = AST_FN_DEFN;
    node->name_sz = token->sz;
    node->name = strndup((char *)token->start, token->sz);
    node->arguments = vec_new_free(1, (vec_free_t)ast_free);
    node->body = vec_new_free(1, (vec_free_t)ast_free);

    if(parser_eat(p, '(')) goto ret_free;

    lexer_next(p->lexer);
    if(token->type == ')');
    else if(token->type == TIDENTIFIER) {
        for(;;) {
            vec_push(node->arguments, ast_node_ident_new(token));
            lexer_next(p->lexer);
            if(token->type == ')') break;
            else if(token->type == ',') {
                if(parser_eat(p, TIDENTIFIER)) goto ret_free;
                continue;
            } else {
                fprintf(stderr, "[Error] Expected token 'TCOMMA' or 'TRPAREN' "
                        "but got '%s'\n",
                        token_type_str(token->type));
                p->error = 1;
                goto ret_free;
            }
        }
    } else {
        fprintf(stderr, "[Error] Expected token 'TIDENTIFIER' or 'TRPAREN' but "
                "got '%s'\n",
                token_type_str(token->type));
        p->error = 1;
        goto ret_free;
    }

    if(parser_parse_block(p, node->body)) goto ret_free;

    return node;
ret_free:
    ast_free((void *)node);
    return NULL;
}

static ast_node_stmt_decl_t *parser_parse_stmt_decl(parser_t *p) {
    if(parser_eat(p, TLET)) return NULL;
    if(parser_eat(p, TIDENTIFIER)) return NULL;
    if(parser_eat(p, '=')) return NULL;

    ast_node_stmt_decl_t *node = malloc(sizeof(ast_node_stmt_decl_t));
    node->hdr.type = AST_STMT_DECL;
    node->ident = ast_node_ident_new(&p->lexer->token);
    node->expr = parser_parse_expr(p);
    if(!node->expr) goto ret_free;
    if(parser_eat(p, ';')) goto ret_free;

    return node;
ret_free:
    ast_free((void *)node);
    return NULL;
}

static ast_node_stmt_expr_t *parser_parse_stmt_expr(parser_t *p) {
    ast_node_stmt_expr_t *node = malloc(sizeof(ast_node_stmt_expr_t));
    node->hdr.type = AST_STMT_EXPR;
    node->expr = parser_parse_expr(p);
    if(!node->expr) {
        free(node);
        return NULL;
    }
    if(parser_eat(p, ';')) goto ret_free;

    return node;
ret_free:
    ast_free((void *)node);
    return NULL;
}

static ast_node_stmt_if_t *parser_parse_stmt_if(parser_t *p) {
    if(parser_eat(p, TIF)) return NULL;

    ast_node_stmt_if_t *node = malloc(sizeof(ast_node_stmt_if_t));
    node->hdr.type = AST_STMT_IF;
    node->condition = parser_parse_expr(p);
    if(!node->condition) {
        free(node);
        return NULL;
    }
    node->branch_true = vec_new(1);
    node->branch_false = vec_new(1);

    if(parser_parse_block(p, node->branch_true)) goto ret_free;
    if(lexer_next(p->lexer)->type != TELSE) lexer_unget(p->lexer);
    else if(parser_parse_block(p, node->branch_false)) goto ret_free;

    return node;
ret_free:
    ast_free((void *)node);
    return NULL;
}

static ast_node_stmt_ret_t *parser_parse_stmt_ret(parser_t *p) {
    if(parser_eat(p, TRETURN)) return NULL;

    ast_node_stmt_ret_t *node = malloc(sizeof(ast_node_stmt_ret_t));
    node->hdr.type = AST_STMT_RET;
    node->expr = parser_parse_expr(p);
    if(!node->expr) {
        free(node);
        return NULL;
    }
    if(parser_eat(p, ';')) goto ret_free;

    return node;
ret_free:
    ast_free((void *)node);
    return NULL;
}

static ast_node_stmt_block_t *parser_parse_stmt_block(parser_t *p) {
    ast_node_stmt_block_t *node = malloc(sizeof(ast_node_stmt_block_t));
    node->hdr.type = AST_STMT_BLOCK;
    node->stmts = vec_new_free(1, (vec_free_t)ast_free);
    if(parser_parse_block(p, node->stmts)) {
        vec_free(node->stmts);
        free(node);
        return NULL;
    }
    return (void *)node;
}

/* NOTE: placeholder */
static ast_node_t *parser_parse_expr(parser_t *p) {
    token_t *token = &p->lexer->token;
    enum token_type valid[] = {
        '(', ')', '+', '-', '*', '/', '%', '<', '>', '&', '|', '^', '~', '!',
        TLEQ_OP, TGEQ_OP, TEQ_OP, TNEQ_OP, TLAND_OP, TLOR_OP,
        TIDENTIFIER, TCONSTANT,
        ',', /* even more HACK */
    };
    for(;;) {
        lexer_next(p->lexer);
        int found = 0;
        for(size_t i = 0; i < sizeof valid / sizeof *valid; ++i)
            if(token->type == valid[i]) {
                found = 1;
                break;
            }
        if(!found) break;
    }
    lexer_unget(p->lexer);
    return (void *)ast_node_ident_new(token);
}

static int parser_parse_block(parser_t *p, vec_t *vec) {
    {
        int err = 0;
        if((err = parser_eat(p, '{'))) return err;
    }

    for(;;) {
        token_t *token = lexer_next(p->lexer);
        switch(token->type) {
        case '}': return 0;

        case TLET: {
            lexer_unget(p->lexer);
            ast_node_t *node = (void *)parser_parse_stmt_decl(p);
            if(node) vec_push(vec, node);
            else return p->error = 1;
            break;
        }

        case TIF: {
            lexer_unget(p->lexer);
            ast_node_t *node = (void *)parser_parse_stmt_if(p);
            if(node) vec_push(vec, node);
            else return p->error = 1;
            break;
        }

        case TRETURN: {
            lexer_unget(p->lexer);
            ast_node_t *node = (void *)parser_parse_stmt_ret(p);
            if(node) vec_push(vec, node);
            else return p->error = 1;
            break;
        }

        case '{': {
            lexer_unget(p->lexer);
            ast_node_t *node = (void *)parser_parse_stmt_block(p);
            if(node) vec_push(vec, node);
            else return p->error = 1;
            break;
        }

        case TIDENTIFIER: case TCONSTANT:
        case '!': case '~': case '(': case ';': {
            lexer_unget(p->lexer);
            ast_node_t *node = (void *)parser_parse_stmt_expr(p);
            if(node) vec_push(vec, node);
            else return p->error = 1;
            break;
        }

        default:
            fprintf(stderr, "[Error] Unexpected token '%s' in code block\n",
                    token_type_str(token->type));
            return p->error = 1;
        }
    }
}

/***** AST functions *****/

static ast_node_ident_t *ast_node_ident_new(token_t *token) {
    ast_node_ident_t *node = malloc(sizeof(ast_node_ident_t));
    node->hdr.type = AST_IDENT;
    node->name_sz = token->sz;
    node->name = strndup((char *)token->start, token->sz);
    return node;
}

static ast_node_const_t *ast_node_const_new(token_t *token) {
    ast_node_const_t *node = malloc(sizeof(ast_node_ident_t));
    node->hdr.type = AST_IDENT;
    {
        char buf[token->sz+1];
        memcpy(buf, token->start, token->sz);
        buf[token->sz] = '\0';
        errno = 0;
        node->value = strtoull(buf, NULL, 10);
        if(errno) perror("strtoull");
    }
    return node;
}

/* TODO */
static void ast_free(ast_node_t *node) {
    printf("freeing node type %d at %p\n", node->type, node);
}

void ast_print(ast_node_t *root) {
    ast_print_internal(root, 0);
}

static void ast_print_internal(ast_node_t *root, size_t n) {
    char pad[n+1];
    for(size_t i = 0; i < n; ++i) pad[i] = ' ';
    pad[n] = '\0';

    switch(root->type) {
    case AST_TU: {
        ast_node_tu_t *node = (void *)root;
        printf("%sTU\n", pad);
        for(size_t i = 0; i < node->functions->sz; ++i)
            ast_print_internal(vec_get(node->functions, i), n+1);
        break;
    }

    case AST_FN_DEFN: {
        ast_node_fn_defn_t *node = (void *)root;
        printf("%sFunction[%.*s(", pad, (int)node->name_sz, node->name);
        for(size_t i = 0; i < node->arguments->sz; ++i) {
            ast_node_ident_t *arg = vec_get(node->arguments, i);
            printf("%.*s,", (int)arg->name_sz, arg->name);
        }
        printf(")]\n");
        for(size_t i = 0; i < node->body->sz; ++i)
            ast_print_internal(vec_get(node->body, i), n+1);
        break;
    }

    /* TODO */
    default:
        printf("%swhatever\n", pad);
        break;
    }
}
