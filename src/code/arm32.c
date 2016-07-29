/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "svalue.h"
#include "operators.h"
#include "bitvector.h"
#include "object.h"
#include "builtin_functions.h"
#include "bignum.h"

#define MACRO  ATTRIBUTE((unused)) static

/* ARM32 machine code backend
 *
 * naming conventions:
 *
 *  <op>_reg_reg(dst, a, b)
 *      execute a <op> b and store to dst
 *  <op>_reg_imm(dst, a, imm, rot)
 *      execute a <op> (ROT(imm, rot*2)) and store to dst
 *  arm32_<op>_reg_int(dst, a, v)
 *      execute a <op> v and store to dst
 *
 */

enum arm32_register {
    ARM_REG_R0,
    ARM_REG_ARG1 = 0,
    ARM_REG_RVAL = 0,
    ARM_REG_R1,
    ARM_REG_ARG2 = 1,
    ARM_REG_R2,
    ARM_REG_ARG3 = 2,
    ARM_REG_R3,
    ARM_REG_ARG4 = 3,

    /* everything below is calee saved */
    ARM_REG_R4,
    ARM_REG_R5,
    ARM_REG_R6,
    ARM_REG_R7,
    ARM_REG_PIKE_LOCALS = 7,

    ARM_REG_R8,
    ARM_REG_PIKE_IP = 8,
    ARM_REG_R9,
    ARM_REG_PIKE_SP = 9,
    ARM_REG_R10,
    ARM_REG_PIKE_FP = 10,
    ARM_REG_R11,
    ARM_REG_FP = 11,
    ARM_REG_R12,
    ARM_REG_IP = 12,
    ARM_REG_R13,
    ARM_REG_SP = 13,
    ARM_REG_R14,
    ARM_REG_LR = 14,
    ARM_REG_R15,
    ARM_REG_PC = 15,
    ARM_REG_NONE = -1
};

unsigned INT32 RBIT(enum arm32_register reg) {
    return 1<<reg;
}

enum arm32_condition {
    ARM_COND_EQ = 0,       /* equal */
    ARM_COND_Z  = 0,       /* zero */
    ARM_COND_NE = 1 << 28, /* not equal */
    ARM_COND_NZ = 1 << 28, /* not zero */
    ARM_COND_CS = 2 << 28,
    ARM_COND_CC = 3 << 28,
    ARM_COND_MI = 4 << 28,
    ARM_COND_PL = 5 << 28,
    ARM_COND_VS = 6 << 28, /* overflow */
    ARM_COND_VC = 7 << 28, /* no overflow */
    /* unsigned comparison */
    ARM_COND_HI = 8 << 28,
    ARM_COND_LS = 9 << 28,
    /* signed comparison */
    ARM_COND_GE = 10 << 28,
    ARM_COND_LT = 11 << 28,
    ARM_COND_GT = 12 << 28,
    ARM_COND_LE = 13 << 28,
    /* unconditional */
    ARM_COND_AL = 14 << 28
};

enum arm32_addr_mode {
    ARM_ADDR_REGISTER,
    ARM_ADDR_IMMEDIATE = 1<<25,
    ARM_ADDR_LSL_IMM = 0,
    ARM_ADDR_LSL_REG = 1 << 3,
    ARM_ADDR_LSR_IMM = 1 << 4,
    ARM_ADDR_LSR_REG = 3 << 4,
    ARM_ADDR_ASR_IMM = 1 << 5,
    ARM_ADDR_ASR_REG = 5 << 5
};

enum data_proc_instr {
    ARM_PROC_AND,
    ARM_PROC_XOR,
    ARM_PROC_SUB,
    ARM_PROC_RSB,
    ARM_PROC_ADD,
    ARM_PROC_ADC,
    ARM_PROC_SBC,
    ARM_PROC_RSC,
    ARM_PROC_TST,
    ARM_PROC_TEQ,
    ARM_PROC_CMP,
    ARM_PROC_CMN,
    ARM_PROC_OR,
    ARM_PROC_MOV,
    ARM_PROC_BIC,
    ARM_PROC_MVN
};

enum shift_instr {
    ARM_SHIFT_LSL,
    ARM_SHIFT_LSR,
    ARM_SHIFT_ASR,
    ARM_SHIFT_ROT
};

#define OPCODE_FUN ATTRIBUTE((unused,warn_unused_result)) static PIKE_OPCODE_T


MACRO void arm32_flush_dirty_regs(void);
MACRO void arm32_call(void *ptr);
MACRO enum arm32_register ra_alloc_any(void);
MACRO void ra_free(enum arm32_register reg);

MACRO void break_my_arm(void) {
}

#ifdef ARM32_LOW_DEBUG
static unsigned INT32 stats_m[F_MAX_INSTR - F_OFFSET];
static unsigned INT32 stats_b[F_MAX_INSTR - F_OFFSET];

#define OPCODE0(X,Y,F) case X: return #X;
#define OPCODE1(X,Y,F) case X: return #X;
#define OPCODE2(X,Y,F) case X: return #X;
#define OPCODE0_TAIL(X,Y,F) case X: return #X;
#define OPCODE1_TAIL(X,Y,F) case X: return #X;
#define OPCODE2_TAIL(X,Y,F) case X: return #X;
#define OPCODE0_JUMP(X,Y,F) case X: return #X;
#define OPCODE1_JUMP(X,Y,F) case X: return #X;
#define OPCODE2_JUMP(X,Y,F) case X: return #X;
#define OPCODE0_TAILJUMP(X,Y,F) case X: return #X;
#define OPCODE1_TAILJUMP(X,Y,F) case X: return #X;
#define OPCODE2_TAILJUMP(X,Y,F) case X: return #X;
#define OPCODE0_PTRJUMP(X,Y,F) case X: return #X;
#define OPCODE1_PTRJUMP(X,Y,F) case X: return #X;
#define OPCODE2_PTRJUMP(X,Y,F) case X: return #X;
#define OPCODE0_TAILPTRJUMP(X,Y,F) case X: return #X;
#define OPCODE1_TAILPTRJUMP(X,Y,F) case X: return #X;
#define OPCODE2_TAILPTRJUMP(X,Y,F) case X: return #X;
#define OPCODE0_RETURN(X,Y,F) case X: return #X;
#define OPCODE1_RETURN(X,Y,F) case X: return #X;
#define OPCODE2_RETURN(X,Y,F) case X: return #X;
#define OPCODE0_TAILRETURN(X,Y,F) case X: return #X;
#define OPCODE1_TAILRETURN(X,Y,F) case X: return #X;
#define OPCODE2_TAILRETURN(X,Y,F) case X: return #X;
#define OPCODE0_BRANCH(X,Y,F) case X: return #X;
#define OPCODE1_BRANCH(X,Y,F) case X: return #X;
#define OPCODE2_BRANCH(X,Y,F) case X: return #X;
#define OPCODE0_TAILBRANCH(X,Y,F) case X: return #X;
#define OPCODE1_TAILBRANCH(X,Y,F) case X: return #X;
#define OPCODE2_TAILBRANCH(X,Y,F) case X: return #X;
#define OPCODE0_ALIAS(X,Y,F,A) case X: return #X;
#define OPCODE1_ALIAS(X,Y,F,A) case X: return #X;
#define OPCODE2_ALIAS(X,Y,F,A) case X: return #X;

const char* arm_get_opcode_name(PIKE_OPCODE_T code) {
    switch (code+F_OFFSET) {
#include "interpret_protos.h"
    default:
        return "<unknown>";
    }
}

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE0_TAIL
#undef OPCODE1_TAIL
#undef OPCODE2_TAIL
#undef OPCODE0_PTRJUMP
#undef OPCODE1_PTRJUMP
#undef OPCODE2_PTRJUMP
#undef OPCODE0_TAILPTRJUMP
#undef OPCODE1_TAILPTRJUMP
#undef OPCODE2_TAILPTRJUMP
#undef OPCODE0_RETURN
#undef OPCODE1_RETURN
#undef OPCODE2_RETURN
#undef OPCODE0_TAILRETURN
#undef OPCODE1_TAILRETURN
#undef OPCODE2_TAILRETURN
#undef OPCODE0_BRANCH
#undef OPCODE1_BRANCH
#undef OPCODE2_BRANCH
#undef OPCODE0_TAILBRANCH
#undef OPCODE1_TAILBRANCH
#undef OPCODE2_TAILBRANCH
#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP
#undef OPCODE0_ALIAS
#undef OPCODE1_ALIAS
#undef OPCODE2_ALIAS

ATTRIBUTE((destructor))
MACRO void write_stats() {
    int i;
    FILE* file = fopen("/home/el/opcode.state", "a");

    for (i = 0; i < F_MAX_INSTR - F_OFFSET; i++) {
        if (!stats_m[i] && !stats_b[i]) continue;

        fprintf(file, "%s\t%u\t%u\n", arm_get_opcode_name(i), stats_b[i], stats_m[i] - stats_b[i]);
    }

    fclose(file);
}
#endif

MACRO void record_opcode(PIKE_OPCODE_T code, int bytecode) {
    code = code; /* prevent unused warning */
    bytecode = bytecode;
#ifdef ARM32_LOW_DEBUG
    if (bytecode) {
        stats_b[code - F_OFFSET] ++;
    } else {
        stats_m[code - F_OFFSET] ++;
    }
#endif
}

OPCODE_FUN set_cond(unsigned INT32 instr, enum arm32_condition cond) {
    return (instr&0x0fffffff)|cond;
}

enum arm32_multiple_mode {
#define MODE(P, U, W)   (((P)<<24) | ((U)<<23) | ((W)<<21))
    ARM_MULT_DB  = MODE(1, 0, 0),
    ARM_MULT_DA  = MODE(0, 0, 0),
    ARM_MULT_IB  = MODE(1, 1, 0),
    ARM_MULT_IA  = MODE(0, 1, 0),
    ARM_MULT_DBW = MODE(1, 0, 1),
    ARM_MULT_DAW = MODE(0, 0, 1),
    ARM_MULT_IBW = MODE(1, 1, 1),
    ARM_MULT_IAW = MODE(0, 1, 1),
    ARM_MULT_ALL = MODE(1, 1, 1)
#undef MODE
};

OPCODE_FUN gen_store_multiple(enum arm32_register addr, enum arm32_multiple_mode mode,
                              unsigned INT32 registers) {
    unsigned INT32 instr = ARM_COND_AL | (1<<27) | registers | mode;

    instr |= addr << 16;

    return instr;
}

MACRO void store_multiple(enum arm32_register addr, enum arm32_multiple_mode mode,
                          unsigned INT32 registers) {
    add_to_program(gen_store_multiple(addr, mode, registers));
}

OPCODE_FUN gen_load_multiple(enum arm32_register addr, enum arm32_multiple_mode mode,
                             unsigned INT32 registers) {
    unsigned INT32 r = gen_store_multiple(addr, mode, registers) | (1<<20);

    return r;
}

MACRO void load_multiple(enum arm32_register addr, enum arm32_multiple_mode mode,
                         unsigned INT32 registers) {
    add_to_program(gen_load_multiple(addr, mode, registers));
}

#ifdef __ARM_ARCH_6T2__
OPCODE_FUN gen_mov_wide(enum arm32_register reg, unsigned short imm) {
    return ARM_COND_AL | (3 << 24) | (imm & 0xfff) | (reg << 12) | ((imm & 0xf000) << 4);
}

MACRO void mov_wide(enum arm32_register reg, unsigned short imm) {
    add_to_program(gen_mov_wide(reg, imm));
}

OPCODE_FUN gen_mov_top(enum arm32_register reg, unsigned short imm) {
    return gen_mov_wide(reg, imm) | (1<<22);
}

MACRO void mov_top(enum arm32_register reg, unsigned short imm) {
    add_to_program(gen_mov_top(reg, imm));
}
#endif

OPCODE_FUN gen_nop() {
    return ARM_COND_AL |(1<<25)|(1<<24)|(1<<21)|(1<<15)|(1<<14)|(1<<13)|(1<<12);
}

