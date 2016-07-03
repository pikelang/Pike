/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifdef PIKE_CONCAT
#include "global.h"
#include "svalue.h"
#include "operators.h"
#include "bitvector.h"
#include "object.h"
#include "builtin_functions.h"
#include "bignum.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#define INT32 int
void add_to_program(unsigned INT32 op);
#endif

#define MACRO  ATTRIBUTE((unused)) static

enum arm32_register {
    ARM_REG_R0,
    ARM_REG_R1,
    ARM_REG_R2,
    ARM_REG_R3,

    /* free for all */
    ARM_REG_R4 = 4,
    ARM_REG_R5 = 5,
    ARM_REG_R6 = 6,
    ARM_REG_R7 = 7,

    ARM_REG_R8 = 8,
    ARM_REG_PIKE_IP = 8,
    ARM_REG_R9 = 9,
    ARM_REG_PIKE_SP = 9,
    ARM_REG_R10 = 10,
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
    ARM_COND_HI = 8 << 28,
    ARM_COND_LS = 9 << 28,
    ARM_COND_GE = 10 << 28,
    ARM_COND_LT = 11 << 28,
    ARM_COND_GT = 12 << 28,
    ARM_COND_LE = 13 << 28,
    ARM_COND_AL = 14 << 28 /* unconditional */
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

#define OPCODE_FUN ATTRIBUTE((unused,warn_unused_result)) static unsigned INT32

void arm32_call(void *ptr);

OPCODE_FUN set_status(unsigned INT32 instr) {
    return instr | (1<<19);
}

OPCODE_FUN set_cond(unsigned INT32 instr, enum arm32_condition cond) {
    return (instr&0x0fffffff)|cond;
}

OPCODE_FUN set_src1_reg(unsigned INT32 instr, enum arm32_register r) {
    return instr | (r << 16);
}

OPCODE_FUN set_dst_reg(unsigned INT32 instr, enum arm32_register r) {
    unsigned INT32 ret = instr | (r << 12);
    return ret;
}

OPCODE_FUN set_src_imm(unsigned INT32 instr, unsigned char imm, unsigned char rot) {
    return ARM_ADDR_IMMEDIATE | instr | imm | (rot << 8);
}

OPCODE_FUN set_src_reg(unsigned INT32 instr, enum arm32_register r) {
    return instr | ARM_ADDR_REGISTER | r;
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
    ARM_MULT_IAW = MODE(0, 1, 1)
#undef MODE
};

OPCODE_FUN store_multiple(enum arm32_register addr, enum arm32_multiple_mode mode,
                              unsigned INT32 registers) {
    unsigned INT32 instr = ARM_COND_AL | (1<<27) | registers | mode;

    instr |= addr << 16;
    
    return instr;
}

OPCODE_FUN load_multiple(enum arm32_register addr, enum arm32_multiple_mode mode,
                             unsigned INT32 registers) {
    unsigned INT32 r = store_multiple(addr, mode, registers) | (1<<20);

    return r;
}

OPCODE_FUN bx_reg(enum arm32_register to) {
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

OPCODE_FUN blx_reg(enum arm32_register to) {
    return bx_reg(to) | (1<<5);
}

OPCODE_FUN b_imm(INT32 dist) {
    unsigned INT32 instr = ARM_COND_AL | (1<<27)|(1<<25);

    instr |= dist & 0xffffff;

    return instr;
}

OPCODE_FUN bl_imm(INT32 dist) {
    return b_imm(dist) | (1<<24);
}

OPCODE_FUN gen_reg(enum data_proc_instr op, enum arm32_register dst, enum arm32_register src) {
    unsigned INT32 instr = ARM_ADDR_REGISTER | ARM_COND_AL;

    instr |= op << 21;
    instr = set_dst_reg(instr, dst);
    instr = set_src_reg(instr, src);

    return instr;
}

OPCODE_FUN gen_reg_reg(enum data_proc_instr op, enum arm32_register dst,
                           enum arm32_register a, enum arm32_register b) {
    unsigned INT32 instr = gen_reg(op, dst, a);

    instr = set_src1_reg(instr, b);

    return instr;
}

OPCODE_FUN gen_cmp_reg_reg(enum arm32_register a, enum arm32_register b) {
    unsigned INT32 instr = gen_reg_reg(ARM_PROC_CMP, 0, a, b);

    instr |= (1<<20);

    return instr;
}

OPCODE_FUN gen_imm(enum data_proc_instr op, enum arm32_register dst,
                       unsigned char imm, unsigned char rot) {
    unsigned INT32 instr = ARM_ADDR_IMMEDIATE | ARM_COND_AL;

    instr |= op << 21;
    instr = set_dst_reg(instr, dst);
    instr = set_src_imm(instr, imm, rot);

    return instr;
}

#define ROTR(v, n)      (((v) >> (n)) | ((v) << (32-(n))))

// returns 1 if v can be represented as a rotated imm, with imm and rot set
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

OPCODE_FUN gen_reg_imm(enum data_proc_instr op, enum arm32_register dst,
                       enum arm32_register reg, unsigned char imm, unsigned char rot) {
    unsigned INT32 instr = gen_imm(op, dst, imm, rot);

    instr = set_src1_reg(instr, reg);

    return instr;
}

OPCODE_FUN gen_cmp_reg_imm(enum arm32_register a, unsigned char imm, unsigned char rot) {
    unsigned INT32 instr = gen_reg_imm(ARM_PROC_CMP, 0, a, imm, rot);

    instr |= (1<<20);

    return instr;
}


OPCODE_FUN str_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    unsigned INT32 instr = ARM_COND_AL;

    instr = set_dst_reg(instr, dst);
    instr = set_src1_reg(instr, base);
    instr |= (1 << 26) | (1<< 24);

    if (offset < 0) {
        instr |= -offset;
    } else {
        instr |= (1<<23);
        instr |= offset;
    }

    return instr;
}

OPCODE_FUN ldr_reg_imm(enum arm32_register dst, enum arm32_register base, INT32 offset) {
    return str_reg_imm(dst, base, offset) | (1<<20);
}

#define GEN_PROC_OP(name, NAME)                                                                          \
ATTRIBUTE((unused))                                                                                      \
static PIKE_OPCODE_T name ## _reg_imm(enum arm32_register dst, enum arm32_register reg, unsigned char imm,   \
                                      unsigned char rot) {                                               \
    return gen_reg_imm(ARM_PROC_ ## NAME, dst, reg, imm, rot);                                           \
}                                                                                                        \
ATTRIBUTE((unused))                                                                                      \
static PIKE_OPCODE_T name ## _reg_reg(enum arm32_register dst, enum arm32_register a, enum arm32_register b) { \
    return gen_reg_reg(ARM_PROC_ ## NAME, dst, a, b);                                                    \
}                                                                                                        \
ATTRIBUTE((unused))                                                                                      \
static PIKE_OPCODE_T name ## s_reg_imm(enum arm32_register dst, enum arm32_register reg, unsigned char imm,  \
                                      unsigned char rot) {                                               \
    return name ## _reg_imm(dst, reg, imm, rot)|(1<<20);                                                 \
}                                                                                                        \
ATTRIBUTE((unused))                                                                                      \
static PIKE_OPCODE_T name ## s_reg_reg(enum arm32_register dst, enum arm32_register a, enum arm32_register b) {\
    return name ## _reg_reg(dst, a, b)|(1<<20);                                                          \
}

