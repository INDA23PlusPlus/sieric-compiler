#include <parser/code.h>
#include <inttypes.h>
#include <stdlib.h>

static inline const char *data_str(ir_instr_data_t *);
static inline void asm_generate_binop(FILE *, const char *);
static inline void asm_generate_relop(FILE *, ir_code_t *, const char *,
                                      const char *);

static int asm_generate_instr(FILE *, ir_code_t *, ir_instr_t *);

static const char asm_pre[] = ""
"bits 64\n"
"extern printf\n"
"extern scanf\n"
"global main\n"
"section .text\n"
"print:\n"
"  push rbp\n"
"  mov rbp, rsp\n"
"  and rsp, ~0xf\n"
"  mov rdi, pri_int\n"
"  mov rsi, [rbp+0x10]\n"
"  xor al, al\n"
"  call printf\n"
"  xor eax, eax\n"
"  leave\n"
"  ret\n"
"input:\n"
"  push rbp\n"
"  mov rbp, rsp\n"
"  sub rsp, 0x8\n"
"  and rsp, ~0xf\n"
"  mov rdi, scn_int\n"
"  lea rsi, [rbp-0x8]\n"
"  call scanf\n"
"  mov rax, [rbp-0x8]\n"
"  leave\n"
"  ret\n"
;

static const char asm_post[] = ""
"section .rodata\n"
"pri_int: db \"%lld\",0xa,0\n"
"scn_int: db \"%lld\",0\n"
;

static inline const char *data_str(ir_instr_data_t *data) {
    static char buf[128];
    if(data->variable) snprintf(buf, sizeof buf, "qword [rbp%c%#zx]",
                       data->ref->bp_offset < 0 ? '-' : '+',
                       (size_t)llabs(data->ref->bp_offset));
    else snprintf(buf, sizeof buf, "%"PRId64, data->imm);
    return buf;
}

int asm_generate(FILE *f, ir_code_t *code) {
    int ret = 0;
    if(fwrite(asm_pre, 1, sizeof asm_pre - 1, f) != sizeof asm_pre - 1) {
        perror("fwrite");
        ret = 1;
        goto ret;
    }

    for(size_t i = 0; i < code->instructions->sz; ++i)
        if((ret = asm_generate_instr(f, code, vec_get(code->instructions, i))))
            goto ret;

    if(fwrite(asm_post, 1, sizeof asm_post - 1, f) != sizeof asm_post - 1) {
        perror("fwrite");
        ret = 1;
        goto ret;
    }
ret:
    return ret;
}

static inline void asm_generate_binop(FILE *f, const char *instr) {
    fprintf(f, "  pop rbx\n  pop rax\n  %s\n", instr);
}

static inline void asm_generate_relop(FILE *f, ir_code_t *code,
                                      const char *condition, const char *jump) {
    fprintf(f, "  pop rbx\n"
               "  pop rax\n"
               "%1$s"
               "  mov rdx, 1\n"
               "  %2$s .%3$zu\n"
               "  mov rdx, 0\n"
               ".%3$zu:\n"
               "  mov rax, rdx\n",
            condition, jump, code->num_label++);
}

static int asm_generate_instr(FILE *f, ir_code_t *code, ir_instr_t *in_) {
    int ret = 0;
    union {
        ir_instr_t *i;
        ir_instr_data_t *data;
        ir_instr_func_t *func;
        ir_instr_if_t *iif;
        ir_instr_label_t *label;
    } in = { .i = in_ };

    switch(in.i->type) {
    case IR_NOP: break;

    case IR_PUSH:
        fprintf(f, "  push %s\n", data_str(in.data));
        break;

    case IR_POP:
        fprintf(f, "  pop %s\n", data_str(in.data));
        break;

    case IR_ASSIGN:
        fprintf(f, "  pop rax\n  mov %s, rax\n", data_str(in.data));
        break;

    case IR_SAVE:
        fprintf(f, "  push rax\n");
        break;

    case IR_SCOPEBEGIN:
        fprintf(f, "  sub rsp, %zd\n", 8*in.data->imm);
        break;

    case IR_SCOPEEND:
        fprintf(f, "  add rsp, %zd\n", 8*in.data->imm);
        break;

    case IR_ADD:
        asm_generate_binop(f, "add rax, rbx");
        break;

    case IR_SUB:
        asm_generate_binop(f, "sub rax, rbx");
        break;

    case IR_MUL:
        asm_generate_binop(f, "mul rbx");
        break;

    case IR_DIV:
        asm_generate_binop(f, "div rbx");
        break;

    case IR_MOD:
        asm_generate_binop(f, "div rbx");
        fprintf(f, "  mov rax, rdx\n");
        break;

    case IR_BITOR:
        asm_generate_binop(f, "or rax, rbx");
        break;

    case IR_BITAND:
        asm_generate_binop(f, "and rax, rbx");
        break;

    case IR_BITXOR:
        asm_generate_binop(f, "xor rax, rbx");
        break;

    case IR_BITNOT:
        fprintf(f, "  pop rax\n  not rax\n");
        break;

    case IR_LNOT:
        fprintf(f, "  pop rax\n"
                   "  mov rbx, 1\n"
                   "  test rax, rax\n"
                   "  jz .%1$zu\n"
                   "  mov rbx, 0\n"
                   ".%1$zu:\n"
                   "  mov rax, rbx\n", code->num_label++);
        break;

    case IR_CALL:
        fprintf(f, "  call %.*s\n  add rsp, %#zx\n",
                (int)in.func->ref->name_sz, in.func->ref->name,
                8*in.func->ref->num_args);
        break;

    case IR_FUNC:
        fprintf(f, "%.*s:\n  push rbp\n  mov rbp, rsp\n",
                (int)in.func->ref->name_sz, in.func->ref->name);
        break;

    case IR_LEAVE:
        /* implicit return 0 */
        fprintf(f, "  xor eax, eax\n.ret:\n  leave\n  ret\n");
        break;

    case IR_LOR:
        asm_generate_relop(f, code, "  or rax, rbx\n  test rax, rax\n", "jnz");
        break;

    case IR_LAND:
        asm_generate_relop(f, code, "  mul rbx\n"
                                    "  or rax, rdx\n"
                                    "  test rax, rax\n", "jnz");
        break;

    case IR_LT:
        asm_generate_relop(f, code, "  cmp rax, rbx\n", "jl");
        break;

    case IR_GT:
        asm_generate_relop(f, code, "  cmp rax, rbx\n", "jg");
        break;

    case IR_LEQ:
        asm_generate_relop(f, code, "  cmp rax, rbx\n", "jle");
        break;

    case IR_GEQ:
        asm_generate_relop(f, code, "  cmp rax, rbx\n", "jge");
        break;

    case IR_EQ:
        asm_generate_relop(f, code, "  cmp rax, rbx\n", "je");
        break;

    case IR_NEQ:
        asm_generate_relop(f, code, "  cmp rax, rbx\n", "jne");
        break;

    case IR_IF:
        fprintf(f, "  pop rax\n  test rax, rax\n  jz .%zu\n",
                in.iif->false_label);
        break;

    case IR_LABEL:
        fprintf(f, ".%zu:\n", in.label->id);
        break;

    case IR_RET:
        fprintf(f, "  pop rax\n  jmp .ret\n");
        break;

    case IR_JMP:
        fprintf(f, "  jmp .%zu\n", in.label->id);
        break;

    default:
        fprintf(stderr, "[Error] Invalid instruction in IR code\n");
        ret = 1;
        goto ret;
    }

ret:
    return ret;
}