MACRO void nop() {
    add_to_program(gen_nop());
}

OPCODE_FUN gen_bx_reg(enum arm32_register to) {
    unsigned INT32 instr = ARM_COND_AL;
    int i;

    instr |= to;
    instr |= (1<<4);
    instr |= (1<<21)|(1<<24);

    for (i = 8; i < 20; i++) {
        instr |= (1<<i);
    }

    return instr;
}

MACRO void bx_reg(enum arm32_register to) {
    add_to_program(gen_bx_reg(to));
}

OPCODE_FUN gen_blx_reg(enum arm32_register to) {
    return gen_bx_reg(to) | (1<<5);
}

MACRO void blx_reg(enum arm32_register to) {
    add_to_program(gen_blx_reg(to));
}

OPCODE_FUN gen_b_imm(INT32 dist) {
    unsigned INT32 instr = ARM_COND_AL | (1<<27)|(1<<25);

    instr |= dist & 0xffffff;

    return instr;
}

MACRO void b_imm(INT32 dist, enum arm32_condition cond) {
    add_to_program(set_cond(gen_b_imm(dist), cond));
}

OPCODE_FUN gen_bl_imm(INT32 dist) {
    return gen_b_imm(dist) | (1<<24);
}

MACRO void bl_imm(INT32 dist) {
    add_to_program(gen_bl_imm(dist));
}

OPCODE_FUN gen_reg(enum data_proc_instr op, enum arm32_register dst, enum arm32_register src) {
    unsigned INT32 instr = ARM_ADDR_REGISTER | ARM_COND_AL;

    instr |= op << 21;
    instr |= dst << 12;
    instr |= src;

    return instr;
}

/* Data processing instruction with two register operands */
OPCODE_FUN gen_reg_reg(enum data_proc_instr op, enum arm32_register dst,
                       enum arm32_register a, enum arm32_register b) {

    return gen_reg(op, dst, b) | (a << 16);
}

/* Data processing instruction with immediate */
OPCODE_FUN gen_imm(enum data_proc_instr op, enum arm32_register dst,
                   unsigned char imm, unsigned char rot) {

    unsigned INT32 instr = ARM_ADDR_IMMEDIATE | ARM_COND_AL;

    assert(!(rot >> 4));

    instr |= op << 21;
    instr |= dst << 12;
    instr |= imm;
    instr |= rot << 8;

    return instr;
}

#define ROTR(v, n)      (((v) >> (n)) | ((v) << (32-(n))))

/* returns 1 if v can be represented as a rotated imm, with imm and rot set */
MACRO int arm32_make_imm(unsigned INT32 v, unsigned char *imm, unsigned char *rot) {
    unsigned INT32 b, n;

    if (v <= 0xff) {
        *imm = v;
        *rot = 0;
        return 1;
    }

    b = ctz32(v) / 2;
    n = v >> (b*2);
    if (n <= 0xff) {
        *imm = n;
        *rot = 16 - b;
        return 1;
    }

    return 0;
}

MACRO unsigned INT32 arm32_extract_imm(unsigned INT32 v, unsigned char *imm, unsigned char *rot) {
    unsigned INT32 b, n;

    if (v <= 0xff) {
        *imm = v;
        *rot = 0;
        return 0;
    }

    b = ctz32(v) / 2;
    v >>= (b*2);

    *imm = v;
    *rot = (*rot - b) % 16;

    return v & ~0xff;
}

MACRO void arm32_mov_int(enum arm32_register reg, unsigned INT32 v);

OPCODE_FUN gen_reg_imm(enum data_proc_instr op, enum arm32_register dst,
                       enum arm32_register reg, unsigned char imm, unsigned char rot) {
    unsigned INT32 instr = gen_imm(op, dst, imm, rot);

    instr |= reg << 16;

    return instr;
}

OPCODE_FUN gen_shift_reg_reg(enum shift_instr op, enum arm32_register dst,
                             enum arm32_register a, enum arm32_register b) {
    unsigned INT32 instr = ARM_COND_AL;
    instr |= dst << 12;
    instr |= 1<<24;
    instr |= 1<<23;
    instr |= 1<<21;
    instr |= 1<<4;
    instr |= op << 5;
    instr |= a << 8;
    instr |= b;

    return instr;
}

OPCODE_FUN gen_shift_reg_imm(enum shift_instr op, enum arm32_register dst,
                             enum arm32_register a, unsigned INT32 imm) {
    unsigned INT32 instr = gen_reg(ARM_PROC_MOV, dst, a);

    assert(!(imm >> 5));

    instr |= op << 5;
    instr |= imm << 7;

    return instr;
}

OPCODE_FUN gen_store_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    unsigned INT32 instr = ARM_COND_AL;

    instr |= dst << 12;
    instr |= base << 16;
    instr |= (1 << 26) | (1<< 24);

    if (offset < 0) {
        instr |= -offset;
        assert(!((-offset) >> 12));
    } else {
        instr |= (1<<23);
        instr |= offset;
        assert(!(offset >> 12));
    }

    return instr;
}

MACRO void store_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    add_to_program(gen_store_reg_imm(dst, base, offset));
}

OPCODE_FUN gen_load_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    return gen_store_reg_imm(dst, base, offset) | (1<<20);
}

MACRO void load_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    add_to_program(gen_load_reg_imm(dst, base, offset));
}

OPCODE_FUN gen_storeh_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    unsigned INT32 instr = ARM_COND_AL;

    instr |= dst << 12;
    instr |= base << 16;
    instr |= (1 << 22) | (1<< 24) | (1<<7) | (1<<5) | (1<<4);

    assert(offset < 0xff && offset > -0xff);

    if (offset < 0) {
        offset =- offset;
    } else {
        instr |= (1<<23);
    }

    instr |= offset & 0xf;
    instr |= (offset & 0xf0) << 4;

    return instr;
}

OPCODE_FUN gen_loadh_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    return gen_storeh_reg_imm(dst, base, offset) | (1<<20);
}

MACRO void storeh_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    add_to_program(gen_storeh_reg_imm(dst, base, offset));
}

MACRO void loadh_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    add_to_program(gen_loadh_reg_imm(dst, base, offset));
}


#define GEN_PROC_OP(name, NAME)                                                                          \
OPCODE_FUN gen_ ## name ## _reg_imm(enum arm32_register dst, enum arm32_register reg,                    \
                                    unsigned char imm, unsigned char rot) {                              \
    return gen_reg_imm(ARM_PROC_ ## NAME, dst, reg, imm, rot);                                           \
}                                                                                                        \
OPCODE_FUN gen_ ## name ## _reg_reg(enum arm32_register dst, enum arm32_register a,                      \
                                    enum arm32_register b) {                                             \
    return gen_reg_reg(ARM_PROC_ ## NAME, dst, a, b);                                                    \
}                                                                                                        \
OPCODE_FUN gen_ ## name ## s_reg_imm(enum arm32_register dst, enum arm32_register reg,                   \
                                     unsigned char imm, unsigned char rot) {                             \
    return gen_ ## name ## _reg_imm(dst, reg, imm, rot)|(1<<20);                                         \
}                                                                                                        \
OPCODE_FUN gen_ ## name ## s_reg_reg(enum arm32_register dst, enum arm32_register a,                     \
                                     enum arm32_register b) {                                            \
    return gen_ ## name ## _reg_reg(dst, a, b)|(1<<20);                                                  \
}                                                                                                        \
                                                                                                         \
MACRO void name ## _reg_imm(enum arm32_register dst, enum arm32_register reg, unsigned char imm,         \
                                      unsigned char rot) {                                               \
    add_to_program(gen_ ## name ## _reg_imm(dst, reg, imm, rot));                                        \
}                                                                                                        \
MACRO void name ## _reg_reg(enum arm32_register dst, enum arm32_register a, enum arm32_register b) {     \
    add_to_program(gen_ ## name ## _reg_reg(dst, a, b));                                                 \
}                                                                                                        \
MACRO void name ## s_reg_imm(enum arm32_register dst, enum arm32_register reg, unsigned char imm,        \
                                      unsigned char rot) {                                               \
    add_to_program(gen_ ## name ## s_reg_imm(dst, reg, imm, rot));                                       \
}                                                                                                        \
MACRO void name ## s_reg_reg(enum arm32_register dst, enum arm32_register a, enum arm32_register b) {    \
    add_to_program(gen_ ## name ## s_reg_reg(dst, a, b));                                                \
}                                                                                                        \
MACRO void arm32_ ## name ## _reg_int(enum arm32_register dst, enum arm32_register a, unsigned INT32 v) {\
    unsigned char imm, rot;                                                                              \
                                                                                                         \
    if (arm32_make_imm(v, &imm, &rot)) {                                                                 \
        name ## _reg_imm(dst, a, imm, rot);                                                              \
    } else {                                                                                             \
        enum arm32_register tmp = ra_alloc_any();                                                        \
        arm32_mov_int(tmp, v);                                                                           \
        name ## _reg_reg(dst, a, tmp);                                                                   \
        ra_free(tmp);                                                                                    \
    }                                                                                                    \
}                                                                                                        \
MACRO void arm32_ ## name ## s_reg_int(enum arm32_register dst, enum arm32_register a, unsigned INT32 v) {\
    unsigned char imm, rot;                                                                              \
                                                                                                         \
    if (arm32_make_imm(v, &imm, &rot)) {                                                                 \
        name ## s_reg_imm(dst, a, imm, rot);                                                             \
    } else {                                                                                             \
        enum arm32_register tmp = ra_alloc_any();                                                        \
        arm32_mov_int(tmp, v);                                                                           \
        name ## s_reg_reg(dst, a, tmp);                                                                  \
        ra_free(tmp);                                                                                    \
    }                                                                                                    \
}                                                                                                        \

#define GEN_SHIFT_OP(name, NAME)                                                                         \
OPCODE_FUN gen_ ## name ## _reg_imm(enum arm32_register dst, enum arm32_register reg,                    \
                                    unsigned INT32 imm) {                                                \
    return gen_shift_reg_imm(ARM_SHIFT_ ## NAME, dst, reg, imm);                                         \
}                                                                                                        \
OPCODE_FUN gen_ ## name ## _reg_reg(enum arm32_register dst, enum arm32_register a,                      \
                                    enum arm32_register b) {                                             \
    return gen_shift_reg_reg(ARM_SHIFT_ ## NAME, dst, a, b);                                             \
}                                                                                                        \
OPCODE_FUN gen_ ## name ## s_reg_imm(enum arm32_register dst, enum arm32_register reg,                   \
                                     unsigned INT32 imm) {                                               \
    return gen_ ## name ## _reg_imm(dst, reg, imm)|(1<<20);                                              \
}                                                                                                        \
OPCODE_FUN gen_ ## name ## s_reg_reg(enum arm32_register dst, enum arm32_register a,                     \
                                     enum arm32_register b) {                                            \
    return gen_ ## name ## _reg_reg(dst, a, b)|(1<<20);                                                  \
}                                                                                                        \
                                                                                                         \
MACRO void name ## _reg_imm(enum arm32_register dst, enum arm32_register reg, unsigned char imm) {       \
    add_to_program(gen_ ## name ## _reg_imm(dst, reg, imm));                                             \
}                                                                                                        \
MACRO void name ## _reg_reg(enum arm32_register dst, enum arm32_register a, enum arm32_register b) {     \
    add_to_program(gen_ ## name ## _reg_reg(dst, a, b));                                                 \
}                                                                                                        \
MACRO void name ## s_reg_imm(enum arm32_register dst, enum arm32_register reg, unsigned char imm) {      \
    add_to_program(gen_ ## name ## s_reg_imm(dst, reg, imm));                                            \
}                                                                                                        \
MACRO void name ## s_reg_reg(enum arm32_register dst, enum arm32_register a, enum arm32_register b) {    \
    add_to_program(gen_ ## name ## s_reg_reg(dst, a, b));                                                \
}                                                                                                        \
MACRO void arm32_ ## name ## _reg_int(enum arm32_register dst, enum arm32_register a, unsigned INT32 v) {\
    name ## _reg_imm(dst, a, v);                                                                         \
}                                                                                                        \
MACRO void arm32_ ## name ## s_reg_int(enum arm32_register dst, enum arm32_register a, unsigned INT32 v) {\
    name ## s_reg_imm(dst, a, v);                                                                        \
}

