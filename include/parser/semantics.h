#ifndef PARSER_SEMANTICS_H_
#define PARSER_SEMANTICS_H_

#include <parser/ast.h>
#include <utils/vector.h>

typedef struct function_ref {
    size_t name_sz;
    char *name;
    size_t num_args;
} function_ref_t;

typedef struct variable_ref {
    size_t name_sz;
    char *name;
    ssize_t bp_offset;
} variable_ref_t;

typedef struct semantics_ctx {
    /* struct scope *global; */

    /** vector of function_ref_t */
    vec_t *functions;

    int error;
} semantics_ctx_t;

typedef struct scope {
    struct semantics_ctx *ctx;
    struct scope *parent;
    vec_t *children;

    /** vector of variable_ref_t */
    vec_t *variables;
    size_t variable_count;
} scope_t;

function_ref_t *function_ref_new(char *, size_t, size_t);
function_ref_t *function_ref_new_node(ast_node_fn_defn_t *);
void function_ref_free(function_ref_t *);
function_ref_t *function_ref_find(semantics_ctx_t *, ast_node_ident_t *);

variable_ref_t *variable_ref_new(char *, size_t, ssize_t);
variable_ref_t *variable_ref_new_scope(scope_t *, ast_node_ident_t *);
variable_ref_t *variable_ref_new_arg(ast_node_ident_t *, size_t, size_t);
void variable_ref_free(variable_ref_t *);
variable_ref_t *variable_ref_find(scope_t *, ast_node_ident_t *);

scope_t *scope_new(semantics_ctx_t *, scope_t *);
void scope_free(scope_t *);

semantics_ctx_t *semantics_new(void);
void semantics_free(semantics_ctx_t *);
int semantics_analyze(semantics_ctx_t *, ast_node_tu_t *);

void semantics_dump_tables(ast_node_tu_t *);

#endif /* PARSER_SEMANTICS_H_ */
