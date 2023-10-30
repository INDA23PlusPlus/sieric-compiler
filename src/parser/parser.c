#include <parser/parser.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>

static ast_node_fn_defn_t *parser_parse_fn_defn(parser_t *);
static ast_node_stmt_decl_t *parser_parse_stmt_decl(parser_t *);
static ast_node_stmt_expr_t *parser_parse_stmt_expr(parser_t *);
static ast_node_stmt_if_t *parser_parse_stmt_if(parser_t *);
static ast_node_stmt_ret_t *parser_parse_stmt_ret(parser_t *);
static ast_node_stmt_block_t *parser_parse_stmt_block(parser_t *);
static int parser_parse_block(parser_t *, vec_t *);

static ast_node_t *parser_parse_expr(parser_t *);
static ast_node_t *parser_parse_expr_lor(parser_t *);
static ast_node_t *parser_parse_expr_land(parser_t *);
static ast_node_t *parser_parse_expr_bitor(parser_t *);
static ast_node_t *parser_parse_expr_bitxor(parser_t *);
static ast_node_t *parser_parse_expr_bitand(parser_t *);
static ast_node_t *parser_parse_expr_eq(parser_t *);
static ast_node_t *parser_parse_expr_rel(parser_t *);
static ast_node_t *parser_parse_expr_add(parser_t *);
static ast_node_t *parser_parse_expr_mult(parser_t *);
static ast_node_t *parser_parse_expr_unary(parser_t *);
static ast_node_t *parser_parse_expr_postfix(parser_t *);

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
    if(p->error) {
        ast_free((void *)tu);
        return NULL;
    }

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
    node->ident = ast_node_ident_new(token);
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
    token_t prev;
    memcpy(&prev, &p->lexer->token, sizeof(token_t));
    if(parser_eat(p, '=')) return NULL;

    ast_node_stmt_decl_t *node = malloc(sizeof(ast_node_stmt_decl_t));
    node->hdr.type = AST_STMT_DECL;
    node->ident = ast_node_ident_new(&prev);
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
    node->branch_true = vec_new_free(1, (vec_free_t)ast_free);
    node->branch_false = vec_new_free(1, (vec_free_t)ast_free);

    if(parser_parse_block(p, node->branch_true)) goto ret_free;
    if(lexer_next(p->lexer)->type != TELSE) lexer_unget(p->lexer);
    else switch(lexer_next(p->lexer)->type) {
    case TIF: {
        lexer_unget(p->lexer);

        ast_node_stmt_if_t *else_if = parser_parse_stmt_if(p);
        if(!else_if) goto ret_free;
        vec_push(node->branch_false, else_if);
        break;
    }

    case '{':
        lexer_unget(p->lexer);

        if(parser_parse_block(p, node->branch_false)) goto ret_free;
        break;

    default:
        goto ret_free;
    }

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

static ast_node_t *parser_parse_expr(parser_t *p) {
    return parser_parse_expr_lor(p);
}

static inline ast_node_t *parser_parse_expr_lor(parser_t *p) {
    ast_node_t *left = parser_parse_expr_land(p);
    if(!left) return NULL;
    while(lexer_next(p->lexer)->type == TLOR_OP) {
        ast_node_t *right = parser_parse_expr_land(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, EXPR_LOR);
    }
    lexer_unget(p->lexer);
    return left;
}

static ast_node_t *parser_parse_expr_land(parser_t *p) {
    ast_node_t *left = parser_parse_expr_bitor(p);
    if(!left) return NULL;
    while(lexer_next(p->lexer)->type == TLAND_OP) {
        ast_node_t *right = parser_parse_expr_bitor(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, EXPR_LAND);
    }
    lexer_unget(p->lexer);
    return left;
}

static ast_node_t *parser_parse_expr_bitor(parser_t *p) {
    ast_node_t *left = parser_parse_expr_bitxor(p);
    if(!left) return NULL;
    while(lexer_next(p->lexer)->type == '|') {
        ast_node_t *right = parser_parse_expr_bitxor(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, EXPR_BITOR);
    }
    lexer_unget(p->lexer);
    return left;
}

static ast_node_t *parser_parse_expr_bitxor(parser_t *p) {
    ast_node_t *left = parser_parse_expr_bitand(p);
    if(!left) return NULL;
    while(lexer_next(p->lexer)->type == '^') {
        ast_node_t *right = parser_parse_expr_bitand(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, EXPR_BITXOR);
    }
    lexer_unget(p->lexer);
    return left;
}

static ast_node_t *parser_parse_expr_bitand(parser_t *p) {
    ast_node_t *left = parser_parse_expr_eq(p);
    if(!left) return NULL;
    while(lexer_next(p->lexer)->type == '&') {
        ast_node_t *right = parser_parse_expr_eq(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, EXPR_BITAND);
    }
    lexer_unget(p->lexer);
    return left;
}