#define GEN_PROC_OP1(name, NAME)									\
OPCODE_FUN gen_ ## name ## _reg(enum arm32_register dst, enum arm32_register src) {			\
    return gen_reg(ARM_PROC_ ## NAME, dst, src);							\
}													\
MACRO void name ## _reg(enum arm32_register dst, enum arm32_register src) {				\
    add_to_program(gen_ ## name ## _reg(dst, src));							\
}													\
OPCODE_FUN gen_ ## name ## _imm(enum arm32_register dst, unsigned char imm, unsigned char rot) {	\
    return gen_imm(ARM_PROC_ ## NAME, dst, imm, rot);							\
}													\
MACRO void name ## _imm(enum arm32_register dst, unsigned char imm, unsigned char rot) {		\
    add_to_program(gen_ ## name ## _imm(dst, imm, rot));						\
}

#define GEN_PROC_CMP_OP(name, NAME)									\
OPCODE_FUN gen_ ## name ## _reg_reg(enum arm32_register a, enum arm32_register b) {		        \
    return (1<<20)|gen_reg_reg(ARM_PROC_ ## NAME, 0, a, b);						\
}													\
MACRO void name ## _reg_reg(enum arm32_register dst, enum arm32_register src) {				\
    add_to_program(gen_ ## name ## _reg_reg(dst, src));							\
}													\
OPCODE_FUN gen_ ## name ## _reg_imm(enum arm32_register a, unsigned char imm, unsigned char rot) {      \
    return (1<<20)|gen_reg_imm(ARM_PROC_ ## NAME, 0, a, imm, rot);					\
}													\
MACRO void name ## _reg_imm(enum arm32_register a, unsigned char imm, unsigned char rot) {	        \
    add_to_program(gen_ ## name ## _reg_imm(a, imm, rot));					        \
}

GEN_PROC_OP(sub, SUB)
GEN_PROC_OP(add, ADD)
GEN_PROC_OP(or, OR)
GEN_PROC_OP(and, AND)
GEN_PROC_OP(xor, XOR)

GEN_SHIFT_OP(lsl, LSL)

GEN_PROC_OP1(mov, MOV)
GEN_PROC_OP1(mvn, MVN)

GEN_PROC_CMP_OP(cmp, CMP)
GEN_PROC_CMP_OP(cmn, CMN)
GEN_PROC_CMP_OP(tst, TST)
GEN_PROC_CMP_OP(teq, TEQ)

MACRO void arm32_mov_int(enum arm32_register reg, unsigned INT32 v) {
    unsigned char imm, rot;

    if (arm32_make_imm(v, &imm, &rot)) {
        mov_imm(reg, imm, rot);
    } else if (arm32_make_imm(~v, &imm, &rot)) {
        mvn_imm(reg, imm, rot);
    } else {
#ifdef __ARM_ARCH_6T2__
        mov_wide(reg, v);
        if (v>>16) mov_top(reg, v>>16);
#else
        rot = 0;

        v = arm32_extract_imm(v, &imm, &rot);

        mov_imm(reg, imm, rot);

        while (v) {
            v = arm32_extract_imm(v, &imm, &rot);
            or_reg_imm(reg, reg, imm, rot);
        }
#endif
    }
}

MACRO void arm32_cmp_int(enum arm32_register reg, unsigned INT32 v) {
    unsigned char imm, rot;

    if (arm32_make_imm(v, &imm, &rot)) {
        cmp_reg_imm(reg, imm, rot);
    } else if (arm32_make_imm(-v, &imm, &rot)) {
        cmn_reg_imm(reg, imm, rot);
    } else {
        enum arm32_register tmp = ra_alloc_any();

        arm32_mov_int(tmp, v);
        cmp_reg_reg(reg, tmp);

        ra_free(tmp);
    }
}

/*
 * TODO: work with labels
 */
MACRO void arm32_rel_cond_jmp(enum arm32_register reg, enum arm32_register b, enum arm32_condition cond, int jmp) {
    cmp_reg_reg(reg, b);
    b_imm(jmp-2, cond);
}


/* This variant of arm32_mov_int will insert the right instructions into
 * a nop-slide so that the register 'dst' will contains 'v'.
 */
#ifdef __ARM_ARCH_6T2__
#define SIZEOF_ADD_SET_REG_AT   2
static void arm32_mov_int_at(unsigned INT32 offset, unsigned INT32 v,
                           enum arm32_register dst) {
    upd_pointer(offset++, gen_mov_wide(dst, v));
    if (v >> 16) upd_pointer(offset++, gen_mov_top(dst, v>>16));
}
#else
#define SIZEOF_ADD_SET_REG_AT   4
static void arm32_mov_int_at(unsigned INT32 offset, unsigned INT32 v,
                           enum arm32_register dst) {
    unsigned char imm, rot = 0;
    int n = 0;

    v = arm32_extract_imm(v, &imm, &rot);
    n++;

    upd_pointer(offset++, gen_mov_imm(dst, imm, rot));;

    while (v) {
        v = arm32_extract_imm(v, &imm, &rot);
        upd_pointer(offset++, gen_or_reg_imm(dst, dst, imm, rot));
        n++;
    }
}
#endif

/*
 * "High" level interface
 */

#define FLAG_SP_LOADED  1
#define FLAG_FP_LOADED  2
#define FLAG_LOCALS_LOADED 4

struct location_list_entry {
    unsigned INT32 location;
    struct location_list_entry *next;
};

struct compiler_state {
    /* currently unused and dirt registers */
    unsigned short free, dirt;
    /* address into which the initial stack push has to be
     * generated
     */
    unsigned INT32 push_addr;
    unsigned INT32 flags;
    struct location_list_entry *pop_list, *push_list;
} compiler_state;


static struct location_list_entry* add_to_list(struct location_list_entry *list, unsigned INT32 location) {
    struct location_list_entry *e = xalloc(sizeof(struct location_list_entry));

    e->location = location;
    e->next = list;

    return e;
}

static void add_to_pop_list(unsigned INT32 location) {
    compiler_state.pop_list = add_to_list(compiler_state.pop_list, location);
}

static void add_to_push_list(unsigned INT32 location) {
    compiler_state.push_list = add_to_list(compiler_state.push_list, location);
}

static void free_list(struct location_list_entry *list) {
    while (list) {
        struct location_list_entry * next = list->next;
        free(list);
        list = next;
    }
}

static void free_pop_list(void) {
    free_list(compiler_state.pop_list);
    compiler_state.pop_list = NULL;
}

static void free_push_list(void) {
    free_list(compiler_state.push_list);
    compiler_state.push_list = NULL;
}

struct label {
    struct location_list_entry *list;
    unsigned INT32 loc;
};

static void label_init(struct label *l) {
    l->list = NULL;
    l->loc = (unsigned INT32)-1;
}

MACRO INT32 label_dist(struct label *l) {
    if (l->loc != (unsigned INT32)-1)
        return l->loc - PIKE_PC - 2;
    l->list = add_to_list(l->list, PIKE_PC);
    return 0;
}

MACRO void label_generate(struct label *l) {
    unsigned INT32 loc = PIKE_PC;

    assert(l->loc == (unsigned INT32)-1);

    l->loc = loc;

    struct location_list_entry *e = l->list;

    while (e) {
        unsigned INT32 instr = read_pointer(e->location);
        if (instr & 0xffffff) {
            Pike_fatal("Setting label distance twice in %x\n", instr);
        }
        upd_pointer(e->location, instr|(loc - e->location - 2));
        e = e->next;
    }

    free_list(l->list);
}

void arm32_init_interpreter_state(void) {
    assert(sizeof(struct svalue) == 8);
    assert(OFFSETOF(pike_frame, num_locals) % 4 == 0);
    assert(OFFSETOF(pike_frame, num_locals) + 2 == OFFSETOF(pike_frame, num_args));

    instrs[F_CATCH - F_OFFSET].address = inter_return_opcode_F_CATCH;
}

MACRO void ra_init(void) {
    /* FIXME: this ought to happen in init_interpreter_state, only, but it is currently overwritten
     * later */
    instrs[F_CATCH - F_OFFSET].address = inter_return_opcode_F_CATCH;

    /* all register r0 through r10 are unused */
    compiler_state.free = RBIT(0)|RBIT(1)|RBIT(2)|RBIT(3)|RBIT(4)|RBIT(12);
    compiler_state.dirt = 0;
    compiler_state.push_addr = -1;
    compiler_state.flags = 0;
    compiler_state.pop_list = NULL;
    compiler_state.push_list = NULL;
}

MACRO enum arm32_register ra_alloc(enum arm32_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    if (!(rbit & compiler_state.free))
        Pike_fatal("Register %d is already in use.\n", reg);

    compiler_state.free ^= rbit;
    compiler_state.dirt |= rbit;

    return reg;
}

MACRO enum arm32_register ra_alloc_persistent(void) {
    unsigned short free = compiler_state.free;

    /* we dont want 0-3 or r12 */
    free &= ~(0xf | RBIT(12));

    if (!free)
        Pike_fatal("No register left: %x\n", compiler_state.free);

    return ra_alloc(ctz32(free));
}

MACRO enum arm32_register ra_alloc_any(void) {
    if (!compiler_state.free)
        Pike_fatal("No register left.\n");

    return ra_alloc(ctz32(compiler_state.free));
}

MACRO void ra_free(enum arm32_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    if (rbit & compiler_state.free)
        Pike_fatal("Register %d is not in use.\n", reg);

    compiler_state.free |= rbit;
}

MACRO int ra_is_dirty(enum arm32_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    return !!(rbit & compiler_state.dirt);
}

MACRO int ra_is_free(enum arm32_register reg) {
    unsigned INT32 rbit = RBIT(reg);

    return !!(rbit & compiler_state.free);
}

MACRO void add_push(void) {
    unsigned INT32 registers;

    add_to_push_list(PIKE_PC);
    add_to_program(0);
}

MACRO void add_pop(void) {
    add_to_pop_list(PIKE_PC);
    add_to_program(0);
}

/* corresponds to ENTRY_PROLOGUE_SIZE */
MACRO void arm32_prologue(void) {
    add_push();
    mov_reg(ARM_REG_PIKE_IP, ARM_REG_ARG1);
}

#define EPILOGUE_SIZE 1
MACRO void arm32_epilogue(void) {
    add_pop();
}

MACRO void arm32_call(void *ptr) {
    unsigned INT32 v = (char*)ptr - (char*)NULL;
    enum arm32_register tmp = ra_alloc_any();

    arm32_mov_int(tmp, v);
    blx_reg(tmp);
    ra_free(tmp);
}


void arm32_flush_instruction_cache(void *addr, size_t len) {
    __builtin___clear_cache(addr, (char*)addr+len);
}

MACRO void arm32_low_load_sp_reg(void) {
    INT32 offset = OFFSETOF(Pike_interpreter_struct, stack_pointer);

    load_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_IP, offset);
}

static void arm32_load_sp_reg(void) {
    if (!(compiler_state.flags & FLAG_SP_LOADED)) {

        compiler_state.flags |= FLAG_SP_LOADED;

        arm32_low_load_sp_reg();
    }
}

static void arm32_store_sp_reg(void) {
    INT32 offset = OFFSETOF(Pike_interpreter_struct, stack_pointer);
    assert(compiler_state.flags & FLAG_SP_LOADED);
    store_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_IP, offset);
}