GEN_PROC_OP(sub, SUB)
GEN_PROC_OP(add, ADD)
GEN_PROC_OP(or, OR)
GEN_PROC_OP(and, AND)


OPCODE_FUN mov_reg(enum arm32_register dst, enum arm32_register src) {
    return gen_reg(ARM_PROC_MOV, dst, src);
}

OPCODE_FUN mov_imm(enum arm32_register dst, unsigned char imm, unsigned char rot) {
    return gen_imm(ARM_PROC_MOV, dst, imm, rot);
}

void arm32_set_reg(enum arm32_register dst, unsigned INT32 v) {

    add_to_program(mov_imm(dst, v, 0));

    if ((v >> 24)&0xff) {
        add_to_program(or_reg_imm(dst, dst, v >> 24, 4));
    }

    if ((v >> 16)&0xff) {
        add_to_program(or_reg_imm(dst, dst, v >> 16, 8));
    }

    if ((v >> 8)&0xff) {
        add_to_program(or_reg_imm(dst, dst, v >> 8, 12));
    }
}


#define SIZEOF_ADD_SET_REG_AT   4
static void arm32_set_reg_at(unsigned INT32 offset, unsigned INT32 v,
                           enum arm32_register dst) {
    upd_pointer(offset++, mov_imm(dst, v, 0));
    upd_pointer(offset++, or_reg_imm(dst, dst, v >> 24, 4));
    upd_pointer(offset++, or_reg_imm(dst, dst, v >> 16, 8));
    upd_pointer(offset++, or_reg_imm(dst, dst, v >> 8, 12));
}

