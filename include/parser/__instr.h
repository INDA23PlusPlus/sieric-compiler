T(NOP,            0x00)

/* data control */
/** ir_instr_data_t */
T(PUSH,           0x01)
/** ir_instr_data_t (only variable) */
T(POP,            0x02)
/** ir_instr_data_t (only variable) */
T(ASSIGN,         0x03)

/* binary op */
T(ADD,            0x10)
T(SUB,            0x11)
T(MUL,            0x12)
T(DIV,            0x13)
T(MOD,            0x14)
T(BITOR,          0x15)
T(BITAND,         0x16)
T(BITXOR,         0x17)

/* unary op */
T(BITNOT,         0x18)
T(LNOT,           0x19)

/* functions */
/** ir_instr_call_t */
T(CALL,           0x30)
/** ir_instr_call_t */
T(FUNC,           0x31)
T(LEAVE,          0x32)

/* relational ops */
T(LOR,            0x40)
T(LAND,           0x41)
T(LT,             0x42)
T(GT,             0x43)
T(LEQ,            0x44)
T(GEQ,            0x45)
T(EQ,             0x46)
T(NEQ,            0x47)

/* control flow */
/** ir_instr_if_t */
T(IF,             0x60)
/** ir_instr_label_t */
T(LABEL,          0x61)
T(RET,            0x62)