static void arm32_load_fp_reg(void) {
    if (!(compiler_state.flags & FLAG_FP_LOADED)) {
        INT32 offset = OFFSETOF(Pike_interpreter_struct, frame_pointer);
        /* load Pike_interpreter_pointer->frame_pointer into ARM_REG_PIKE_FP */
        load_reg_imm(ARM_REG_PIKE_FP, ARM_REG_PIKE_IP, offset);
        compiler_state.flags |= FLAG_FP_LOADED;
        compiler_state.flags &= ~FLAG_LOCALS_LOADED;
    }
}

MACRO void arm32_load_locals_reg(void) {
    arm32_load_fp_reg();

    if (!(compiler_state.flags & FLAG_LOCALS_LOADED)) {
        INT32 offset = OFFSETOF(pike_frame, locals);

        load_reg_imm(ARM_REG_PIKE_LOCALS, ARM_REG_PIKE_FP, offset);

        compiler_state.flags |= FLAG_LOCALS_LOADED;
    }
}

MACRO void arm32_change_sp_reg(INT32 offset) {
    assert(compiler_state.flags & FLAG_SP_LOADED);
    offset *= sizeof(struct svalue);
    if (offset < 0) {
        offset = -offset;
        arm32_sub_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, offset);
    } else {
        arm32_add_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, offset);
    }
}

MACRO void arm32_change_sp(INT32 offset) {
    arm32_load_sp_reg();
    arm32_change_sp_reg(offset);
    arm32_store_sp_reg();
}

static void arm32_flush_dirty_regs(void) {
    arm32_store_sp_reg();
}

MACRO void arm32_call_efun(void (*fun)(int), int args) {
    arm32_mov_int(ra_alloc(ARM_REG_ARG1), args);
    arm32_call(fun);
    ra_free(ARM_REG_ARG1);
    if (args != 1 && compiler_state.flags & FLAG_SP_LOADED) {
        arm32_change_sp_reg(-(args-1));
    }
}

MACRO void arm32_assign_int_reg(enum arm32_register dst, enum arm32_register value, int subtype) {
    unsigned INT32 combined = TYPE_SUBTYPE(PIKE_T_INT, subtype);
    enum arm32_register tmp1 = ra_alloc_any(), tmp2 = ra_alloc_any();

    assert(tmp1 < tmp2);

    arm32_mov_int(tmp1, combined);
    mov_reg(tmp2, value);

    store_multiple(dst, ARM_MULT_IA, RBIT(tmp1)|RBIT(tmp2));

    ra_free(tmp1);
    ra_free(tmp2);
}

MACRO void arm32_push_int_reg(enum arm32_register value, int subtype) {
    unsigned INT32 combined = TYPE_SUBTYPE(PIKE_T_INT, subtype);
    enum arm32_register tmp1 = ra_alloc_any(), tmp2 = ra_alloc_any();

    assert(tmp1 < tmp2);

    arm32_load_sp_reg();

    arm32_mov_int(tmp1, combined);
    mov_reg(tmp2, value);

    store_multiple(ARM_REG_PIKE_SP, ARM_MULT_IAW, RBIT(tmp1)|RBIT(tmp2));

    ra_free(tmp1);
    ra_free(tmp2);

    arm32_store_sp_reg();
}

MACRO void arm32_push_int(unsigned INT32 value, int subtype) {
    unsigned INT32 combined = TYPE_SUBTYPE(PIKE_T_INT, subtype);
    enum arm32_register tmp1 = ra_alloc_any(), tmp2 = ra_alloc_any();

    assert(tmp1 < tmp2);

    arm32_load_sp_reg();

    arm32_mov_int(tmp1, combined);
    arm32_mov_int(tmp2, value);

    store_multiple(ARM_REG_PIKE_SP, ARM_MULT_IAW, RBIT(tmp1)|RBIT(tmp2));

    ra_free(tmp1);
    ra_free(tmp2);

    arm32_store_sp_reg();
}

MACRO void arm32_push_ptr_type(enum arm32_register treg, enum arm32_register vreg) {
    assert(treg < vreg);

    arm32_load_sp_reg();
    store_multiple(ARM_REG_PIKE_SP, ARM_MULT_IAW, RBIT(treg)|RBIT(vreg));
    arm32_store_sp_reg();

    /* add reference */
    load_reg_imm(treg, vreg, 0);
    arm32_add_reg_int(treg, treg, 1);
    store_reg_imm(treg, vreg, 0);
}

MACRO void arm32_move_svaluep_nofree(enum arm32_register dst, enum arm32_register from) {
    enum arm32_register treg = ra_alloc_any(),
                        vreg = ra_alloc_any();

    assert(treg < vreg);

    load_multiple(from, ARM_MULT_IA, RBIT(treg)|RBIT(vreg));
    store_multiple(dst, ARM_MULT_IA, RBIT(treg)|RBIT(vreg));

    ra_free(treg);
    ra_free(vreg);
}

MACRO void arm32_assign_svaluep_nofree(enum arm32_register dst, enum arm32_register from) {
    enum arm32_register treg = ra_alloc_any(),
                        vreg = ra_alloc_any();

    assert(treg < vreg);

    load_multiple(from, ARM_MULT_IA, RBIT(treg)|RBIT(vreg));
    store_multiple(dst, ARM_MULT_IA, RBIT(treg)|RBIT(vreg));

    arm32_ands_reg_int(treg, treg, TYPE_SUBTYPE(MIN_REF_TYPE, 0));

    add_to_program(set_cond(gen_load_reg_imm(treg, vreg, OFFSETOF(pike_string, refs)), ARM_COND_NZ));
    add_to_program(set_cond(gen_add_reg_imm(treg, treg, 1, 0), ARM_COND_NZ));
    add_to_program(set_cond(gen_store_reg_imm(treg, vreg, OFFSETOF(pike_string, refs)), ARM_COND_NZ));

    ra_free(treg);
    ra_free(vreg);
}

MACRO void arm32_push_svaluep_off(enum arm32_register src, INT32 offset) {
    enum arm32_register tmp1 = ra_alloc_any(),
                        tmp2 = ra_alloc_any();

    assert(tmp1 < tmp2);

    if (offset) {
        arm32_add_reg_int(tmp1, src, offset*sizeof(struct svalue));

        load_multiple(tmp1, ARM_MULT_IA, RBIT(tmp1)|RBIT(tmp2));
    } else {
        load_multiple(src, ARM_MULT_IA, RBIT(tmp1)|RBIT(tmp2));
    }

    arm32_load_sp_reg();

    store_multiple(ARM_REG_PIKE_SP, ARM_MULT_IAW, RBIT(tmp1)|RBIT(tmp2));

    arm32_store_sp_reg();

    arm32_ands_reg_int(tmp1, tmp1, TYPE_SUBTYPE(MIN_REF_TYPE, 0));

    add_to_program(set_cond(gen_load_reg_imm(tmp1, tmp2, OFFSETOF(pike_string, refs)), ARM_COND_NZ));
    add_to_program(set_cond(gen_add_reg_imm(tmp1, tmp1, 1, 0), ARM_COND_NZ));
    add_to_program(set_cond(gen_store_reg_imm(tmp1, tmp2, OFFSETOF(pike_string, refs)), ARM_COND_NZ));

    ra_free(tmp1);
    ra_free(tmp2);
}

/* the returned condition will be true if both types are type_subtype */
MACRO enum arm32_condition arm32_eq_types(enum arm32_register type1, enum arm32_register type2,
                                          unsigned INT32 type_subtype) {
    unsigned char imm, rot;
    int ok = arm32_make_imm(type_subtype, &imm, &rot);

    assert(ok);

    cmp_reg_reg(type1, type2);
    add_to_program(set_cond(gen_cmp_reg_imm(type1, imm, rot), ARM_COND_EQ));

    return ARM_COND_EQ;
}

/* the returned condition will be true if unless both types are type_subtype */
MACRO enum arm32_condition arm32_ne_types(enum arm32_register type1, enum arm32_register type2,
                                          unsigned INT32 type_subtype) {
    unsigned char imm, rot;
    int ok = arm32_make_imm(type_subtype, &imm, &rot);

    assert(ok);

    cmp_reg_reg(type1, type2);
    add_to_program(set_cond(gen_cmp_reg_imm(type1, imm, rot), ARM_COND_EQ));

    return ARM_COND_NE;
}

MACRO void arm32_jump_real_cmp(struct label *l, enum arm32_register type1, enum arm32_register type2) {
    unsigned INT32 mask = BIT_INT|BIT_STRING|BIT_PROGRAM|BIT_MAPPING|BIT_ARRAY|BIT_MULTISET|BIT_TYPE;
    enum arm32_register reg = ra_alloc_any(),
                        one = ra_alloc_any(),
                        mask_reg = ra_alloc_any();

    arm32_mov_int(reg, mask);
    arm32_mov_int(one, 1);

    lsl_reg_reg(mask_reg, one, type1);
    ands_reg_reg(mask_reg, reg, mask_reg);

    add_to_program(set_cond(gen_lsl_reg_reg(mask_reg, one, type2), ARM_COND_NZ));
    add_to_program(set_cond(gen_ands_reg_reg(mask_reg, mask, mask_reg), ARM_COND_NZ));

    b_imm(label_dist(l), ARM_COND_Z);

    ra_free(reg);
    ra_free(one);
    ra_free(mask_reg);
}

void arm32_flush_codegen_state(void) {
    compiler_state.flags = 0;
}

void arm32_start_function(int UNUSED(no_pc)) {
    ra_init();
}

/*
 * NOTE:
 * This function is here because of an initial misunderstanding. We believed that a function would
 * _always_ exit through one its own exits and that therefore it would be possible to track register
 * usage. This is unfortunately not the case, so we have to always push and pop the same register,
 * regardless of whether we are going to use them or not.
 */
void arm32_end_function(int UNUSED(no_pc)) {
    unsigned INT32 registers, instr;
    struct location_list_entry *e;

    /* NOTE: always need to push an even number of registers */
    registers = RBIT(ARM_REG_PIKE_SP)
                |RBIT(ARM_REG_PIKE_IP)
                |RBIT(ARM_REG_PIKE_FP)
                |RBIT(ARM_REG_PIKE_LOCALS)
                |RBIT(4);

    e = compiler_state.pop_list;

    instr = gen_load_multiple(ARM_REG_SP, ARM_MULT_IAW, registers|RBIT(ARM_REG_PC));
    while (e) {
        upd_pointer(e->location, instr);
        e = e->next;
    }

    e = compiler_state.push_list;

    instr = gen_store_multiple(ARM_REG_SP, ARM_MULT_DBW, registers|RBIT(ARM_REG_LR));

    while (e) {
        upd_pointer(e->location, instr);
        e = e->next;
    }

    free_pop_list();
    free_push_list();
}

/* Store $pc into Pike_fp->pc */
void arm32_update_pc(void) {
    unsigned INT32 v = PIKE_PC;
    INT32 offset;
    enum arm32_register tmp = ra_alloc_any();

    arm32_load_fp_reg();
    v = 4*(PIKE_PC - v + 2);
    arm32_sub_reg_int(tmp, ARM_REG_PC, v);
    store_reg_imm(tmp, ARM_REG_PIKE_FP, OFFSETOF(pike_frame, pc));
    ra_free(tmp);
}

void arm32_ins_entry(void) {
    arm32_prologue();
    arm32_flush_codegen_state();
}

static void arm32_ins_maybe_exit(void) {
    struct label noreturn;

    label_init(&noreturn);

    arm32_cmp_int(ARM_REG_RVAL, -1);
    b_imm(label_dist(&noreturn), ARM_COND_NE);
    arm32_epilogue();
    label_generate(&noreturn);
}

MACRO void arm32_maybe_update_pc() {
  {
    static int last_prog_id=-1;
    static size_t last_num_linenumbers=(size_t)~0;
    if(last_prog_id != Pike_compiler->new_program->id ||
       last_num_linenumbers != Pike_compiler->new_program->num_linenumbers)
    {
      unsigned INT32 tmp = PIKE_PC;
      last_prog_id=Pike_compiler->new_program->id;
      last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;

      UPDATE_PC();
    }
  }
}