/*
 * "High" level interface
 */

#define FLAG_SP_LOADED  1
#define FLAG_FP_LOADED  2
#define FLAG_SP_CHANGED 4

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
};

static void label_init(struct label *l) {
    l->list = NULL;
}

MACRO INT32 label_dist(struct label *l) {
    l->list = add_to_list(l->list, PIKE_PC);
    return 0;
}

MACRO void label_generate(struct label *l) {
    unsigned INT32 loc = PIKE_PC;

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

MACRO void ra_init(void) {
    /* all register r0 through r10 are unused
     *
     */
    compiler_state.free = RBIT(0)|RBIT(1)|RBIT(2)|RBIT(3)|RBIT(4)|RBIT(5);
    compiler_state.dirt = 0;
    compiler_state.push_addr = -1;
    compiler_state.flags = 0;
    // FIXME: not quite the place
    instrs[F_CATCH - F_OFFSET].address = inter_return_opcode_F_CATCH;
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

    /* we dont want 0-3 */
    free &= 0xfff0;

    if (!free)
        Pike_fatal("No register left.\n");

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
    //add_to_program(mov_reg(ARM_REG_IP, ARM_REG_SP));
    add_push();
    add_to_program(mov_reg(ARM_REG_PIKE_IP, ARM_REG_R0));
    //add_to_program(sub_reg_imm(ARM_REG_FP, ARM_REG_IP, 4, 0));
}

#define EPILOGUE_SIZE 2
MACRO void arm32_epilogue(void) {
    add_pop();
    add_to_program(bx_reg(ARM_REG_LR));
}

void arm32_call(void *ptr) {
    unsigned INT32 v = (char*)ptr - (char*)NULL;
    enum arm32_register tmp = ra_alloc_any();

    //add_to_program(MOV_REG(ARM_REG_R0, ARM_REG_R2));
    arm32_set_reg(tmp, v);
    add_to_program(blx_reg(tmp));
    ra_free(tmp);
}


void arm32_flush_instruction_cache(void *addr, size_t len) {
    __clear_cache(addr, (char*)addr+len);
}

#ifdef PIKE_CONCAT
static void arm32_load_sp_reg(void) {
    if (!(compiler_state.flags & FLAG_SP_LOADED)) {
        INT32 offset = OFFSETOF(Pike_interpreter_struct, stack_pointer);

        compiler_state.flags |= FLAG_SP_LOADED;

        add_to_program(
           ldr_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_IP,
                       offset));
    }
}

static void arm32_store_sp_reg(void) {
    if (compiler_state.flags & FLAG_SP_CHANGED) {
        INT32 offset = OFFSETOF(Pike_interpreter_struct, stack_pointer);
        if (!(compiler_state.flags & FLAG_SP_LOADED)) {
            Pike_fatal("pike sp not loaded into register.\n");
        }
        add_to_program(
            str_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_IP, offset));
        compiler_state.flags &= ~FLAG_SP_CHANGED;
    }
}

static void arm32_load_fp_reg(void) {
    if (!(compiler_state.flags & FLAG_FP_LOADED)) {
        INT32 offset = OFFSETOF(Pike_interpreter_struct, frame_pointer);
        /* load Pike_interpreter_pointer->frame_pointer into ARM_REG_PIKE_FP */
        add_to_program(ldr_reg_imm(ARM_REG_PIKE_FP, ARM_REG_PIKE_IP, offset));
        compiler_state.flags |= FLAG_FP_LOADED;
    }
}

