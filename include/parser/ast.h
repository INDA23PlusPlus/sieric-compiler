#ifndef PARSER_AST_H_
#define PARSER_AST_H_

/* TODO: change all vectors of statements to block statement nodes to make
 * semantic analysis more clean */

#include <parser/token.h>
#include <utils/vector.h>
#include <stdint.h>

/****** types ******/

enum ast_node_type {
AST_TU,
AST_FN_DEFN,
AST_STMT_DECL,
AST_STMT_EXPR,
AST_STMT_IF,
AST_STMT_RET,
AST_STMT_BLOCK,
AST_EXPR_BINARY,
AST_EXPR_UNARY,
/** the only postfix operator is () */
AST_EXPR_CALL,
AST_IDENT,
AST_CONST,
};

enum expr_binary_type {
EXPR_LOR,
EXPR_LAND,
EXPR_BITOR,
EXPR_BITXOR,
EXPR_BITAND,
EXPR_EQ,
EXPR_NEQ,
EXPR_LT,
EXPR_GT,
EXPR_LEQ,
EXPR_GEQ,
EXPR_ADD,
EXPR_SUB,
EXPR_MULT,
EXPR_DIV,
EXPR_MOD,
};

enum expr_unary_type {
EXPR_LNOT,
EXPR_BITNOT,
};

struct semantics_ctx;
struct scope;

typedef struct ast_node {
    enum ast_node_type type;
} ast_node_t;

typedef struct ast_node_tu {
    ast_node_t hdr;

    /** vector of ast_node_fn_defn_t */
    vec_t *functions;

    struct semantics_ctx *ctx;
} ast_node_tu_t;

typedef struct ast_node_fn_defn {
    ast_node_t hdr;

    struct ast_node_ident *ident;
    /** vector of ast_node_t of statement types */
    vec_t *body;
    /** vector of ast_node_ident_t */
    vec_t *arguments;

    struct scope *scope;
} ast_node_fn_defn_t;

typedef struct ast_node_ident {
    ast_node_t hdr;

    char *name;
    size_t name_sz;
} ast_node_ident_t;

typedef struct ast_node_const {
    ast_node_t hdr;

    int64_t value;
} ast_node_const_t;

typedef struct ast_node_expr_binary {
    ast_node_t hdr;

    enum expr_binary_type type;
    /** ast_node_t of an expression type (or const/ident) */
    struct ast_node *left, *right;
} ast_node_expr_binary_t;

typedef struct ast_node_expr_unary {
    ast_node_t hdr;

    enum expr_unary_type type;
    /** ast_node_t of an expression type (or const/ident) */
    struct ast_node *op;
} ast_node_expr_unary_t;

typedef struct ast_node_expr_call {
    ast_node_t hdr;

    struct ast_node_ident *ident;
    /** vector ast_node_t of expression types (or const/ident) */
    vec_t *args;
} ast_node_expr_call_t;

typedef struct ast_node_stmt_decl {
    ast_node_t hdr;

    struct ast_node_ident *ident;
    /** ast_node_t of an expression type (or const/ident) */
    struct ast_node *expr;
} ast_node_stmt_decl_t;

typedef struct ast_node_stmt_expr {
    ast_node_t hdr;

    /** ast_node_t of an expression type (or const/ident) */
    struct ast_node *expr;
} ast_node_stmt_expr_t;

typedef struct ast_node_stmt_if {
    ast_node_t hdr;

    ast_node_t *condition;
    /** vector of ast_node_t of statement types */
    vec_t *branch_true, *branch_false;
    struct scope *scope_true, *scope_false;
} ast_node_stmt_if_t;

typedef struct ast_node_stmt_ret {
    ast_node_t hdr;

    /** ast_node_t of an expression type (or const/ident) */
    struct ast_node *expr;
} ast_node_stmt_ret_t;

typedef struct ast_node_stmt_block {
    ast_node_t hdr;

    /** vector of ast_node_t of statement types */
    vec_t *stmts;

    struct scope *scope;
} ast_node_stmt_block_t;

/****** functions ******/

ast_node_ident_t *ast_node_ident_new(token_t *);
ast_node_const_t *ast_node_const_new(token_t *);
ast_node_expr_binary_t *ast_node_expr_binary_new(
    ast_node_t *, ast_node_t *, enum expr_binary_type);
ast_node_expr_unary_t *ast_node_expr_unary_new(
    ast_node_t *, enum expr_unary_type);

void ast_print(ast_node_t *);
void ast_free(ast_node_t *);

#endif /* PARSER_AST_H_ */