static void low_ins_call(void *addr) {
  arm32_maybe_update_pc();
  arm32_call(addr);
}

MACRO void arm32_call_c_function(void * addr) {
    compiler_state.flags &= ~FLAG_SP_LOADED;
    compiler_state.flags &= ~FLAG_FP_LOADED;
    arm32_call(addr);
}

MACRO void arm32_call_c_opcode(unsigned int opcode) {
  void *addr = instrs[opcode-F_OFFSET].address;
  int flags = instrs[opcode-F_OFFSET].flags;

  record_opcode(opcode, 1);

  if (flags & I_UPDATE_SP) {
    compiler_state.flags &= ~FLAG_SP_LOADED;
  }
  if (flags & I_UPDATE_M_SP) {}
  if (flags & I_UPDATE_FP) {
    compiler_state.flags &= ~FLAG_FP_LOADED;
  }

  low_ins_call(addr);
}

MACRO void arm32_free_svalue_off(enum arm32_register src, int off, int guaranteed) {
    unsigned INT32 combined = TYPE_SUBTYPE(MIN_REF_TYPE, 0);
    unsigned char imm, rot;
    struct label end;
    enum arm32_register reg = ra_alloc(ARM_REG_ARG1);
    enum arm32_register tmp = ra_alloc_any();

    guaranteed = guaranteed;

    off *= sizeof(struct svalue);

    label_init(&end);

    load_reg_imm(reg, src, off);

    arm32_ands_reg_int(reg, reg, combined);

    b_imm(label_dist(&end), ARM_COND_Z);

    load_reg_imm(tmp, src, off+OFFSETOF(svalue, u));

    load_reg_imm(reg, tmp, OFFSETOF(pike_string, refs));
    subs_reg_imm(reg, reg, 1, 0);
    store_reg_imm(reg, tmp, OFFSETOF(pike_string, refs));

    b_imm(label_dist(&end), ARM_COND_NZ);

    if (off > 0) {
        arm32_add_reg_int(reg, src, off);
    } else if (off < 0 ) {
        arm32_sub_reg_int(reg, src, -off);
    } else {
        mov_reg(reg, src);
    }

    arm32_call(really_free_svalue);

    label_generate(&end);
    ra_free(reg);
    ra_free(tmp);
}

MACRO void arm32_ins_branch_check_threads_etc(int a) {
}

static void arm32_free_svalue(enum arm32_register reg, int guaranteed_ref) {
    arm32_free_svalue_off(reg, 0, guaranteed_ref);
}

static void arm32_mark(enum arm32_register base, int offset) {

  enum arm32_register tmp = ra_alloc_any();

  load_reg_imm(tmp, ARM_REG_PIKE_IP, OFFSETOF(Pike_interpreter_struct, mark_stack_pointer));

  if (offset) {
    enum arm32_register tmp2 = ra_alloc_any();

    offset *= sizeof(struct svalue);

    if (offset > 0) {
      arm32_add_reg_int(tmp2, base, offset);
    } else {
      arm32_sub_reg_int(tmp2, base, -offset);
    }
    store_reg_imm(tmp2, tmp, 0);

    ra_free(tmp2);
  } else {
    store_reg_imm(base, tmp, 0);
  }

  add_reg_imm(tmp, tmp, sizeof(void*), 0);
  store_reg_imm(tmp, ARM_REG_PIKE_IP, OFFSETOF(Pike_interpreter_struct, mark_stack_pointer));

  ra_free(tmp);
}

MACRO void arm32_pop_mark(enum arm32_register dst) {
  enum arm32_register tmp = ra_alloc_any();

  load_reg_imm(tmp, ARM_REG_PIKE_IP, OFFSETOF(Pike_interpreter_struct, mark_stack_pointer));

  arm32_sub_reg_int(tmp, tmp, sizeof(void*));

  load_reg_imm(dst, tmp, 0);

  store_reg_imm(tmp, ARM_REG_PIKE_IP, OFFSETOF(Pike_interpreter_struct, mark_stack_pointer));

  ra_free(tmp);
}

#ifdef ARM32_PIKE_DEBUG
MACRO void arm_green_on(void) {
    if (Pike_interpreter.trace_level > 2)
        fprintf(stderr, "\33[032m");
}

MACRO void arm_green_off(void) {
    if (Pike_interpreter.trace_level > 2)
        fprintf(stderr, "\33[0m");
}

MACRO void arm32_debug_instr_prologue_0(PIKE_INSTR_T instr) {
  arm32_call(arm_green_on);
  arm32_maybe_update_pc();
  arm32_mov_int(ra_alloc(ARM_REG_ARG1), instr-F_OFFSET);
  arm32_call(simple_debug_instr_prologue_0);
  arm32_call(arm_green_off);
  ra_free(ARM_REG_ARG1);
}

MACRO void arm32_debug_instr_prologue_1(PIKE_INSTR_T instr, INT32 arg1) {
  arm32_call(arm_green_on);
  arm32_maybe_update_pc();
  arm32_mov_int(ra_alloc(ARM_REG_ARG1), instr-F_OFFSET);
  arm32_mov_int(ra_alloc(ARM_REG_ARG2), arg1);
  arm32_call(simple_debug_instr_prologue_1);
  arm32_call(arm_green_off);
  ra_free(ARM_REG_ARG1);
  ra_free(ARM_REG_ARG2);
}

MACRO void arm32_debug_instr_prologue_2(PIKE_INSTR_T instr, INT32 arg1, INT32 arg2) {
  arm32_call(arm_green_on);
  arm32_maybe_update_pc();
  arm32_mov_int(ra_alloc(ARM_REG_ARG1), instr-F_OFFSET);
  arm32_mov_int(ra_alloc(ARM_REG_ARG2), arg1);
  arm32_mov_int(ra_alloc(ARM_REG_ARG3), arg2);
  arm32_call(simple_debug_instr_prologue_2);
  arm32_call(arm_green_off);
  ra_free(ARM_REG_ARG1);
  ra_free(ARM_REG_ARG2);
  ra_free(ARM_REG_ARG3);
}
#else
#define arm32_debug_instr_prologue_1(a, b)  do {} while(0)
#define arm32_debug_instr_prologue_2(a, b, c)  do {} while(0)
#define arm32_debug_instr_prologue_0(a) do { } while(0)
#endif