static void arm32_change_sp(INT32 offset) {
    arm32_load_sp_reg();
    offset *= sizeof(struct svalue);
    if (offset < 0) {
        offset = -offset;
        add_to_program(sub_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue), 0));
    } else {
        add_to_program(sub_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue), 0));
    }
    compiler_state.flags |= FLAG_SP_CHANGED;
}

static void arm32_flush_dirty_regs(void) {
    arm32_store_sp_reg();
}

static void arm32_push_int(unsigned INT32 value, int subtype) {
    unsigned INT32 combined = TYPE_SUBTYPE(PIKE_T_INT, subtype);
    enum arm32_register tmp1 = ra_alloc_any(), tmp2 = ra_alloc_any();

    arm32_load_sp_reg();

    arm32_set_reg(tmp1, combined);
    arm32_set_reg(tmp2, value);

    add_to_program(store_multiple(ARM_REG_PIKE_SP, ARM_MULT_IAW, RBIT(tmp1)|RBIT(tmp2)));

    ra_free(tmp1);
    ra_free(tmp2);

    compiler_state.flags |= FLAG_SP_CHANGED;

    arm32_flush_dirty_regs();
}

void arm32_flush_codegen_state(void) {
    //fprintf(stderr, "flushing codegen state.\n");
    compiler_state.flags = 0;
}

void arm32_start_function(int UNUSED(no_pc)) {
    ra_init();
}

void arm32_end_function(int UNUSED(no_pc)) {
    unsigned INT32 registers, instr;
    struct location_list_entry *e;

    registers = RBIT(ARM_REG_PIKE_SP)
                |RBIT(ARM_REG_PIKE_IP)
                |RBIT(ARM_REG_PIKE_FP)
                |RBIT(ARM_REG_LR)
                |RBIT(4)
                |RBIT(5);

    e = compiler_state.pop_list;

    instr = load_multiple(ARM_REG_SP, ARM_MULT_IAW, registers);
    while (e) {
        upd_pointer(e->location, instr);
        e = e->next;
    }

    e = compiler_state.push_list;

    instr = store_multiple(ARM_REG_SP, ARM_MULT_DBW, registers);

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
    add_to_program(sub_reg_imm(tmp, ARM_REG_PC, v, 0));
    add_to_program(str_reg_imm(tmp, ARM_REG_PIKE_FP, OFFSETOF(pike_frame, pc)));
    ra_free(tmp);
}

void arm32_ins_entry(void) {
    arm32_prologue();
    arm32_flush_codegen_state();
}

#if 0
void arm32_ins_exit(void) {
    arm32_set_reg(ARM_REG_R0, -1);
    arm32_epilogue();
}
#endif

static void add_rel_cond_jmp(enum arm32_register reg, enum arm32_register b, enum arm32_condition cond, int jmp) {
    add_to_program(gen_cmp_reg_reg(reg, b));
    add_to_program(set_cond(b_imm(jmp-2), cond));
}

static void arm32_ins_maybe_exit(void) {
    arm32_set_reg(ARM_REG_R1, -1);
    add_rel_cond_jmp(ARM_REG_R0, ARM_REG_R1, ARM_COND_NE, EPILOGUE_SIZE+1);
    arm32_epilogue();
}

static void low_ins_call(void *addr) {
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

  arm32_call(addr);
}

static void arm32_call_c_function(void * addr) {
    arm32_store_sp_reg();
    compiler_state.flags &= ~FLAG_SP_LOADED;
    compiler_state.flags &= ~FLAG_FP_LOADED;
    arm32_call(addr);
}

