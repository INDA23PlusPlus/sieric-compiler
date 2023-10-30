#include <parser/parser.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>

static void pad_printf(size_t, const char *restrict, ...);
static void ast_print_internal(ast_node_t *root, size_t n);

ast_node_ident_t *ast_node_ident_new(token_t *token) {
    ast_node_ident_t *node = malloc(sizeof(ast_node_ident_t));
    node->hdr.type = AST_IDENT;
    node->name_sz = token->sz;
    node->name = strndup((char *)token->start, token->sz);
    return node;
}

ast_node_const_t *ast_node_const_new(token_t *token) {
    ast_node_const_t *node = malloc(sizeof(ast_node_ident_t));
    node->hdr.type = AST_CONST;
    {
        char buf[token->sz+1];
        memcpy(buf, token->start, token->sz);
        buf[token->sz] = '\0';
        errno = 0;
        node->value = strtoll(buf, NULL, 10);
        if(errno) perror("strtoll");
    }
    return node;
}

ast_node_expr_binary_t *ast_node_expr_binary_new(
    ast_node_t *left, ast_node_t *right, enum expr_binary_type type) {
    ast_node_expr_binary_t *node = malloc(sizeof(ast_node_expr_binary_t));
    node->hdr.type = AST_EXPR_BINARY;
    node->left = left;
    node->right = right;
    node->type = type;
    return node;
}

ast_node_expr_unary_t *ast_node_expr_unary_new(
    ast_node_t *op, enum expr_unary_type type) {
    ast_node_expr_unary_t *node = malloc(sizeof(ast_node_expr_unary_t));
    node->hdr.type = AST_EXPR_UNARY;
    node->op = op;
    node->type = type;
    return node;
}

void ast_free(ast_node_t *root) {
    switch(root->type) {
    case AST_TU: {
        ast_node_tu_t *node = (void *)root;
        vec_free(node->functions);
        break;
    }

    case AST_FN_DEFN: {
        ast_node_fn_defn_t *node = (void *)root;
        vec_free(node->arguments);
        vec_free(node->body);
        ast_free((void *)node->ident);
        break;
    }

    case AST_STMT_DECL: {
        ast_node_stmt_decl_t *node = (void *)root;
        ast_free(node->expr);
        ast_free((void *)node->ident);
        break;
    }

    case AST_STMT_EXPR: {
        ast_node_stmt_expr_t *node = (void *)root;
        ast_free(node->expr);
        break;
    }

    case AST_STMT_IF: {
        ast_node_stmt_if_t *node = (void *)root;
        vec_free(node->branch_true);
        vec_free(node->branch_false);
        ast_free(node->condition);
        break;
    }

    case AST_STMT_RET: {
        ast_node_stmt_ret_t *node = (void *)root;
        ast_free(node->expr);
        break;
    }

    case AST_STMT_BLOCK: {
        ast_node_stmt_block_t *node = (void *)root;
        vec_free(node->stmts);
        break;
    }

    case AST_EXPR_BINARY: {
        ast_node_expr_binary_t *node = (void *)root;
        ast_free(node->left);
        ast_free(node->right);
        break;
    }

    case AST_EXPR_UNARY: {
        ast_node_expr_unary_t *node = (void *)root;
        ast_free(node->op);
        break;
    }

    case AST_EXPR_CALL: {
        ast_node_expr_call_t *node = (void *)root;
        vec_free(node->args);
        ast_free((void *)node->ident);
        break;
    }

    case AST_IDENT: {
        ast_node_ident_t *node = (void *)root;
        free(node->name);
        break;
    }

    case AST_CONST: break;
    }
    free(root);
}

static void pad_printf(size_t n, const char *restrict fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    for(size_t i = 0; i < 2*n; ++i) putchar(' ');
    vprintf(fmt, ap);
    va_end(ap);
}

void ast_print(ast_node_t *root) {
    ast_print_internal(root, 0);
}

