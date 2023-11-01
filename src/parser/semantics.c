#include <parser/semantics.h>
#include <stdlib.h>
#include <string.h>

static void scope_dump(scope_t *, size_t);

static int semantics_analyze_fn(semantics_ctx_t *, ast_node_fn_defn_t *);
static int semantics_analyze_block(semantics_ctx_t *, scope_t *, vec_t *);
static int semantics_analyze_stmt(semantics_ctx_t *, scope_t *, ast_node_t *);
static int semantics_analyze_expr(semantics_ctx_t *, scope_t *, ast_node_t *);

function_ref_t *function_ref_new(char *name, size_t name_sz, size_t num_args) {
    function_ref_t *ref = malloc(sizeof(function_ref_t));
    ref->name = malloc(ref->name_sz = name_sz);
    memcpy(ref->name, name, name_sz);
    ref->num_args = num_args;
    return ref;
}

function_ref_t *function_ref_new_node(ast_node_fn_defn_t *fn) {
    return function_ref_new(fn->ident->name, fn->ident->name_sz,
                            fn->arguments->sz);
}

void function_ref_free(function_ref_t *ref) {
    free(ref->name);
    free(ref);
}

variable_ref_t *variable_ref_new(char *name, size_t name_sz,
                                 ssize_t bp_offset) {
    variable_ref_t *ref = malloc(sizeof(variable_ref_t));
    ref->name = malloc(ref->name_sz = name_sz);
    memcpy(ref->name, name, name_sz);
    ref->bp_offset = bp_offset;
    return ref;
}

variable_ref_t *variable_ref_new_scope(scope_t *scope,
                                       ast_node_ident_t *ident) {
    return vec_push(
        scope->variables,
        variable_ref_new(ident->name, ident->name_sz,
                         -8 * ++scope->variable_count)
    );
}

variable_ref_t *variable_ref_new_arg(ast_node_ident_t *ident,
                                     size_t argi, size_t argn) {
    return variable_ref_new(ident->name, ident->name_sz, 8*(1 + argn - argi));
}

void variable_ref_free(variable_ref_t *ref) {
    free(ref->name);
    free(ref);
}

scope_t *scope_new(semantics_ctx_t *ctx, scope_t *parent) {
    scope_t *scope = malloc(sizeof(scope_t));
    scope->ctx = ctx;
    scope->parent = parent;
    if(parent) {
        scope->variable_count = parent->variable_count;
        vec_push(parent->children, scope);
    } else scope->variable_count = 0;
    scope->children = vec_new_free(1, (vec_free_t)scope_free);
    scope->variables = vec_new_free(1, (vec_free_t)variable_ref_free);
    return scope;
}

void scope_free(scope_t *scope) {
    vec_free(scope->children);
    vec_free(scope->variables);
    if(scope->parent) {
        size_t idx = vec_index_of_eq(scope->parent->children, scope);
        vec_delete(scope->parent->children, idx);
    }
    free(scope);
}

static void scope_dump(scope_t *scope, size_t n) {
    char pad[2*n+1];
    memset(pad, ' ', 2*n);
    pad[2*n] = '\0';

    for(size_t i = 0; i < scope->variables->sz; ++i) {
        variable_ref_t *ref = vec_get(scope->variables, i);
        size_t abs_off = llabs(ref->bp_offset);
        char sign = ref->bp_offset < 0 ? '-' : '+';
        printf("%s%.*s at [rbp%c%#zx]\n",
               pad, (int)ref->name_sz, ref->name, sign, abs_off);
    }
    printf("%sSubscopes: {\n", pad);
    for(size_t i = 0; i < scope->children->sz; ++i) {
        scope_dump(vec_get(scope->children, i), n+1);
        printf("%s,\n", pad);
    }
    printf("%s}\n", pad);
}

semantics_ctx_t *semantics_new(void) {
    semantics_ctx_t *ctx = malloc(sizeof(semantics_ctx_t));
    /* ctx->global = scope_new(ctx, NULL); */
    ctx->functions = vec_new_free(3, (vec_free_t)function_ref_free);
    vec_push(ctx->functions, function_ref_new("print", 5, 1));
    vec_push(ctx->functions, function_ref_new("input", 5, 0));
    ctx->error = 0;
    return ctx;
}

void semantics_free(semantics_ctx_t *ctx) {
    vec_free(ctx->functions);
    free(ctx);
}