static void arm32_call_c_opcode(unsigned int opcode) {
  void *addr = instrs[opcode-F_OFFSET].address;
  int flags = instrs[opcode-F_OFFSET].flags;

  if ((flags & I_UPDATE_SP) && (compiler_state.flags & FLAG_SP_LOADED)) {
    compiler_state.flags &= ~FLAG_SP_LOADED;
  }
  if (flags & I_UPDATE_M_SP) {}
  if ((flags & I_UPDATE_FP) && (compiler_state.flags & FLAG_FP_LOADED)) {
    compiler_state.flags &= ~FLAG_FP_LOADED;
  }

  low_ins_call(addr);
}

void break_my_arm(void) {
}

MACRO void arm32_free_svalue_off(enum arm32_register src, int off, int guaranteed) {
    unsigned INT32 combined = TYPE_SUBTYPE(MIN_REF_TYPE, 0);
    unsigned char imm, rot;
    struct label end;
    enum arm32_register reg = ra_alloc(ARM_REG_R0);
    enum arm32_register tmp = ra_alloc_any();

    off *= sizeof(struct svalue);

    label_init(&end);

    // TODO: works great, but maybe not for your usecase
    arm32_call_c_function(break_my_arm);

    add_to_program(ldr_reg_imm(reg, src, off));

    if (arm32_make_imm(combined, &imm, &rot)) {
        add_to_program(ands_reg_imm(reg, reg, imm, rot));
    } else {
        arm32_set_reg(tmp, combined);
        add_to_program(ands_reg_reg(reg, reg, tmp));
    }

    add_to_program(set_cond(b_imm(label_dist(&end)), ARM_COND_Z));

    add_to_program(ldr_reg_imm(tmp, src, off+OFFSETOF(svalue, u)));

    add_to_program(ldr_reg_imm(reg, tmp, OFFSETOF(pike_string, refs)));
    add_to_program(subs_reg_imm(reg, reg, 1, 0));
    add_to_program(str_reg_imm(reg, tmp, OFFSETOF(pike_string, refs)));

    add_to_program(set_cond(b_imm(label_dist(&end)), ARM_COND_NZ));

    if (off > 0) {
        add_to_program(add_reg_imm(reg, src, off, 0));
    } else if (off < 0 ) {
        add_to_program(sub_reg_imm(reg, src, -off, 0));
    } else {
        add_to_program(mov_reg(reg, src));
    }

    arm32_call_c_function(really_free_svalue);

    label_generate(&end);
    ra_free(reg);
    ra_free(tmp);
}


static void arm32_free_svalue(enum arm32_register reg, int guaranteed_ref) {
    arm32_free_svalue_off(reg, 0, guaranteed_ref);
}

static void arm32_mark(int offset) {
  arm32_load_sp_reg();

  enum arm32_register tmp = ra_alloc_any();

  add_to_program(ldr_reg_imm(tmp, ARM_REG_PIKE_IP, OFFSETOF(Pike_interpreter_struct, mark_stack_pointer)));

  if (offset) {
    enum arm32_register tmp2 = ra_alloc_any();

    offset *= sizeof(struct svalue);

    if (offset > 0) {
      add_to_program(add_reg_imm(tmp2, ARM_REG_PIKE_SP, offset, 0));
    } else {
      add_to_program(sub_reg_imm(tmp2, ARM_REG_PIKE_SP, -offset, 0));
    }
    add_to_program(str_reg_imm(tmp2, tmp, 0));

    ra_free(tmp2);
  } else {
    add_to_program(str_reg_imm(ARM_REG_PIKE_SP, tmp, 0));
  }

  add_to_program(add_reg_imm(tmp, tmp, sizeof(void*), 0));
  add_to_program(str_reg_imm(tmp, ARM_REG_PIKE_IP, OFFSETOF(Pike_interpreter_struct, mark_stack_pointer)));

  ra_free(tmp);
}