static ast_node_t *parser_parse_expr_eq(parser_t *p) {
    ast_node_t *left = parser_parse_expr_rel(p);
    enum expr_binary_type type;
    if(!left) return NULL;
    for(;;) {
        switch(lexer_next(p->lexer)->type) {
        case TEQ_OP:  type = EXPR_EQ; break;
        case TNEQ_OP: type = EXPR_NEQ; break;

        default:
            lexer_unget(p->lexer);
            return left;
        }
        ast_node_t *right = parser_parse_expr_rel(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, type);
    }
}

static ast_node_t *parser_parse_expr_rel(parser_t *p) {
    ast_node_t *left = parser_parse_expr_add(p);
    enum expr_binary_type type;
    if(!left) return NULL;
    for(;;) {
        switch(lexer_next(p->lexer)->type) {
        case '<':     type = EXPR_LT; break;
        case '>':     type = EXPR_GT; break;
        case TLEQ_OP: type = EXPR_LEQ; break;
        case TGEQ_OP: type = EXPR_GEQ; break;

        default:
            lexer_unget(p->lexer);
            return left;
        }
        ast_node_t *right = parser_parse_expr_add(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, type);
    }
}

static ast_node_t *parser_parse_expr_add(parser_t *p) {
    ast_node_t *left = parser_parse_expr_mult(p);
    enum expr_binary_type type;
    if(!left) return NULL;
    for(;;) {
        switch(lexer_next(p->lexer)->type) {
        case '+': type = EXPR_ADD; break;
        case '-': type = EXPR_SUB; break;

        default:
            lexer_unget(p->lexer);
            return left;
        }
        ast_node_t *right = parser_parse_expr_mult(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, type);
    }
}

static ast_node_t *parser_parse_expr_mult(parser_t *p) {
    ast_node_t *left = parser_parse_expr_unary(p);
    enum expr_binary_type type;
    if(!left) return NULL;
    for(;;) {
        switch(lexer_next(p->lexer)->type) {
        case '*': type = EXPR_MULT; break;
        case '/': type = EXPR_DIV; break;
        case '%': type = EXPR_MOD; break;

        default:
            lexer_unget(p->lexer);
            return left;
        }
        ast_node_t *right = parser_parse_expr_unary(p);
        if(!right) {
            ast_free(left);
            return NULL;
        }
        left = (void *)ast_node_expr_binary_new(left, right, type);
    }
}

static ast_node_t *parser_parse_expr_unary(parser_t *p) {
    enum expr_unary_type type;
    switch(lexer_next(p->lexer)->type) {
    case '~': type = EXPR_BITNOT; break;
    case '!': type = EXPR_LNOT; break;

    default:
        lexer_unget(p->lexer);
        return parser_parse_expr_postfix(p);
    }
    ast_node_t *op = parser_parse_expr_unary(p);
    if(!op) return NULL;

    return (void *)ast_node_expr_unary_new(op, type);
}

/* function calling is the only "postfix" operator */
static ast_node_t *parser_parse_expr_postfix(parser_t *p) {
    token_t *t = lexer_next(p->lexer);
    token_t prev;

    switch(t->type) {
    case TCONSTANT:
        return (void *)ast_node_const_new(t);

    case '(': {
        ast_node_t *expr = parser_parse_expr(p);
        parser_eat(p, ')');
        return expr;
    }

    default:
        fprintf(stderr, "[Error] Expected token 'TCONSTANT', 'TIDENTIFIER', or "
                        "'TLPAREN' but got '%s'\n", token_type_str(t->type));
        p->error = 1;
        return NULL;

    case TIDENTIFIER:
        memcpy(&prev, t, sizeof(token_t));
        if(lexer_next(p->lexer)->type != '(') {
            lexer_unget(p->lexer);
            return (void *)ast_node_ident_new(&prev);
        }
        /* this is actually a function call */
        break;
    }

    ast_node_expr_call_t *node = malloc(sizeof(ast_node_expr_call_t));
    node->hdr.type = AST_EXPR_CALL;
    node->ident = ast_node_ident_new(&prev);
    node->args = vec_new_free(1, (vec_free_t)ast_free);

    if(lexer_next(p->lexer)->type == ')') return (void *)node;
    lexer_unget(p->lexer);
    ast_node_t *expr = parser_parse_expr(p);
    if(!expr) goto ret_free;
    vec_push(node->args, expr);

    while(lexer_next(p->lexer)->type != ')') {
        lexer_unget(p->lexer);
        if(parser_eat(p, ',')) goto ret_free;
        if(!(expr = parser_parse_expr(p))) goto ret_free;
        vec_push(node->args, expr);
    }

    return (void *)node;
ret_free:
    ast_free((void *)node);
    return NULL;
}