static void low_ins_f_byte(unsigned int opcode)
{
  int flags;
  INT32 rel_addr = rel_addr;

  assert(opcode-F_OFFSET<=255);

  flags = instrs[opcode-F_OFFSET].flags;

  switch (opcode) {
  case F_UNDEFINED:
      arm32_debug_instr_prologue_0(opcode);
      arm32_push_int(0, NUMBER_UNDEFINED);
      return;
  case F_CONST_1:
      arm32_debug_instr_prologue_0(opcode);
      arm32_push_int(-1, NUMBER_NUMBER);
      return;
  case F_CONST1:
      arm32_debug_instr_prologue_0(opcode);
      arm32_push_int(1, NUMBER_NUMBER);
      return;
  case F_CONST0:
      arm32_debug_instr_prologue_0(opcode);
      arm32_push_int(0, NUMBER_NUMBER);
      return;
  case F_CATCH:
      {
          int i;
          mov_reg(ra_alloc(ARM_REG_R0), ARM_REG_PC);

          rel_addr = PIKE_PC;
          /* we will move an offset into r2 later */
          for (i = 0; i < SIZEOF_ADD_SET_REG_AT; i++) nop();

          add_reg_reg(ARM_REG_R0, ARM_REG_R0, ARM_REG_R2);
      }
      break;
  case F_POP_VALUE:
      arm32_change_sp(-1);
      arm32_free_svalue(ARM_REG_PIKE_SP, 0);
      return;
  case F_POP_TO_MARK: /* this opcode sucks noodles, introduce F_POP_TO_LOCAL(num) */
      {
          struct label done, loop;
          enum arm32_register reg = ra_alloc_persistent();

          label_init(&done);
          label_init(&loop);

          arm32_load_sp_reg();

          arm32_pop_mark(reg);
          cmp_reg_reg(ARM_REG_PIKE_SP, reg);
          /* jump if pike_sp <= reg */
          b_imm(label_dist(&done), ARM_COND_LS);

          label_generate(&loop);

          arm32_sub_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, 1*sizeof(struct svalue));
          arm32_free_svalue(ARM_REG_PIKE_SP, 0);

          cmp_reg_reg(ARM_REG_PIKE_SP, reg);
          /* jump if pike_sp > reg */
          b_imm(label_dist(&loop), ARM_COND_HI);

          arm32_store_sp_reg();

          label_generate(&done);
          ra_free(reg);
      }
      return;
  case F_EQ:
  case F_NE:
  case F_GT:
  case F_GE:
  case F_LT:
  case F_LE:
      {
          struct label real_cmp, real_pop, end;
          enum arm32_register reg, type1, type2, tmp;
          enum arm32_condition cond;
          int (*cmp)(const struct svalue *a, const struct svalue *b);
          int swap = 0, negate = 0;

          label_init(&real_cmp);
          label_init(&real_pop);
          label_init(&end);

          type1 = ra_alloc_any();
          type2 = ra_alloc_any();

          arm32_load_sp_reg();

          load_reg_imm(type1, ARM_REG_PIKE_SP, -2*sizeof(struct svalue));
          load_reg_imm(type2, ARM_REG_PIKE_SP, -1*sizeof(struct svalue));

          switch (opcode) {
          case F_NE:
              negate = 1;
              /* FALL THROUGH */
          case F_EQ:
              cmp = is_eq;

              arm32_jump_real_cmp(&real_cmp, type1, type2);

              reg = ra_alloc_persistent();
              tmp = ra_alloc_any();

              load_reg_imm(tmp, ARM_REG_PIKE_SP, -2*sizeof(struct svalue)+OFFSETOF(svalue, u));
              load_reg_imm(reg, ARM_REG_PIKE_SP, -1*sizeof(struct svalue)+OFFSETOF(svalue, u));

              /* TODO: make those shorter */
              if (opcode == F_EQ) {
                  xors_reg_reg(reg, tmp, reg);
                  add_to_program(set_cond(gen_mov_imm(reg, 1, 0), ARM_COND_NZ));
              } else {
                  xors_reg_reg(reg, tmp, reg);
                  add_to_program(set_cond(gen_mov_imm(reg, 1, 0), ARM_COND_Z));
                  add_to_program(set_cond(gen_mov_imm(reg, 0, 0), ARM_COND_NZ));
              }

              /* jump to real pop, if not both integers */
              cond = arm32_ne_types(type1, type2, TYPE_SUBTYPE(PIKE_T_INT, NUMBER_NUMBER));
              b_imm(label_dist(&real_pop), cond);

              break;
          case F_GT:
          case F_GE:
          case F_LE:
          case F_LT:
              cond = arm32_ne_types(type1, type2, TYPE_SUBTYPE(PIKE_T_INT, NUMBER_NUMBER));
              b_imm(label_dist(&real_cmp), cond);

              reg = ra_alloc_persistent();
              tmp = ra_alloc_any();

              load_reg_imm(tmp, ARM_REG_PIKE_SP, -2*sizeof(struct svalue)+OFFSETOF(svalue, u));
              load_reg_imm(reg, ARM_REG_PIKE_SP, -1*sizeof(struct svalue)+OFFSETOF(svalue, u));

              cmp_reg_reg(tmp, reg);

              switch (opcode) {
              case F_GT:
                  swap = 1;
                  cmp = is_lt;
                  add_to_program(set_cond(gen_mov_imm(reg, 1, 0), ARM_COND_GT));
                  add_to_program(set_cond(gen_mov_imm(reg, 0, 0), ARM_COND_LE));
                  break;
              case F_GE:
                  cmp = is_le;
                  swap = 1;
                  add_to_program(set_cond(gen_mov_imm(reg, 1, 0), ARM_COND_GE));
                  add_to_program(set_cond(gen_mov_imm(reg, 0, 0), ARM_COND_LT));
                  break;
              case F_LT:
                  cmp = is_lt;
                  add_to_program(set_cond(gen_mov_imm(reg, 1, 0), ARM_COND_LT));
                  add_to_program(set_cond(gen_mov_imm(reg, 0, 0), ARM_COND_GE));
                  break;
              case F_LE:
                  cmp = is_le;
                  add_to_program(set_cond(gen_mov_imm(reg, 1, 0), ARM_COND_LE));
                  add_to_program(set_cond(gen_mov_imm(reg, 0, 0), ARM_COND_GT));
                  break;
              }
              break;
          }

          ra_free(type1);
          ra_free(type2);
          ra_free(tmp);
          /* SIMPLE POP INT: */

          arm32_sub_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue));
          store_reg_imm(reg, ARM_REG_PIKE_SP, -1*sizeof(struct svalue)+OFFSETOF(svalue, u));

          arm32_store_sp_reg();

          b_imm(label_dist(&end), ARM_COND_AL);
          /* COMPLEX CMP: */
          label_generate(&real_cmp);

          ra_alloc(ARM_REG_ARG1);
          ra_alloc(ARM_REG_ARG2);

          if (swap) {
              arm32_sub_reg_int(ARM_REG_ARG2, ARM_REG_PIKE_SP, 2*sizeof(struct svalue));
              arm32_sub_reg_int(ARM_REG_ARG1, ARM_REG_PIKE_SP, 1*sizeof(struct svalue));
          } else {
              arm32_sub_reg_int(ARM_REG_ARG1, ARM_REG_PIKE_SP, 2*sizeof(struct svalue));
              arm32_sub_reg_int(ARM_REG_ARG2, ARM_REG_PIKE_SP, 1*sizeof(struct svalue));
          }

          arm32_call(cmp);

          if (negate) {
              arm32_xor_reg_int(reg, ARM_REG_RVAL, 1);
          } else {
              mov_reg(reg, ARM_REG_RVAL);
          }

          ra_free(ARM_REG_ARG1);
          ra_free(ARM_REG_ARG2);

          /* COMPLEX POP: */
          label_generate(&real_pop);

          arm32_free_svalue_off(ARM_REG_PIKE_SP, -1, 0);
          arm32_free_svalue_off(ARM_REG_PIKE_SP, -2, 0);
          /* the order of the free and pop is important, because really_free_svalue should not
           * use that region of the stack we are trying to free */
          arm32_sub_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, 2*sizeof(struct svalue));

          arm32_push_int_reg(reg, NUMBER_NUMBER);

          /* END: */
          label_generate(&end);

          ra_free(reg);
      }
      return;
  case F_MARK:
      arm32_load_sp_reg();
      arm32_mark(ARM_REG_PIKE_SP, 0);
      return;
  case F_MARK2:
      ins_f_byte(F_MARK);
      ins_f_byte(F_MARK);
      return;
  case F_MARK_AND_CONST0:
      ins_f_byte(F_MARK);
      ins_f_byte(F_CONST0);
      return;
  case F_MARK_AND_CONST1:
      ins_f_byte(F_MARK);
      ins_f_byte(F_CONST1);
      return;
  case F_MAKE_ITERATOR:
      arm32_debug_instr_prologue_0(opcode);
      arm32_call_efun(f_get_iterator, 1);
      return;
  case F_ADD:
      arm32_debug_instr_prologue_0(opcode);
      arm32_call_efun(f_add, 2);
      return;
  case F_ADD_INTS:
      {
          struct label end, slow;
          enum arm32_register reg1, reg2;
          enum arm32_condition cond;

          label_init(&end);
          label_init(&slow);

          arm32_debug_instr_prologue_0(opcode);

          reg1 = ra_alloc_any();
          reg2 = ra_alloc_any();

          arm32_load_sp_reg();

          load_reg_imm(reg1, ARM_REG_PIKE_SP, -2*sizeof(struct svalue));
          load_reg_imm(reg2, ARM_REG_PIKE_SP, -1*sizeof(struct svalue));

          cond = arm32_ne_types(reg1, reg2, TYPE_SUBTYPE(PIKE_T_INT, NUMBER_NUMBER));
          b_imm(label_dist(&slow), cond);

          load_reg_imm(reg1, ARM_REG_PIKE_SP, -2*sizeof(struct svalue)+OFFSETOF(svalue, u));
          load_reg_imm(reg2, ARM_REG_PIKE_SP, -1*sizeof(struct svalue)+OFFSETOF(svalue, u));

          adds_reg_reg(reg1, reg1, reg2);

          add_to_program(
            set_cond(
                gen_store_reg_imm(reg1, ARM_REG_PIKE_SP, -2*sizeof(struct svalue)+OFFSETOF(svalue, u)),
                ARM_COND_VC));

          ra_free(reg1);
          ra_free(reg2);

          b_imm(label_dist(&end), ARM_COND_VC);
          label_generate(&slow);
          ra_alloc(ARM_REG_ARG1);
          arm32_mov_int(ARM_REG_ARG1, 2);
          arm32_call(f_add);
          ra_free(ARM_REG_ARG1);
          label_generate(&end);
          arm32_sub_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue));
          arm32_store_sp_reg();

          return;
      }
  }

  arm32_call_c_opcode(opcode);


  if (opcode == F_CATCH) ra_free(ARM_REG_R0);

  if (flags & I_RETURN) {
      enum arm32_register kludge_reg = ARM_REG_NONE;

      if (opcode == F_RETURN_IF_TRUE) {
          kludge_reg = ra_alloc_persistent();
          /* -4 == 4*(JUMP_EPILOGUE_SIZE - 2) */
          sub_reg_imm(kludge_reg, ARM_REG_PC, 4, 0);
      }

      arm32_ins_maybe_exit();

      if (opcode == F_RETURN_IF_TRUE) {
          arm32_rel_cond_jmp(ARM_REG_R0, kludge_reg, ARM_COND_EQ, 2);
          bx_reg(ARM_REG_R0);
          ra_free(kludge_reg);
          return;
      }
  }
  if (flags & I_JUMP) {
    /* This is the code that JUMP_EPILOGUE_SIZE compensates for. */
    bx_reg(ARM_REG_RVAL);

    compiler_state.flags &= ~FLAG_FP_LOADED;

    if (opcode == F_CATCH) {
        arm32_mov_int_at(rel_addr, 4*(PIKE_PC - rel_addr - 1), ARM_REG_R2);
    }
  }
}

void ins_f_byte(unsigned int opcode)
{
  record_opcode(opcode, 0);

  low_ins_f_byte(opcode);
}

void ins_f_byte_with_arg(unsigned int opcode, INT32 arg1)
{
  struct label done;
  label_init(&done);

  record_opcode(opcode, 0);

  switch (opcode) {
  case F_NUMBER:
      arm32_debug_instr_prologue_1(opcode, arg1);
      arm32_push_int(arg1, NUMBER_NUMBER);
      return;
  case F_NEG_NUMBER:
      if (!INT32_NEG_OVERFLOW(arg1)) {
        arm32_debug_instr_prologue_1(opcode, arg1);
        arm32_push_int(-arg1, 0);
        return;
      }
      break;
  case F_SUBTRACT_INT:
    {
      struct label fallback;
      enum arm32_register tmp;
      arm32_debug_instr_prologue_1(opcode, arg1);

      tmp = ra_alloc_any();
      label_init(&fallback);
      arm32_load_sp_reg();
      load_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+0);
      cmp_reg_imm(tmp, 0, 0);
      b_imm(label_dist(&fallback), ARM_COND_NE);
      load_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4);
      arm32_subs_reg_int(tmp, tmp, arg1);
      b_imm(label_dist(&fallback), ARM_COND_VS);
      store_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4);
      ra_free(tmp);
      b_imm(label_dist(&done), ARM_COND_AL);
      label_generate(&fallback);
      break;
    }
  case F_ADD_INT:
    {
      struct label fallback;
      unsigned char imm, rot;
      enum arm32_register tmp;
      arm32_debug_instr_prologue_1(opcode, arg1);

      tmp = ra_alloc_any();
      label_init(&fallback);
      arm32_load_sp_reg();
      load_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+0);
      cmp_reg_imm(tmp, 0, 0);
      b_imm(label_dist(&fallback), ARM_COND_NE);
      load_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4);
      arm32_adds_reg_int(tmp, tmp, arg1);
      b_imm(label_dist(&fallback), ARM_COND_VS);
      store_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4);
      ra_free(tmp);
      b_imm(label_dist(&done), ARM_COND_AL);
      label_generate(&fallback);
      break;
    }
  case F_MARK_AND_CONST0:
      ins_f_byte(F_MARK);
      ins_f_byte(F_CONST0);
      return;
  case F_MARK_AND_CONST1:
      ins_f_byte(F_MARK);
      ins_f_byte(F_CONST1);
      return;
  case F_MARK_AND_STRING:
      ins_f_byte(F_MARK);
      ins_f_byte_with_arg(F_STRING, arg1);
      return;
  case F_MARK_AND_GLOBAL:
      ins_f_byte(F_MARK);
      ins_f_byte_with_arg(F_GLOBAL, arg1);
      return;
  case F_MARK_AND_LOCAL:
      ins_f_byte(F_MARK);
      ins_f_byte_with_arg(F_LOCAL, arg1);
      return;
  case F_LOCAL:
      arm32_debug_instr_prologue_1(opcode, arg1);
      arm32_load_locals_reg();
      arm32_push_svaluep_off(ARM_REG_PIKE_LOCALS, arg1);
      return;
  case F_MARK_AT:
      arm32_debug_instr_prologue_1(opcode, arg1);
      arm32_load_locals_reg();
      arm32_mark(ARM_REG_PIKE_LOCALS, arg1);
      return;
  case F_STRING:
      {
          enum arm32_register treg = ra_alloc_any(),
                              vreg = ra_alloc_any();

          arm32_load_fp_reg();

          load_reg_imm(vreg, ARM_REG_PIKE_FP, OFFSETOF(pike_frame, context));
          load_reg_imm(vreg, vreg, OFFSETOF(inherit, prog));
          load_reg_imm(vreg, vreg, OFFSETOF(program, strings));
          load_reg_imm(vreg, vreg, arg1*sizeof(struct pike_string*));

          arm32_mov_int(treg, TYPE_SUBTYPE(PIKE_T_STRING, 0));

          arm32_push_ptr_type(treg, vreg);

          ra_free(treg);
          ra_free(vreg);
          return;
      }
  case F_ASSIGN_LOCAL_AND_POP:
  case F_ASSIGN_LOCAL:
      {
          enum arm32_register tmp;
          arm32_debug_instr_prologue_1(opcode, arg1);
          arm32_load_locals_reg();

          tmp = ra_alloc_persistent();
          arm32_add_reg_int(tmp, ARM_REG_PIKE_LOCALS, arg1 * sizeof(struct svalue));

          arm32_free_svalue_off(tmp, 0, 0);

          arm32_load_sp_reg();
          arm32_sub_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue));

          if (opcode == F_ASSIGN_LOCAL_AND_POP) {
              arm32_store_sp_reg();
              arm32_move_svaluep_nofree(tmp, ARM_REG_PIKE_SP);
          } else {
              arm32_assign_svaluep_nofree(tmp, ARM_REG_PIKE_SP);
              arm32_add_reg_int(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue));
          }

          ra_free(tmp);
          return;
      }
  case F_PROTECT_STACK:
      {
          enum arm32_register reg;

          arm32_debug_instr_prologue_1(opcode, arg1);

          arm32_load_fp_reg();
          reg = ra_alloc_any();
          arm32_mov_int(reg, arg1);
          storeh_reg_imm(reg, ARM_REG_PIKE_FP, OFFSETOF(pike_frame, expendible_offset));

          ra_free(reg);
          return;
      }
  case F_THIS_OBJECT:
      /* NOTE: we only implement this trivial case and let the others be handled by the opcode fun */
      if (arg1) break;
      {
          enum arm32_register type, value;

          arm32_debug_instr_prologue_1(opcode, arg1);

          arm32_load_fp_reg();

          type = ra_alloc_any();
          value = ra_alloc_any();

          arm32_mov_int(type, PIKE_T_OBJECT);
          load_reg_imm(value, ARM_REG_PIKE_FP, OFFSETOF(pike_frame, current_object));
          arm32_push_ptr_type(type, value);

          ra_free(type);
          ra_free(value);
          return;
      }
  case F_SIZEOF_LOCAL:
  case F_SIZEOF_LOCAL_STRING:
      {
          ra_alloc(ARM_REG_ARG1);

          arm32_load_locals_reg();

          arm32_add_reg_int(ARM_REG_ARG1, ARM_REG_PIKE_LOCALS, arg1*sizeof(struct svalue));
          arm32_call(pike_sizeof);

          arm32_push_int_reg(ARM_REG_RVAL, NUMBER_NUMBER);

          ra_free(ARM_REG_ARG1);
          return;
      }
  }
  arm32_mov_int(ra_alloc(ARM_REG_ARG1), arg1);
  low_ins_f_byte(opcode);
  ra_free(ARM_REG_ARG1);
  label_generate(&done);
  return;
}

