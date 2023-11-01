#include <parser/code.h>
#include <stdlib.h>

static inline ir_instr_data_t *instr_new_var_find(enum ir_instr_type, scope_t *,
                                                  ast_node_ident_t *);
static inline ir_instr_func_t *instr_new_func_find(enum ir_instr_type,
                                                   ir_code_t *,
                                                   ast_node_ident_t *);

static int code_generate_fn(ir_code_t *, ast_node_fn_defn_t *);
static int code_generate_block(ir_code_t *, scope_t *, vec_t *);
static int code_generate_stmt(ir_code_t *, scope_t *, ast_node_t *);
static int code_generate_expr(ir_code_t *, scope_t *, ast_node_t *, int);

static const struct instrtypestr {
#define T(t, v) char str##t[sizeof(#t)];
#include <parser/__instr.h>
#undef T
} instrtypestr = {
#define T(t, v) #t,
#include <parser/__instr.h>
#undef T
};

static const unsigned short instrtypeidx[] = {
#define T(t, v) [(v)] = offsetof(struct instrtypestr, str##t),
#include <parser/__instr.h>
#undef T
};

ir_instr_t *instr_new(enum ir_instr_type type) {
    ir_instr_t *instr = malloc(sizeof(ir_instr_t));
    instr->type = type;
    return instr;
}

ir_instr_label_t *instr_new_label(enum ir_instr_type type, size_t id) {
    ir_instr_label_t *instr = malloc(sizeof(ir_instr_label_t));
    instr->hdr.type = type;
    instr->id = id;
    return instr;
}

ir_instr_if_t *instr_new_if(ir_code_t *code) {
    ir_instr_if_t *instr = malloc(sizeof(ir_instr_if_t));
    instr->hdr.type = IR_IF;
    instr->false_label = code->num_label++;
    instr->end_label = code->num_label++;
    return instr;
}

ir_instr_data_t *instr_new_var(enum ir_instr_type type, variable_ref_t *ref) {
    ir_instr_data_t *instr = malloc(sizeof(ir_instr_data_t));
    instr->hdr.type = type;
    instr->variable = 1;
    instr->ref = ref;
    return instr;
}

static inline ir_instr_data_t *instr_new_var_find(enum ir_instr_type type,
                                                  scope_t *scope,
                                                  ast_node_ident_t *ident) {
    return instr_new_var(type, variable_ref_find(scope, ident));
}

static inline ir_instr_func_t *instr_new_func_find(enum ir_instr_type type,
                                                   ir_code_t *code,
                                                   ast_node_ident_t *ident) {
    return instr_new_func(type, function_ref_find(code->ctx, ident));
}

ir_instr_data_t *instr_new_imm(enum ir_instr_type type, int64_t imm) {
    ir_instr_data_t *instr = malloc(sizeof(ir_instr_data_t));
    instr->hdr.type = type;
    instr->variable = 0;
    instr->imm = imm;
    return instr;
}

ir_instr_func_t *instr_new_func(enum ir_instr_type type, function_ref_t *ref) {
    ir_instr_func_t *instr = malloc(sizeof(ir_instr_func_t));
    instr->hdr.type = type;
    instr->ref = ref;
    return instr;
}

const char *instr_type_str(enum ir_instr_type type) {
    if((size_t)type >= sizeof instrtypeidx / sizeof *instrtypeidx) type = 0;
    return (char *)&instrtypestr + instrtypeidx[type];
}

ir_code_t *code_new(ast_node_tu_t *tu) {
    ir_code_t *code = malloc(sizeof(ir_code_t));
    code->ctx = tu->ctx;
    code->num_label = 0;
    code->instructions = vec_new_free(1, free);

    for(size_t i = 0; i < tu->functions->sz; ++i)
        if(code_generate_fn(code, vec_get(tu->functions, i)))
            return code_free(code), NULL;

    return code;
}

void code_free(ir_code_t *code) {
    vec_free(code->instructions);
    free(code);
}

static int code_generate_fn(ir_code_t *code, ast_node_fn_defn_t *fn) {
    int ret = 0;
    vec_t *ins = code->instructions;
    vec_push(ins, instr_new_func(IR_FUNC, fn->ref));
    if((ret = code_generate_block(code, fn->scope, fn->body)))
        goto ret;
    vec_push(ins, instr_new(IR_LEAVE));
ret:
    return ret;
}

static int code_generate_block(ir_code_t *code, scope_t *scope, vec_t *body) {
    int ret = 0;
    for(size_t i = 0; i < body->sz; ++i)
        if((ret = code_generate_stmt(code, scope, vec_get(body, i))))
            break;
    return ret;
}

