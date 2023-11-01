#ifndef PARSER_CODE_H_
#define PARSER_CODE_H_

#include <parser/ast.h>
#include <parser/semantics.h>

enum ir_instr_type {
#define T(t, v) IR_##t = v,
#include <parser/__instr.h>
#undef T
};

typedef struct ir_instr {
    enum ir_instr_type type;
} ir_instr_t;

typedef struct ir_instr_label {
    struct ir_instr hdr;
    size_t id;
} ir_instr_label_t;

typedef struct ir_instr_if {
    struct ir_instr hdr;
    size_t false_label, end_label;
} ir_instr_if_t;

typedef struct ir_instr_data {
    struct ir_instr hdr;
    int variable;
    union {
        variable_ref_t *ref;
        int64_t imm;
    };
} ir_instr_data_t;

typedef struct ir_instr_func {
    struct ir_instr hdr;
    function_ref_t *ref;
} ir_instr_func_t;

typedef struct ir_code {
    /** vector of ir_instr_t */
    vec_t *instructions;
    size_t num_label;
} ir_code_t;

ir_instr_t *instr_new(enum ir_instr_type);
ir_instr_label_t *instr_new_label(size_t);
ir_instr_if_t *instr_new_if(ir_code_t *);
ir_instr_data_t *instr_new_var(enum ir_instr_type, variable_ref_t *);
ir_instr_data_t *instr_new_imm(enum ir_instr_type, int64_t);
ir_instr_func_t *instr_new_func(enum ir_instr_type, function_ref_t *);

const char *instr_type_str(enum ir_instr_type);

ir_code_t *code_new(ast_node_tu_t *);
void code_free(ir_code_t *);
void code_dump(ir_code_t *);

#endif /* PARSER_CODE_H_ */