void ins_f_byte_with_2_args(unsigned int opcode, INT32 arg1, INT32 arg2)
{
  record_opcode(opcode, 0);

  switch (opcode) {
  case F_MARK_AND_EXTERNAL:
      ins_f_byte(F_MARK);
      ins_f_byte_with_2_args(F_EXTERNAL, arg1, arg2);
      return;
  case F_INIT_FRAME: {
          enum arm32_register tmp;
          arm32_debug_instr_prologue_2(opcode, arg1, arg2);
          arm32_load_fp_reg();

          tmp = ra_alloc_any();
          arm32_mov_int(tmp, arg2|(arg1<<16));

          store_reg_imm(tmp, ARM_REG_PIKE_FP, OFFSETOF(pike_frame, num_locals));
          ra_free(tmp);
          return;
      }
  case F_FILL_STACK:
      {
          enum arm32_register reg, treg, vreg;
          struct label skip, loop;
          label_init(&skip);
          label_init(&loop);

          arm32_load_sp_reg();
          arm32_load_locals_reg();

          reg = ra_alloc_any();
          treg = ra_alloc_any();
          vreg = ra_alloc_any();

          assert(treg < vreg);

          arm32_add_reg_int(reg, ARM_REG_PIKE_LOCALS, arg1*sizeof(struct svalue));

          cmp_reg_reg(reg, ARM_REG_PIKE_SP);
          /* jump if pike_sp >= reg */
          b_imm(label_dist(&skip), ARM_COND_LS);
          arm32_mov_int(treg, TYPE_SUBTYPE(PIKE_T_INT, arg2 ? NUMBER_UNDEFINED : NUMBER_NUMBER));
          arm32_mov_int(vreg, 0);

          label_generate(&loop);
          store_multiple(ARM_REG_PIKE_SP, ARM_MULT_IAW, RBIT(treg)|RBIT(vreg));
          cmp_reg_reg(reg, ARM_REG_PIKE_SP);
          /* jump if pike_sp < reg */
          b_imm(label_dist(&loop), ARM_COND_HI);

          arm32_store_sp_reg();
          label_generate(&skip);

          ra_free(reg);
          ra_free(treg);
          ra_free(vreg);

          return;
      }
  case F_2_LOCALS:
      ins_f_byte_with_arg(F_LOCAL, arg1);
      ins_f_byte_with_arg(F_LOCAL, arg2);
      return;
  }
  arm32_mov_int(ra_alloc(ARM_REG_ARG1), arg1);
  arm32_mov_int(ra_alloc(ARM_REG_ARG2), arg2);
  low_ins_f_byte(opcode);
  ra_free(ARM_REG_ARG1);
  ra_free(ARM_REG_ARG2);
  return;
}

int arm32_low_ins_f_jump(unsigned int opcode, int backward_jump) {
    INT32 ret;

    arm32_maybe_update_pc();

    arm32_call_c_opcode(opcode);

    /* Do we need to reload the stack pointer? */
    arm32_load_sp_reg();

    cmp_reg_imm(ARM_REG_RVAL, 0, 0);

    if (backward_jump) {
        struct label skip;
        label_init(&skip);

        b_imm(label_dist(&skip), ARM_COND_EQ);

        ret = PIKE_PC;
        b_imm(0, ARM_COND_AL);

        label_generate(&skip);
    } else {
        ret = PIKE_PC;
        b_imm(0, ARM_COND_NE);
    }

    return ret;
}

int arm32_ins_f_jump(unsigned int opcode, int backward_jump) {
    INT32 ret;

    if (!(instrs[opcode - F_OFFSET].flags & I_BRANCH))
        return -1;

    record_opcode(opcode, 0);

    switch (opcode) {
    case F_QUICK_BRANCH_WHEN_ZERO:
    case F_QUICK_BRANCH_WHEN_NON_ZERO:
        {
            enum arm32_register tmp = ra_alloc_any();
            arm32_change_sp(-1);
            load_reg_imm(tmp, ARM_REG_PIKE_SP, 4);
            cmp_reg_imm(tmp, 0, 0);
            ret = PIKE_PC;
            if (opcode == F_QUICK_BRANCH_WHEN_ZERO)
                b_imm(0, ARM_COND_Z);
            else
                b_imm(0, ARM_COND_NZ);
            ra_free(tmp);
            return ret;
        }
    case F_BRANCH:
        ret = PIKE_PC;
        b_imm(0, ARM_COND_AL);
        return ret;

    case F_BRANCH_WHEN_NE:
    case F_BRANCH_WHEN_EQ:
    case F_BRANCH_WHEN_LT:
    case F_BRANCH_WHEN_LE:
    case F_BRANCH_WHEN_GT:
    case F_BRANCH_WHEN_GE:
        {
            ra_alloc(ARM_REG_ARG1);
            ra_alloc(ARM_REG_ARG2);

            arm32_load_sp_reg();

            arm32_sub_reg_int(ARM_REG_ARG1, ARM_REG_PIKE_SP, 2*sizeof(struct svalue));
            arm32_sub_reg_int(ARM_REG_ARG2, ARM_REG_PIKE_SP, 1*sizeof(struct svalue));

            switch (opcode) {
            case F_BRANCH_WHEN_NE:
                arm32_call(is_eq);
                cmp_reg_imm(ARM_REG_RVAL, 0, 0);
                break;
            case F_BRANCH_WHEN_EQ:
                arm32_call(is_eq);
                cmp_reg_imm(ARM_REG_RVAL, 1, 0);
                break;
            case F_BRANCH_WHEN_LT:
                arm32_call(is_lt);
                cmp_reg_imm(ARM_REG_RVAL, 1, 0);
                break;
            case F_BRANCH_WHEN_LE:
                arm32_call(is_le);
                cmp_reg_imm(ARM_REG_RVAL, 1, 0);
                break;
            case F_BRANCH_WHEN_GT:
                arm32_call(is_le);
                cmp_reg_imm(ARM_REG_RVAL, 0, 0);
                break;
            case F_BRANCH_WHEN_GE:
                arm32_call(is_lt);
                cmp_reg_imm(ARM_REG_RVAL, 0, 0);
                break;
            }

            ret = PIKE_PC;
            b_imm(0, ARM_COND_EQ);

            ra_free(ARM_REG_ARG1);
            ra_free(ARM_REG_ARG2);
            return ret;
        }
    }

    return arm32_low_ins_f_jump(opcode, backward_jump);
}

int arm32_ins_f_jump_with_arg(unsigned int opcode, INT32 arg1, int backward_jump) {
    int instr;

    if (!(instrs[opcode - F_OFFSET].flags & I_BRANCH))
        return -1;

    record_opcode(opcode, 0);

    arm32_mov_int(ra_alloc(ARM_REG_ARG1), arg1);

    instr = arm32_low_ins_f_jump(opcode, backward_jump);

    ra_free(ARM_REG_ARG1);

    return instr;
}

int arm32_ins_f_jump_with_2_args(unsigned int opcode, INT32 arg1, INT32 arg2, int backward_jump) {
    int instr;

    if (!(instrs[opcode - F_OFFSET].flags & I_BRANCH))
        return -1;

    record_opcode(opcode, 0);

    arm32_mov_int(ra_alloc(ARM_REG_ARG1), arg1);
    arm32_mov_int(ra_alloc(ARM_REG_ARG2), arg2);

    instr = arm32_low_ins_f_jump(opcode, backward_jump);

    ra_free(ARM_REG_ARG1);
    ra_free(ARM_REG_ARG2);

    return instr;
}

void arm32_update_f_jump(INT32 offset, INT32 to_offset) {
    PIKE_OPCODE_T instr = read_pointer(offset) & ~0xffffff;

    instr |= (to_offset - offset - 2)&0xffffff;

    upd_pointer(offset, instr);
}

int arm32_read_f_jump(INT32 offset) {
    int rel_addr = (int)(((unsigned INT32)read_pointer(offset) & 0xffffff) << 8) >> 8;

    return rel_addr + offset + 2;
}

unsigned INT32 extract_immediate(PIKE_OPCODE_T instr) {
    unsigned int imm = instr & 0xff;
    unsigned int rot = (instr>>8) & 0xf;

    rot *= 2;

    return (imm >> rot) | (imm << (32-rot));
}

#define CASE(X, Y) case ARM_REG_ ## X: return #Y;
MACRO const char *reg_to_name(enum arm32_register reg) {
    switch (reg&15) {
        CASE(R0, r0);
        CASE(R1, r1);
        CASE(R2, r2);
        CASE(R3, r3);
        CASE(R4, r4);
        CASE(R5, r5);
        CASE(R6, r6);
        CASE(PIKE_LOCALS, pike_locals);
        CASE(PIKE_IP, pike_ip);
        CASE(PIKE_SP, pike_sp);
        CASE(PIKE_FP, pike_fp);
        CASE(R11, r11);
        CASE(R12, r12);
        CASE(R13, $sp);
        CASE(LR, $lr);
        CASE(PC, $pc);
    }

    return "undisclosed location";
}
#undef CASE

#define CASE(X, Y) case ARM_MULT_ ## X: *W = ""; return #Y
#define CASEW(X, Y) case ARM_MULT_ ## X: *W = "!"; return #Y
MACRO const char *mult_mode_to_name(PIKE_OPCODE_T instr, const char **W) {
    instr &= (1<<24)|(1<<23)|(1<<21);
    switch (instr) {
        CASE(DB, db);
        CASE(DA, da);
        CASE(IB, ib);
        CASE(IA, ia);
        CASEW(DBW, db);
        CASEW(DAW, da);
        CASEW(IBW, ib);
        CASEW(IAW, ia);
    }

    *W = "";
    return "?";
}
#undef CASE
#undef CASEW

#define CASE(X, Y) case ARM_PROC_ ## X: return #Y
MACRO const char *proc_to_name(enum data_proc_instr op) {
    op >>= 21;

    switch (op&0xf) {
        CASE(AND, and);
        CASE(XOR, xor);
        CASE(SUB, sub);
        CASE(RSB, rsb);
        CASE(ADD, add);
        CASE(ADC, adc);
        CASE(SBC, sbc);
        CASE(RSC, rsc);
        CASE(TST, tst);
        CASE(TEQ, teq);
        CASE(CMP, cmp);
        CASE(CMN, cmn);
        CASE(OR, or);
        CASE(MOV, mov);
        CASE(BIC, bic);
        CASE(MVN, mvn);
    }

    return "special operation";
}
#undef CASE