static int code_generate_stmt(ir_code_t *code, scope_t *scope, ast_node_t *root) {
    int ret = 0;
    vec_t *ins = code->instructions;

    switch(root->type) {
    case AST_STMT_DECL: {
        ast_node_stmt_decl_t *stmt = (void *)root;
        if((ret = code_generate_expr(code, scope, stmt->expr, 1)))
            goto ret;
        vec_push(ins, instr_new_var_find(IR_ASSIGN, scope, stmt->ident));
        break;
    }

    case AST_STMT_EXPR: {
        ast_node_stmt_expr_t *stmt = (void *)root;
        if((ret = code_generate_expr(code, scope, stmt->expr, 0)))
            goto ret;
        break;
    }

    case AST_STMT_IF: {
        ast_node_stmt_if_t *stmt = (void *)root;
        if((ret = code_generate_expr(code, scope, stmt->condition, 1)))
            goto ret;
        ir_instr_if_t *iif = instr_new_if(code);
        vec_push(ins, iif);
        if((ret = code_generate_block(code, stmt->scope_true,
                                      stmt->branch_true)))
            goto ret;
        vec_push(ins, instr_new_label(IR_JMP, iif->end_label));
        vec_push(ins, instr_new_label(IR_LABEL, iif->false_label));
        if((ret = code_generate_block(code, stmt->scope_false,
                                      stmt->branch_false)))
            goto ret;
        vec_push(ins, instr_new_label(IR_LABEL, iif->end_label));
        break;
    }

    case AST_STMT_RET: {
        ast_node_stmt_ret_t *stmt = (void *)root;
        if((ret = code_generate_expr(code, scope, stmt->expr, 1)))
            goto ret;
        vec_push(ins, instr_new(IR_RET));
        break;
    }

    case AST_STMT_BLOCK: {
        ast_node_stmt_block_t *stmt = (void *)root;
        if((ret = code_generate_block(code, stmt->scope, stmt->stmts)))
            goto ret;
        break;
    }

    default:
        fprintf(stderr, "[Error] Non-statement node type '%d' in a statement"
                "list!\n", root->type);
        ret = 1;
        goto ret;
    }

ret:
    return ret;
}

static int code_generate_expr(ir_code_t *code, scope_t *scope, ast_node_t *root,
                              int save) {
    int ret = 0;
    vec_t *ins = code->instructions;

    switch(root->type) {
    case AST_EXPR_BINARY: {
        ast_node_expr_binary_t *expr = (void *)root;

        static const enum ir_instr_type binops[] = {
        [EXPR_LOR] = IR_LOR,
        [EXPR_LAND] = IR_LAND,
        [EXPR_BITOR] = IR_BITOR,
        [EXPR_BITXOR] = IR_BITXOR,
        [EXPR_BITAND] = IR_BITAND,
        [EXPR_EQ] = IR_EQ,
        [EXPR_NEQ] = IR_NEQ,
        [EXPR_LT] = IR_LT,
        [EXPR_GT] = IR_GT,
        [EXPR_LEQ] = IR_LEQ,
        [EXPR_GEQ] = IR_GEQ,
        [EXPR_ADD] = IR_ADD,
        [EXPR_SUB] = IR_SUB,
        [EXPR_MULT] = IR_MUL,
        [EXPR_DIV] = IR_DIV,
        [EXPR_MOD] = IR_MOD,
        };

        if((ret = code_generate_expr(code, scope, expr->left, 1)))
            goto ret;
        if((ret = code_generate_expr(code, scope, expr->right, 1)))
            goto ret;
        vec_push(ins, instr_new(binops[expr->type]));
        break;
    }

    case AST_EXPR_UNARY: {
        ast_node_expr_unary_t *expr = (void *)root;

        static const enum ir_instr_type unops[] = {
        [EXPR_LNOT] = IR_LNOT,
        [EXPR_BITNOT] = IR_BITNOT,
        };

        if((ret = code_generate_expr(code, scope, expr->op, 1)))
            goto ret;
        vec_push(ins, instr_new(unops[expr->type]));
        break;
    }

    case AST_EXPR_CALL: {
        ast_node_expr_call_t *expr = (void *)root;
        for(size_t i = 0; i < expr->args->sz; ++i)
            if((ret = code_generate_expr(code, scope,
                                         vec_get(expr->args, i), 1)))
                goto ret;
        vec_push(ins, instr_new_func_find(IR_CALL, code, expr->ident));
        break;
    }

    case AST_CONST: {
        ast_node_const_t *c = (void *)root;
        if(save) vec_push(ins, instr_new_imm(IR_PUSH, c->value));
        goto ret;
    }

    case AST_IDENT: {
        ast_node_ident_t *ident = (void *)root;
        if(save) vec_push(ins, instr_new_var_find(IR_PUSH, scope, ident));
        goto ret;
    }

    default:
        fprintf(stderr, "[Error] Invalid node type '%d' found in an"
                "expression!\n", root->type);
        ret = 1;
        goto ret;
    }

    if(save) vec_push(ins, instr_new(IR_SAVE));
ret:
    return ret;
}

void code_dump(ir_code_t *code) {
    for(size_t i = 0; i < code->instructions->sz; ++i) {
        ir_instr_t *in = vec_get(code->instructions, i);
        switch(in->type) {
        case IR_PUSH:
            printf("PUSH ");
            if(0) {
        case IR_POP:
            printf("POP ");
            } if(0) {
        case IR_ASSIGN:
            printf("ASSIGN ");
            }
            {
                ir_instr_data_t *data = (void *)in;
                if(data->variable)
                    printf("%.*s [rbp%c%#zx]\n",
                           (int)data->ref->name_sz, data->ref->name,
                           data->ref->bp_offset < 0 ? '-' : '+',
                           (size_t)llabs(data->ref->bp_offset));
                else
                    printf("%zd\n", data->imm);
            }
            break;

        case IR_CALL:
            printf("CALL ");
            if(0) {
        case IR_FUNC:
            printf("FUNC ");
            }
            {
                ir_instr_func_t *func = (void *)in;
                printf("%.*s[%zu]\n", (int)func->ref->name_sz, func->ref->name,
                    func->ref->num_args);
                break;
            }

        case IR_IF: {
            ir_instr_if_t *iif = (void *)in;
            printf("IF false:%zu end:%zu\n",
                   iif->false_label, iif->end_label);
            break;
        }

        case IR_LABEL:
            printf("LABEL ");
            if(0) {
        case IR_JMP:
            printf("JMP ");
            }
            {
                ir_instr_label_t *label = (void *)in;
                printf("%zu\n", label->id);
                break;
            }

        default:
            printf("%s\n", instr_type_str(in->type));
            break;
        }
    }
}