static void ast_print_internal(ast_node_t *root, size_t n) {
    char pad[2*n+1];
    for(size_t i = 0; i < 2*n; ++i) pad[i] = ' ';
    pad[2*n] = '\0';

    switch(root->type) {
    case AST_TU: {
        ast_node_tu_t *node = (void *)root;
        printf("%sTranslationUnit\n", pad);
        for(size_t i = 0; i < node->functions->sz; ++i)
            ast_print_internal(vec_get(node->functions, i), n+1);
        break;
    }

    case AST_FN_DEFN: {
        ast_node_fn_defn_t *node = (void *)root;
        printf("%sFunction[%.*s(",
               pad, (int)node->ident->name_sz, node->ident->name);
        for(size_t i = 0; i < node->arguments->sz; ++i) {
            ast_node_ident_t *arg = vec_get(node->arguments, i);
            printf("%.*s,", (int)arg->name_sz, arg->name);
        }
        printf(")]\n");
        for(size_t i = 0; i < node->body->sz; ++i)
            ast_print_internal(vec_get(node->body, i), n+1);
        break;
    }

    case AST_STMT_DECL: {
        ast_node_stmt_decl_t *node = (void *)root;
        printf("%sDeclaration[%.*s]\n",
               pad, (int)node->ident->name_sz, node->ident->name);
        ast_print_internal(node->expr, n+1);
        break;
    }

    case AST_STMT_EXPR: {
        ast_node_stmt_expr_t *node = (void *)root;
        printf("%sExpressionStatement\n", pad);
        ast_print_internal(node->expr, n+1);
        break;
    }

    case AST_STMT_IF: {
        ast_node_stmt_if_t *node = (void *)root;
        printf("%sIfStatement\n", pad);

        pad_printf(n+1, "Condition\n");
        ast_print_internal(node->condition, n+2);

        pad_printf(n+1, "TrueBranch\n");
        for(size_t i = 0; i < node->branch_true->sz; ++i)
            ast_print_internal(vec_get(node->branch_true, i), n+2);

        pad_printf(n+1, "FalseBranch\n");
        for(size_t i = 0; i < node->branch_false->sz; ++i)
            ast_print_internal(vec_get(node->branch_false, i), n+2);

        break;
    }

    case AST_STMT_RET: {
        ast_node_stmt_ret_t *node = (void *)root;
        printf("%sReturnStatement\n", pad);
        ast_print_internal(node->expr, n+1);
        break;
    }

    case AST_STMT_BLOCK: {
        ast_node_stmt_block_t *node = (void *)root;
        printf("%sBlockStatement\n", pad);
        for(size_t i = 0; i < node->stmts->sz; ++i)
            ast_print_internal(vec_get(node->stmts, i), n+1);
        break;
    }

    case AST_EXPR_BINARY: {
        ast_node_expr_binary_t *node = (void *)root;
        static const char *ops[] = {
        [EXPR_LOR] = "||",
        [EXPR_LAND] = "&&",
        [EXPR_BITOR] = "|",
        [EXPR_BITXOR] = "^",
        [EXPR_BITAND] = "&",
        [EXPR_EQ] = "==",
        [EXPR_NEQ] = "!=",
        [EXPR_LT] = "<",
        [EXPR_GT] = ">",
        [EXPR_LEQ] = "<=",
        [EXPR_GEQ] = ">=",
        [EXPR_ADD] = "+",
        [EXPR_SUB] = "-",
        [EXPR_MULT] = "*",
        [EXPR_DIV] = "/",
        [EXPR_MOD] = "%",
        };
        printf("%sBinaryOperation[%s]\n", pad, ops[node->type]);

        pad_printf(n+1, "Left\n");
        ast_print_internal(node->left, n+2);

        pad_printf(n+1, "Right\n");
        ast_print_internal(node->right, n+2);
        break;
    }

    case AST_EXPR_UNARY: {
        ast_node_expr_unary_t *node = (void *)root;
        static const char *ops[] = {
        [EXPR_LNOT] = "!",
        [EXPR_BITNOT] = "~",
        };
        printf("%sUnaryOperation[%s]\n", pad, ops[node->type]);
        ast_print_internal(node->op, n+1);
        break;
    }

    case AST_EXPR_CALL: {
        ast_node_expr_call_t *node = (void *)root;
        printf("%sFunctionCall[%.*s]\n",
               pad, (int)node->ident->name_sz, node->ident->name);
        for(size_t i = 0; i < node->args->sz; ++i)
            ast_print_internal(vec_get(node->args, i), n+1);
        break;
    }

    case AST_IDENT: {
        ast_node_ident_t *node = (void *)root;
        printf("%sIdentifier[%.*s]\n", pad, (int)node->name_sz, node->name);
        break;
    }

    case AST_CONST: {
        ast_node_const_t *node = (void *)root;
        printf("%sConstant[%"PRId64"]\n", pad, node->value);
        break;
    }

    default:
        printf("%sUnknownNode\n", pad);
        break;
    }
}
