#include <parser/code.h>
#include <stdlib.h>

static int code_generate_fn(vec_t *, ast_node_fn_defn_t *);

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

ir_instr_label_t *instr_new_label(size_t id) {
    ir_instr_label_t *instr = malloc(sizeof(ir_instr_label_t));
    instr->hdr.type = IR_LABEL;
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
    code->num_label = 0;
    code->instructions = vec_new_free(1, free);

    for(size_t i = 0; i < tu->functions->sz; ++i)
        if(code_generate_fn(code->instructions, vec_get(tu->functions, i)))
            return code_free(code), NULL;

    return code;
}

void code_free(ir_code_t *code) {
    vec_free(code->instructions);
    free(code);
}

static int code_generate_fn(vec_t *ins, ast_node_fn_defn_t *fn) {
    vec_push(ins, instr_new_func(IR_FUNC, fn->ref));
    vec_push(ins, instr_new_imm(IR_PUSH, 0));
    vec_push(ins, instr_new(IR_RET));
    vec_push(ins, instr_new(IR_LEAVE));
    return 0;
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

        case IR_CALL: case IR_FUNC: {
            ir_instr_func_t *func = (void *)in;
            printf(in->type == IR_CALL ? "CALL %.*s[%zu]\n"
                                       : "FUNC %.*s[%zu]\n",
                   (int)func->ref->name_sz, func->ref->name,
                   func->ref->num_args);
            break;
        }

        case IR_IF: {
            ir_instr_if_t *iif = (void *)in;
            printf("IF false:%zu end:%zu\n",
                   iif->false_label, iif->end_label);
            break;
        }

        case IR_LABEL: {
            ir_instr_label_t *label = (void *)in;
            printf("LABEL %zu\n", label->id);
            break;
        }

        default:
            printf("%s\n", instr_type_str(in->type));
            break;
        }
    }
}
