#include <parser/code.h>
#include <inttypes.h>
#include <stdlib.h>

static inline const char *data_str(ir_instr_data_t *);

static int asm_generate_instr(FILE *, ir_instr_t *);

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
        if((ret = asm_generate_instr(f, vec_get(code->instructions, i))))
            goto ret;

    if(fwrite(asm_post, 1, sizeof asm_post - 1, f) != sizeof asm_post - 1) {
        perror("fwrite");
        ret = 1;
        goto ret;
    }
ret:
    return ret;
}

#define BINOP_PRE "  pop rbx\n  pop rax\n"
#define UNOP_PRE "  pop rax\n"
#define CMP_PRE "  pop rbx\n  pop rax\n  cmp rax, rbx\n"
#define SET_POST "  movzx rax, al\n"

static int asm_generate_instr(FILE *f, ir_instr_t *in_) {
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
        fprintf(f, BINOP_PRE "  add rax, rbx\n");
        break;

    case IR_SUB:
        fprintf(f, BINOP_PRE "  sub rax, rbx\n");
        break;

    case IR_MUL:
        fprintf(f, BINOP_PRE "  mul rbx\n");
        break;

    case IR_DIV:
        fprintf(f, BINOP_PRE "  div rbx\n");
        break;

    case IR_MOD:
        fprintf(f, BINOP_PRE "  div rbx\n  mov rax, rdx");
        break;

    case IR_BITOR:
        fprintf(f, BINOP_PRE "  or rax, rbx\n");
        break;

    case IR_BITAND:
        fprintf(f, BINOP_PRE "  and rax, rbx\n");
        break;

    case IR_BITXOR:
        fprintf(f, BINOP_PRE "  xor rax, rbx\n");
        break;

    case IR_BITNOT:
        fprintf(f, UNOP_PRE "  not rax\n");
        break;

    case IR_LNOT:
        fprintf(f, UNOP_PRE "  cmp rax, 0\n  setne al\n" SET_POST);
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
        fprintf(f, BINOP_PRE "  or rax, rbx\n"
                             "  text rax, rax\n"
                             "  setnz al\n" SET_POST);
        break;

    case IR_LAND:
        fprintf(f, BINOP_PRE "  mul rbx\n"
                             "  or rax, rdx\n"
                             "  test rax, rax\n"
                             "  setnz al\n" SET_POST);
        break;

    case IR_LT:
        fprintf(f, CMP_PRE "  setl al\n" SET_POST);
        break;

    case IR_GT:
        fprintf(f, CMP_PRE "  setg al\n" SET_POST);
        break;

    case IR_LEQ:
        fprintf(f, CMP_PRE "  setle al\n" SET_POST);
        break;

    case IR_GEQ:
        fprintf(f, CMP_PRE "  setge al\n" SET_POST);
        break;

    case IR_EQ:
        fprintf(f, CMP_PRE "  sete al\n" SET_POST);
        break;

    case IR_NEQ:
        fprintf(f, CMP_PRE "  setne al\n" SET_POST);
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