static void low_ins_f_byte(unsigned int b)
{
  int flags;
  INT32 rel_addr = rel_addr;

#ifdef PIKE_DEBUG
  if(b-F_OFFSET>255)
    Pike_error("Instruction too big %d\n",b);
#endif

  flags = instrs[b-F_OFFSET].flags;

  switch (b) {
  case F_UNDEFINED:
      arm32_push_int(0, NUMBER_UNDEFINED);
      return;
  case F_CONST_1:
      arm32_push_int(-1, NUMBER_NUMBER);
      return;
  case F_CONST1:
      arm32_push_int(1, NUMBER_NUMBER);
      return;
  case F_CONST0:
      arm32_push_int(0, NUMBER_NUMBER);
      return;
  case F_CATCH:
      {
          int i;
          //add_to_program(0);
          //add_to_program(LDR_IMM(ARM_REG_R12, ARM_REG_R0, 0));
          //add_to_program(ADD_REG_IN
          add_to_program(mov_reg(ra_alloc(ARM_REG_R0), ARM_REG_PC));

          rel_addr = PIKE_PC;
          /* we will move an offset into R8 */
          for (i = 0; i < SIZEOF_ADD_SET_REG_AT; i++)
            add_to_program(0);

          add_to_program(add_reg_reg(ARM_REG_R0, ARM_REG_R0, ARM_REG_R2));
      }
      break;
  case F_POP_VALUE:
      arm32_load_sp_reg();
      add_to_program(sub_reg_imm(ARM_REG_PIKE_SP, ARM_REG_PIKE_SP, sizeof(struct svalue), 0));
      compiler_state.flags |= FLAG_SP_CHANGED;
      arm32_free_svalue(ARM_REG_PIKE_SP, 0);
      return;
  case F_MARK:
      arm32_mark(0);
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
  }

  arm32_call_c_opcode(b);

  if (b == F_CATCH) ra_free(ARM_REG_R0);

  if (flags & I_RETURN) {
      enum arm32_register kludge_reg = ARM_REG_NONE;

      if (b == F_RETURN_IF_TRUE) {
          kludge_reg = ra_alloc_persistent();
          /* -4 == 4*(JUMP_EPILOGUE_SIZE - 2) */
          add_to_program(sub_reg_imm(kludge_reg, ARM_REG_PC, 4, 0));
      }
    
      arm32_ins_maybe_exit();

      if (b == F_RETURN_IF_TRUE) {
          add_rel_cond_jmp(ARM_REG_R0, kludge_reg, ARM_COND_EQ, 2);
          add_to_program(bx_reg(ARM_REG_R0));
          ra_free(kludge_reg);
          return;
      }
  }
  if (flags & I_JUMP) {
    /* This is the code that JUMP_EPILOGUE_SIZE compensates for. */
    add_to_program(bx_reg(ARM_REG_R0));

    if (b == F_CATCH) {
        arm32_set_reg_at(rel_addr, 4*(PIKE_PC - rel_addr - 1), ARM_REG_R2);
    }
  }
}

void ins_f_byte(unsigned int opcode)
{
  low_ins_f_byte(opcode);
}