int semantics_analyze(semantics_ctx_t *ctx, ast_node_tu_t *tu) {
    int ret = 0;
    tu->ctx = ctx;

    /* pre-add all functions to the global scope so all functions can see
     * eachother */
    for(size_t i = 0; i < tu->functions->sz; ++i)
        vec_push(ctx->functions,
                 function_ref_new_node(vec_get(tu->functions, i)));

    for(size_t i = 0; i < tu->functions->sz; ++i)
        if((ret = semantics_analyze_fn(ctx, vec_get(tu->functions, i))))
            break;

    return ctx->error = ret;
}

static int semantics_analyze_fn(semantics_ctx_t *ctx, ast_node_fn_defn_t *fn) {
    scope_t *scope = scope_new(ctx, NULL);
    fn->scope = scope;
    /* add all function arguments to the variable list */
    for(size_t i = 0; i < fn->arguments->sz; ++i)
        vec_push(scope->variables,
                 variable_ref_new_arg(vec_get(fn->arguments, i),
                                      i, fn->arguments->sz)
        );

    return semantics_analyze_block(ctx, scope, fn->body);
}

static int semantics_analyze_block(semantics_ctx_t *ctx, scope_t *scope,
                                   vec_t *body) {
    int ret = 0;
    for(size_t i = 0; i < body->sz; ++i)
        if((ret = semantics_analyze_stmt(ctx, scope, vec_get(body, i))))
            break;
    return ret;
}

static int semantics_analyze_stmt(semantics_ctx_t *ctx, scope_t *scope,
                                  ast_node_t *root) {
    int ret = 0;
    switch(root->type) {
    case AST_STMT_DECL: {
        ast_node_stmt_decl_t *stmt = (void *)root;
        /* analyze expression before adding the new variable */
        if((ret = semantics_analyze_expr(ctx, scope, stmt->expr)))
            goto ret;
        variable_ref_new_scope(scope, stmt->ident);
        break;
    }

    case AST_STMT_EXPR: {
        ast_node_stmt_expr_t *stmt = (void *)root;
        if((ret = semantics_analyze_expr(ctx, scope, stmt->expr)))
            goto ret;
        break;
    }

    case AST_STMT_IF: {
        ast_node_stmt_if_t *stmt = (void *)root;
        if((ret = semantics_analyze_expr(ctx, scope, stmt->condition)))
            goto ret;
        stmt->scope_true = scope_new(ctx, scope);
        if((ret = semantics_analyze_block(ctx, stmt->scope_true,
                                          stmt->branch_true)))
            goto ret;

        if(stmt->branch_false->sz) {
            stmt->scope_false = scope_new(ctx, scope);
            if((ret = semantics_analyze_block(ctx, stmt->scope_false,
                                              stmt->branch_false)))
                goto ret;
        }

        break;
    }

    case AST_STMT_RET: {
        ast_node_stmt_ret_t *stmt = (void *)root;
        if((ret = semantics_analyze_expr(ctx, scope, stmt->expr)))
            goto ret;
        break;
    }

    case AST_STMT_BLOCK: {
        ast_node_stmt_block_t *stmt = (void *)root;
        stmt->scope = scope_new(ctx, scope);
        if((ret = semantics_analyze_block(ctx, stmt->scope, stmt->stmts)))
            goto ret;
        break;
    }

    default:
        fprintf(stderr, "[Error] Non-statement node in a statement list!\n");
        ret = ctx->error = 1;
        goto ret;
    }

ret:
    return ret;
}

static int semantics_analyze_expr(semantics_ctx_t *ctx, scope_t *scope,
                                  ast_node_t *root) {
    return 0;
}

void semantics_dump_tables(ast_node_tu_t *tu) {
    semantics_ctx_t *ctx = tu->ctx;

    puts("Functions:");
    for(size_t i = 0; i < ctx->functions->sz; ++i) {
        function_ref_t *ref = vec_get(ctx->functions, i);
        printf("%.*s[%zu]\n", (int)ref->name_sz, ref->name, ref->num_args);
    }

    puts("Scopes:");
    for(size_t i = 0; i < tu->functions->sz; ++i) {
        ast_node_fn_defn_t *fn = vec_get(tu->functions, i);
        printf("%.*s:\n", (int)fn->ident->name_sz, fn->ident->name);
        scope_dump(fn->scope, 1);
    }
}