#define CASE(X, Y) case ARM_COND_ ## X: return #Y
MACRO const char *cond_to_name(PIKE_OPCODE_T op) {
    op &= (0xf << 28);

    switch (op) {
        CASE(EQ, eq);
        CASE(NE, ne);
        CASE(CS, cs);
        CASE(CC, cc);
        CASE(MI, mi);
        CASE(PL, pl);
        CASE(VS, vs);
        CASE(VC, vc);
        CASE(HI, hi);
        CASE(LS, ls);
        CASE(GE, ge);
        CASE(LT, lt);
        CASE(GT, gt);
        CASE(LE, le);
    }

    return "";
}
#undef CASE

#define CASE(X, Y) case ARM_SHIFT_ ## X: return #Y
MACRO const char *shift_to_name(PIKE_OPCODE_T op) {
    op >>= 5;

    switch (op&3) {
        CASE(LSL, lsl);
        CASE(LSR, lsr);
        CASE(ASR, asr);
        CASE(ROT, rot);
    }

    return "special shift";
}
#undef CASE

MACRO int check_instr(PIKE_OPCODE_T mask, PIKE_OPCODE_T instr, const char **cond, const char **status) {
    int ret;

    /* unset ARM_COND_AL */
    if (cond) mask = set_cond(mask, 0);

    if (mask == (instr & mask)) {
        if (cond) *cond = cond_to_name(instr);
        if (status) *status = (instr & (1<<20)) ? "s" : "";
        return 1;
    }

    return 0;
}

MACRO char *get_registers(PIKE_OPCODE_T field, char *outbuf /* >= 256 bytes */) {
    int i, first = 1;

    char *ret = outbuf;

    *outbuf = 0;
    outbuf += sprintf(outbuf, "{");

    field &= 0xffff;

    for (i = 0; i < 16; i++) {
        if (RBIT(i) & field) {
            if (!first)
                outbuf += sprintf(outbuf, ", ");
            else
                first = 0;
            outbuf += sprintf(outbuf, "%s", reg_to_name(i));
        }
    }

    outbuf += sprintf(outbuf, "}");

    return ret;
}

char *interpret_location(enum arm32_register reg, int offset) {
    char *ret;

    int sv_mod = abs(offset) % sizeof(struct svalue);
    int sv_div = (offset - sv_mod) / (int)sizeof(struct svalue);

    switch (reg) {
    case ARM_REG_PIKE_IP:
        if (offset == OFFSETOF(Pike_interpreter_struct, stack_pointer)) {
            return strdup("Pike_sp");
        }
        if (offset == OFFSETOF(Pike_interpreter_struct, mark_stack_pointer)) {
            return strdup("Pike_mark_sp");
        }
        if (offset == OFFSETOF(Pike_interpreter_struct, frame_pointer)) {
            return strdup("Pike_fp");
        }
        break;
    case ARM_REG_PIKE_FP:
        if (offset == OFFSETOF(pike_frame, locals)) {
            return strdup("Pike_fp->locals");
        }
        if (offset == OFFSETOF(pike_frame, pc)) {
            return strdup("Pike_fp->pc");
        }
        break;
    case ARM_REG_PIKE_LOCALS:
        if (!(sv_mod % 4)) {
            if (-1 == asprintf(&ret, "Pike_fp->locals[%d].%s", sv_div,
                               !(sv_mod) ? "type" : "value"))
                return NULL;
            return ret;
        } else {
            return strdup("Pike_fp->locals[MISALIGNED!!!]");
        }
        break;
    case ARM_REG_PIKE_SP:
        if (!(sv_mod % 4)) {
            if (-1 == asprintf(&ret, "Pike_sp[%d].%s", sv_div,
                               !(sv_mod) ? "type" : "value"))
                return NULL;
            return ret;
        } else {
            return strdup("Pike_sp[MISALIGNED!!!]");
        }
        break;
    default:
        break;
    }

    return NULL;
}

void arm32_disassemble_code(PIKE_OPCODE_T *addr, size_t bytes) {
    size_t i;
    size_t opcodes = bytes / sizeof(PIKE_OPCODE_T);

    char **lines = xcalloc(opcodes, sizeof(char*));
    char **comments = xcalloc(opcodes, sizeof(char*));
    char *labels = xcalloc(opcodes, sizeof(char));
    char label = 'A';
    char reglist[256];

    unsigned int imm;

    for (i = 0; i < opcodes; i++) {
        PIKE_OPCODE_T instr = addr[i];

        /* condition and status flag */
        const char *C, *S, *W;

#define CHECK(op)    check_instr(gen_ ## op, instr, 0, 0)
#define CHECK_C(op)    check_instr(gen_ ## op, instr, &C, 0)
#define CHECK_CS(op)    check_instr(gen_ ## op, instr, &C, &S)
#define CHECK_S(op)    check_instr(gen_ ## op, instr, 0, &S)
#define PRINT(fmt, X...)     do {                               \
    if (-1 == asprintf(lines+i, "  %p:\t" fmt, addr+i,## X))    \
        Pike_fatal("Failed to disassemble\n");                  \
} while(0)
#define COMMENT(fmt, X...)     do {                             \
    if (-1 == asprintf(comments+i, "%s " fmt,                    \
                       comments[i] ? comments[i] : "",## X))    \
        Pike_fatal("Failed to disassemble\n");                  \
} while(0)
#define LABEL(pos) (labels[pos] ? labels[pos] : (labels[pos] = label++))

        /* popcount 15 */
        if (CHECK(bx_reg(0))) {
            PRINT("bx\t%s", reg_to_name(instr));
            continue;
        }
        /* popcount 7 */
        if (CHECK_C(nop())) {
            PRINT("nop%s", C);
            continue;
        }
        /* popcount 6 */
#ifdef __ARM_ARCH_6T2__
        if (CHECK_C(mov_top(0, 0))) {
            imm = (instr & 0xfff) | (instr>>4 & 0xf000);
            if (!(instr & (1<<20))) {
                PRINT("movt%s\t%s, #%u",
                        C, reg_to_name(instr>>12), imm);
                if (imm >= 10) COMMENT("=0x%x", imm);
                continue;
            }
        }
        /* popcount 5 */
        if (CHECK_C(mov_wide(0, 0))) {
            imm = (instr & 0xfff) | (instr>>4 & 0xf000);
            if (!(instr & (1<<20))) {
                PRINT("movw%s\t%s, #%u\t",
                        C, reg_to_name(instr>>12), imm);
                if (imm >= 10) COMMENT("=0x%x", imm);
                continue;
            }
        }
#endif
        if (CHECK_C(load_multiple(0, 0, 0))) {
            /* check if its a b_imm */
            if (!(instr & (1<<25))) {
                const char *mode = mult_mode_to_name(instr, &W);
                PRINT("ldm%s%s\t%s%s, %s", mode, C, reg_to_name(instr >> 16), W,
                      get_registers(instr, reglist));
                continue;
            }
        }
        /* popcount 4 */
        if (CHECK_C(store_multiple(0, 0, 0))) {
            /* check if its a b_imm */
            if (!(instr & (1<<25))) {
                const char *mode = mult_mode_to_name(instr, &W);
                PRINT("stm%s%s\t%s%s, %s", mode, C, reg_to_name(instr >> 16), W,
                      get_registers(instr, reglist));
                continue;
            }
        }
        if (CHECK_C(mov_imm(0, 0, 0))) {
            PRINT("mov%s %s, %u", C, reg_to_name(instr>>12), imm = extract_immediate(instr));
            COMMENT("=0x%08x", imm);
            continue;
        }
        if (CHECK_C(mvn_imm(0, 0, 0))) {
            PRINT("mvn%s %s, %u", C, reg_to_name(instr>>12), imm = extract_immediate(instr));
            COMMENT("=0x%08x", ~imm);
            continue;
        }
        if (CHECK_CS(shift_reg_reg(0, 0, 0, 0))) {
            PRINT("shift reg reg");
            continue;
        }
        if (CHECK_C(cmp_reg_imm(0, 0, 0))) {
            PRINT("cmp%s\t%s, #%u", C, reg_to_name(instr>>16), imm = extract_immediate(instr));
            COMMENT("=0x%08x", imm);
            continue;
        }
        /* popcount 3 */
        if (CHECK_CS(shift_reg_imm(0, 0, 0, 0))) {
            PIKE_OPCODE_T imm = (instr >> 7) & 0x1f;
            if (imm) {
                PRINT("%s%s%s\t%s, %s, #%d", shift_to_name(instr), C, S,
                      reg_to_name(instr >> 12), reg_to_name(instr >> 16), imm);
                continue;
            }
            /* its a mov */
        }
        if (CHECK_CS(mov_reg(0, 0))) {
            PRINT("mov%s%s\t%s, %s", C, S, reg_to_name(instr >> 12), reg_to_name(instr >> 16));
            continue;
        }
        if (CHECK_C(cmp_reg_reg(0, 0))) {
            PRINT("cmp%s\t%s, %s", C,
                  reg_to_name(instr>>16),
                  reg_to_name(instr));
            continue;
        }
        if (CHECK_C(load_reg_imm(0, 0, 0)&~(1<<23))) {
            int offset = instr & 0xfff;
            if (!(instr & (1 << 23))) offset = -offset;
            if (offset) {
                PRINT("ldr%s\t%s, [%s, #%d]", C, reg_to_name(instr >> 12),
                      reg_to_name(instr >> 16), offset);
            } else {
                PRINT("ldr%s\t%s, [%s]", C, reg_to_name(instr >> 12),
                      reg_to_name(instr >> 16));
            }
            char * loc = interpret_location((instr>>16)&0xf, offset);
            if (loc) {
                COMMENT("%s", loc);
                free(loc);
            }
            continue;
        }
        if (CHECK_C(store_reg_imm(0, 0, 0)&~(1<<23))) {
            int offset = instr & 0xfff;
            if (!(instr & (1 << 23))) offset = -offset;
            if (offset) {
                PRINT("str%s\t%s, [%s, #%d]", C, reg_to_name(instr >> 12),
                      reg_to_name(instr >> 16), offset);
            } else {
                PRINT("str%s\t%s, [%s]", C, reg_to_name(instr >> 12),
                      reg_to_name(instr >> 16));
            }
            char *loc = interpret_location((instr>>16)&0xf, offset);
            if (loc) {
                COMMENT("%s", loc);
                free(loc);
            }
            continue;
        }
        /* popcount 2 */
        if (CHECK_C(b_imm(0))) {
            int dist = (int)((instr & 0xffffff) << 8) >> 8;
            PRINT("b%s\t#%d", C, dist+2);
            if (i+dist+2 > 0 && i+dist+2 < opcodes)
                COMMENT("=> %c", LABEL(i+dist+2));
            continue;
        }
        /* popcount 1 */
        if (CHECK_CS(reg_imm(0, 0, 0, 0, 0))) {
            PRINT("%s%s%s\t%s, %s, #%u",
                    proc_to_name(instr), C, S,
                    reg_to_name(instr>>12),
                    reg_to_name(instr>>16),
                    imm = extract_immediate(instr));
            if (imm > 9)
                COMMENT("=0x%08x", imm);
            continue;
        }
        if (CHECK_CS(reg_reg(0, 0, 0, 0))) {
            PRINT("%s%s%s\t%s, %s, %s",
                    proc_to_name(instr), C, S,
                    reg_to_name(instr>>12),
                    reg_to_name(instr>>16),
                    reg_to_name(instr));
            continue;
        }
        PRINT("<unknown instruction %x>", addr[i]);
    }

    for (i = 0; i < opcodes; i++) {
        size_t len;

        if (labels[i])
            fprintf(stderr, "%c:\n", labels[i]);

        len = fprintf(stderr, "%s", lines[i]);

        if (comments[i]) {
            unsigned int j, plen = 0;

            for (j = 0; j < len; j++) {
                if (lines[i][j] == '\t') {
                    plen += 8 - (plen % 8);
                } else {
                    ++plen;
                }
            }

            fprintf(stderr, "%*s;%s", plen < 50 ? 50-plen : 0, "", comments[i]);
            free(comments[i]);
        }
        free(lines[i]);
        fprintf(stderr, "\n");
    }

    free(labels);
    free(lines);
    free(comments);
}