void ins_f_byte_with_arg(unsigned int a, INT32 b)
{
  struct label done;
  label_init(&done);

  switch (a) {
  case F_NUMBER:
      arm32_push_int(b, NUMBER_NUMBER);
      return;
  case F_NEG_NUMBER:
      if (!INT32_NEG_OVERFLOW(b)) {
        arm32_push_int(-b, 0);
        return;
      }
      break;
  case F_SUBTRACT_INT:
    {
      struct label fallback;
      enum arm32_register tmp = ra_alloc_any(), tmp2;
      label_init(&fallback);
      arm32_load_sp_reg();
      add_to_program(ldr_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+0));
      add_to_program(gen_cmp_reg_imm(tmp, 0, 0));
      add_to_program(set_cond(b_imm(label_dist(&fallback)), ARM_COND_NE));
      add_to_program(ldr_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4));
      tmp2 = ra_alloc_any();
      arm32_set_reg(tmp2, b);
      add_to_program(subs_reg_reg(tmp, tmp2, tmp));
      ra_free(tmp2);
      add_to_program(set_cond(b_imm(label_dist(&fallback)), ARM_COND_VS));
      add_to_program(str_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4));
      ra_free(tmp);
      add_to_program(b_imm(label_dist(&done)));
      label_generate(&fallback);
      break;
    }
  case F_ADD_INT:
    {
      struct label fallback;
      unsigned char imm, rot;
      enum arm32_register tmp = ra_alloc_any(), tmp2;
      label_init(&fallback);
      arm32_load_sp_reg();
      add_to_program(ldr_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+0));
      add_to_program(gen_cmp_reg_imm(tmp, 0, 0));
      add_to_program(set_cond(b_imm(label_dist(&fallback)), ARM_COND_NE));
      add_to_program(ldr_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4));
      if (arm32_make_imm(b, &imm, &rot)) {
          add_to_program(adds_reg_imm(tmp, tmp, imm, rot));
      } else {
          tmp2 = ra_alloc_any();
          arm32_set_reg(tmp2, b);
          add_to_program(adds_reg_reg(tmp, tmp, tmp2));
          ra_free(tmp2);
      }
      add_to_program(set_cond(b_imm(label_dist(&fallback)), ARM_COND_VS));
      add_to_program(str_reg_imm(tmp, ARM_REG_PIKE_SP, -sizeof(struct svalue)+4));
      ra_free(tmp);
      add_to_program(b_imm(label_dist(&done)));
      label_generate(&fallback);
      break;
    }
  case F_MARK_AND_CONST0:
      ins_f_byte(F_MARK);
      ins_f_byte(F_CONST0);
      return;
  case F_MARK_AND_CONST1:
      ins_f_byte(F_MARK);
      ins_f_byte(F_CONST0);
      return;
  case F_MARK_AND_STRING:
      ins_f_byte(F_MARK);
      ins_f_byte_with_arg(F_STRING, b);
      return;
  case F_MARK_AND_GLOBAL:
      ins_f_byte(F_MARK);
      ins_f_byte_with_arg(F_GLOBAL, b);
      return;
  case F_MARK_AND_LOCAL:
      ins_f_byte(F_MARK);
      ins_f_byte_with_arg(F_LOCAL, b);
      return;
  }
  arm32_set_reg(ra_alloc(ARM_REG_R0), b);
  low_ins_f_byte(a);
  ra_free(ARM_REG_R0);
  label_generate(&done);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    INT32 b,
			    INT32 c)
{
  switch (a) {
  case F_MARK_AND_EXTERNAL:
      ins_f_byte(F_MARK);
      ins_f_byte_with_2_args(F_EXTERNAL, b, c);
      return;
  }
  arm32_set_reg(ra_alloc(ARM_REG_R0), b);
  arm32_set_reg(ra_alloc(ARM_REG_R1), c);
  low_ins_f_byte(a);
  ra_free(ARM_REG_R0);
  ra_free(ARM_REG_R1);
  return;
}
#else
unsigned INT32 *to;

void add_to_program(unsigned INT32 op) {
    *(to++) = op;
    printf("added %08x\n", op);
}

void foo() {
    printf("foo was called.\n");
}

void segv(int signal) {
    printf("I (%d) been naughty.\n", getpid());
    pause();
}

int main(void) {
    void *fun = mmap (NULL, 1024*4, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    void (*addr)(void) = fun;

    int a = 23;
    int b = 0;
    unsigned INT32 registers = 0;
    int i;
    
    for (i = 3; i <= 11; i++) {
        registers |= RBIT(i);
    }

    if (fun == MAP_FAILED) {
        fprintf(stderr, "MMAP FAIL!\n");
        return 1;
    }
    signal(SIGSEGV, segv);

    to = fun;

    printf("fun: %p\n", fun);
    ADD_PROLOGUE(registers);
    arm32_call(foo);
    ADD_SET_REG(ARM_REG_R4, (INT32)&a, ARM_REG_R5);
    add_to_program(ldr_reg_imm(ARM_REG_R3, ARM_REG_R4, 0));
    ADD_SET_REG(ARM_REG_R4, (INT32)&b, ARM_REG_R5);
    add_to_program(str_reg_imm(ARM_REG_R3, ARM_REG_R4, 0));
    ADD_EPILOGUE(registers);

    mprotect(fun, 1024*4, PROT_READ | PROT_EXEC);
    __builtin___clear_cache(fun, (char*)fun+1024*4);

    addr();

    printf("%d %d\n", a, b);

    munmap(fun, 1024*4);

    return 0;
}

void bar() {
}

#endif
