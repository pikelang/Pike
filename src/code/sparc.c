/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sparc.c,v 1.45 2005/06/23 16:30:02 grubba Exp $
*/

/*
 * Machine code generator for sparc.
 *
 * Henrik Grubbström 20010720
 */

#include "global.h"
#include "svalue.h"
#include "operators.h"
#include "object.h"
#include "builtin_functions.h"

/*
 * Register definitions
 */

#define SPARC_REG_G0	0
#define SPARC_REG_G1	1
#define SPARC_REG_G2	2
#define SPARC_REG_G3	3
#define SPARC_REG_G4	4
#define SPARC_REG_G5	5
#define SPARC_REG_G6	6
#define SPARC_REG_G7	7
#define SPARC_REG_O0	8
#define SPARC_REG_O1	9
#define SPARC_REG_O2	10
#define SPARC_REG_O3	11
#define SPARC_REG_O4	12
#define SPARC_REG_O5	13
#define SPARC_REG_O6	14	/* SP */
#define SPARC_REG_O7	15
#define SPARC_REG_L0	16
#define SPARC_REG_L1	17
#define SPARC_REG_L2	18
#define SPARC_REG_L3	19
#define SPARC_REG_L4	20
#define SPARC_REG_L5	21
#define SPARC_REG_L6	22
#define SPARC_REG_L7	23
#define SPARC_REG_I0	24
#define SPARC_REG_I1	25
#define SPARC_REG_I2	26
#define SPARC_REG_I3	27
#define SPARC_REG_I4	28
#define SPARC_REG_I5	29
#define SPARC_REG_I6	30	/* FP */
#define SPARC_REG_I7	31	/* PC */

/*
 * ALU operations.
 */

#define SPARC_OP3_AND		0x01
#define SPARC_OP3_ANDcc		0x11
#define SPARC_OP3_ANDN		0x05
#define SPARC_OP3_ANDNcc	0x15
#define SPARC_OP3_OR		0x02
#define SPARC_OP3_ORcc		0x12
#define SPARC_OP3_ORN		0x06
#define SPARC_OP3_ORNcc		0x16
#define SPARC_OP3_XOR		0x03
#define SPARC_OP3_XORcc		0x13
#define SPARC_OP3_XNOR		0x07
#define SPARC_OP3_XNORcc	0x17
#define SPARC_OP3_ADD		0x00
#define SPARC_OP3_ADDcc		0x10
#define SPARC_OP3_ADDC		0x08
#define SPARC_OP3_ADDCcc	0x18
#define SPARC_OP3_SUB		0x04
#define SPARC_OP3_SUBcc		0x14
#define SPARC_OP3_SUBC		0x0c
#define SPARC_OP3_SUBCcc	0x1c
#define SPARC_OP3_SLL		0x25
#define SPARC_OP3_SRL		0x26
#define SPARC_OP3_SRA		0x27
#define SPARC_OP3_RD		0x28
#define SPARC_OP3_JMPL		0x38
#define SPARC_OP3_RETURN	0x39
#define SPARC_OP3_SAVE		0x3c
#define SPARC_OP3_RESTORE	0x3d

#define SPARC_RD_REG_CCR	0x02
#define SPARC_RD_REG_PC		0x05

#define SPARC_ALU_OP(OP3, D, S1, S2, I)	\
    add_to_program(0x80000000|((D)<<25)|((OP3)<<19)|((S1)<<14)|((I)<<13)| \
		   ((S2)&0x1fff))

#define SPARC_OP3_LDUW		0x00
#define SPARC_OP3_LDUH		0x02
#define SPARC_OP3_LDUB		0x01
#define SPARC_OP3_LDSW		0x08
#define SPARC_OP3_LDSH		0x0a
#define SPARC_OP3_LDSB		0x08
#define SPARC_OP3_LDX		0x0b
#define SPARC_OP3_STW		0x04
#define SPARC_OP3_STH		0x06
#define SPARC_OP3_STX		0x0e

#define SPARC_MEM_OP(OP3, D, S1, S2, I) \
    add_to_program(0xc0000000|((D)<<25)|((OP3)<<19)|((S1)<<14)|((I)<<13)| \
		   ((S2)&0x1fff))

#define SPARC_OR(D,S1,S2,I)	SPARC_ALU_OP(SPARC_OP3_OR, D, S1, S2, I)

#define SPARC_SRA(D,S1,S2,I)	SPARC_ALU_OP(SPARC_OP3_SRA, D, S1, S2, I)
#define SPARC_SLL(D,S1,S2,I)	SPARC_ALU_OP(SPARC_OP3_SLL, D, S1, S2, I)

#define SPARC_ADD(D,S1,S2,I)	SPARC_ALU_OP(SPARC_OP3_ADD, D, S1, S2, I)
#define SPARC_SUBcc(D,S1,S2,I)	SPARC_ALU_OP(SPARC_OP3_SUBcc, D, S1, S2, I)

#define SPARC_RD(D, RDREG)	SPARC_ALU_OP(SPARC_OP3_RD, D, RDREG, 0, 0)

#define SPARC_RETURN(S1, S2, I)	SPARC_ALU_OP(SPARC_OP3_RETURN, 0, S1, S2, I)

#define SPARC_JMPL(D,S1,S2,I)	SPARC_ALU_OP(SPARC_OP3_JMPL, D, S1, S2, I)
#define SPARC_RET()		SPARC_JMPL(SPARC_REG_G0, SPARC_REG_I7, 8, 1)
#define SPARC_RESTORE(D,S1,S2,I) SPARC_ALU_OP(SPARC_OP3_RESTORE, D, S1, S2, I)

#define SPARC_LDX(D,S1,S2,I)	SPARC_MEM_OP(SPARC_OP3_LDX, D, S1, S2, I)
#define SPARC_LDUW(D,S1,S2,I)	SPARC_MEM_OP(SPARC_OP3_LDUW, D, S1, S2, I)
#define SPARC_LDUH(D,S1,S2,I)	SPARC_MEM_OP(SPARC_OP3_LDUH, D, S1, S2, I)

#define SPARC_STX(D,S1,S2,I)	SPARC_MEM_OP(SPARC_OP3_STX, D, S1, S2, I)
#define SPARC_STW(D,S1,S2,I)	SPARC_MEM_OP(SPARC_OP3_STW, D, S1, S2, I)
#define SPARC_STH(D,S1,S2,I)	SPARC_MEM_OP(SPARC_OP3_STH, D, S1, S2, I)

#define SPARC_SETHI(D, VAL) \
    add_to_program(0x01000000|((D)<<25)|(((VAL)>>10)&0x3fffff))

#define SPARC_BE(DISP22, A) \
    add_to_program(0x02800000|((A)<<29)|(((DISP22)>>2)&0x1fffff))

#define SPARC_BNE(DISP22, A) \
    add_to_program(0x12800000|((A)<<29)|(((DISP22)>>2)&0x1fffff))

#define SPARC_CALL(DISP30) \
    add_to_program(0x40000000 | (((DISP30) >> 2) & 0x3fffffff))


#define SET_REG(REG, X) do {						\
    INT64 val_ = X;							\
    INT32 reg_ = REG;							\
    /*fprintf(stderr, "SET_REG(0x%02x, %p)\n", reg_, (void *)val_);*/	\
    if ((-4096 <= val_) && (val_ <= 4095)) {				\
      /* or %g0, val_, reg */						\
      SPARC_OR(reg_, SPARC_REG_G0, val_, 1);				\
    } else if ((-0x80000000LL <= val_) && (val_ <= 0x7fffffffLL)) {	\
      /* sethi %hi(val_), reg */					\
      SPARC_SETHI(reg_, val_);						\
      if (val_ & 0x3ff) {						\
	/* or reg, %lo(val_), reg */					\
        SPARC_OR(reg_, reg_, val_ & 0x3ff, 1);				\
      }									\
      if (val_ < 0) {							\
        /* Sign extend. */						\
	/* sra reg, %g0, reg */						\
        SPARC_SRA(reg_, reg_, SPARC_REG_G0, 0);				\
      }									\
    } else {								\
      /* FIXME: SPARC64 */						\
      if (!(val_>>34)) {						\
	/* The top 30 bits are zero. */					\
	SPARC_SETHI(reg_, val_>>2);					\
	SPARC_SLL(reg_, reg_, 2, 1);					\
	if (val_ & 0xfff) {						\
	  SPARC_OR(reg_, reg_, val_ & 0xfff, 1);			\
	}								\
      }	else {								\
	Pike_fatal("Value out of range: %p\n", (void *)val_);		\
      }									\
    }									\
  } while(0)

      /* FIXME: SPARC64 */
#define ADD_CALL(X, DELAY_OK) do {					\
    PIKE_OPCODE_T *ptr_ = (PIKE_OPCODE_T *)(X);				\
    ptrdiff_t delta_;							\
    struct program *p_ = Pike_compiler->new_program;			\
    INT32 off_ = p_->num_program;					\
    /* noop		*/						\
    INT32 delay_ = 0x01000000;						\
									\
    if (DELAY_OK) {							\
      /* Move the previous opcode to the delay-slot. */			\
      delay_ = p_->program[--off_];					\
    } else {								\
      add_to_program(0); /* Placeholder... */				\
    }									\
    /*fprintf(stderr, "call %p (pc:%p)\n", ptr_, p_->program);*/	\
    /* call X	*/							\
    delta_ = ptr_ - (p_->program + off_);				\
    if ((-0x20000000L <= delta_) && (delta_ <= 0x1fffffff)) {		\
      p_->program[off_] = 0x40000000 | (delta_ & 0x3fffffff);		\
      add_to_relocations(off_);						\
    } else {								\
      /* NOTE: Assumes top 30 bits are zero! */				\
      /* sethi %hi(ptr_>>2), %o7 */					\
      p_->program[off_] =						\
	0x01000000|(SPARC_REG_O7<<25)|((((size_t)ptr_)>>12)&0x3fffff);	\
      /* sll %o7, 2, %o7 */						\
      SPARC_SLL(SPARC_REG_O7, SPARC_REG_O7, 2, 1);			\
      /* call %o7 + %lo(ptr_) */					\
      SPARC_JMPL(SPARC_REG_O7, SPARC_REG_O7, ((size_t)ptr_)&0xfff, 1);	\
    }									\
    add_to_program(delay_);						\
    sparc_last_pc = off_;	/* Value in %o7. */			\
    sparc_codegen_state |= SPARC_CODEGEN_PC_IS_SET;			\
  } while(0)

/*
 * Register conventions:
 *
 * I0	Scratch
 *
 * I6	Frame pointer
 * I7	Return address
 *
 * L0	&Pike_interpreter
 * L1	Pike_fp
 * L7	&d_flag
 *
 * O6	Stack Pointer
 * O7	Program Counter
 *
 */

#define SPARC_REG_PIKE_IP	SPARC_REG_L0
#define SPARC_REG_PIKE_FP	SPARC_REG_L1
#define SPARC_REG_PIKE_SP	SPARC_REG_L2
#define SPARC_REG_PIKE_MARK_SP	SPARC_REG_L3
#define SPARC_REG_PIKE_OBJ	SPARC_REG_L4
#define SPARC_REG_PIKE_DEBUG	SPARC_REG_L7
#define SPARC_REG_SP		SPARC_REG_O6
#define SPARC_REG_PC		SPARC_REG_O7

/*
 * Code generator state.
 */
unsigned INT32 sparc_codegen_state = 0;
static int sparc_pike_sp_bias = 0;
ptrdiff_t sparc_last_pc = 0;

#ifdef PIKE_BYTECODE_SPARC64
#define PIKE_LDPTR	SPARC_LDX
#define PIKE_STPTR	SPARC_STX
#else /* !PIKE_BYTECODE_SPARC64 */
#define PIKE_LDPTR	SPARC_LDUW
#define PIKE_STPTR	SPARC_STW
#endif /* PIKE_BYTECODE_SPARC64 */

#define LOAD_PIKE_INTERPRETER() do {				\
    if (!(sparc_codegen_state & SPARC_CODEGEN_IP_IS_SET)) {	\
      SET_REG(SPARC_REG_PIKE_IP,				\
	      ((ptrdiff_t)(&Pike_interpreter)));		\
      sparc_codegen_state |= SPARC_CODEGEN_IP_IS_SET;		\
    }								\
  } while(0)

#define LOAD_PIKE_FP() do {						\
    if (!(sparc_codegen_state & SPARC_CODEGEN_FP_IS_SET)) {		\
      LOAD_PIKE_INTERPRETER();						\
      /* lduw [ %ip, %offset(Pike_interpreter, frame_pointer) ], %l1 */	\
      PIKE_LDPTR(SPARC_REG_PIKE_FP, SPARC_REG_PIKE_IP,			\
		 OFFSETOF(Pike_interpreter, frame_pointer), 1);		\
      sparc_codegen_state |= SPARC_CODEGEN_FP_IS_SET;			\
    }									\
  } while(0)

#define LOAD_PIKE_SP()	do {						\
    if (!(sparc_codegen_state & SPARC_CODEGEN_SP_IS_SET)) {		\
      LOAD_PIKE_INTERPRETER();						\
      /* lduw [ %ip, %offset(Pike_interpreter, stack_pointer) ], %l2 */	\
      PIKE_LDPTR(SPARC_REG_PIKE_SP, SPARC_REG_PIKE_IP,			\
		 OFFSETOF(Pike_interpreter, stack_pointer), 1);		\
      sparc_codegen_state |= SPARC_CODEGEN_SP_IS_SET;			\
      sparc_pike_sp_bias = 0;						\
    } else if (sparc_pike_sp_bias > 0xf00) {				\
      /* Make sure there's always space for at least 256 bytes. */	\
      SPARC_ADD(SPARC_REG_PIKE_SP, SPARC_REG_PIKE_SP,			\
		sparc_pike_sp_bias, 1);					\
      sparc_pike_sp_bias = 0;						\
      sparc_codegen_state |= SPARC_CODEGEN_SP_NEEDS_STORE;		\
    }									\
  } while(0)

#define LOAD_PIKE_MARK_SP()	do {						\
    if (!(sparc_codegen_state & SPARC_CODEGEN_MARK_SP_IS_SET)) {	\
      LOAD_PIKE_INTERPRETER();						\
      /* lduw [ %ip, %offset(Pike_interpreter, mark_stack_pointer) ], %l2 */	\
      PIKE_LDPTR(SPARC_REG_PIKE_MARK_SP, SPARC_REG_PIKE_IP,		\
		 OFFSETOF(Pike_interpreter, mark_stack_pointer), 1);	\
      sparc_codegen_state |= SPARC_CODEGEN_MARK_SP_IS_SET;		\
    }									\
  } while(0)

#define SPARC_FLUSH_UNSTORED()	do {					\
    if (sparc_pike_sp_bias) {						\
      SPARC_ADD(SPARC_REG_PIKE_SP, SPARC_REG_PIKE_SP,			\
		sparc_pike_sp_bias, 1);					\
      sparc_pike_sp_bias = 0;						\
      sparc_codegen_state |= SPARC_CODEGEN_SP_NEEDS_STORE;		\
    }									\
    if (sparc_codegen_state & SPARC_CODEGEN_MARK_SP_NEEDS_STORE) {	\
      /* stw %pike_mark_sp, [ %ip, %offset(Pike_interpreter, mark_stack_pointer) ] */	\
      PIKE_STPTR(SPARC_REG_PIKE_MARK_SP, SPARC_REG_PIKE_IP,		\
		 OFFSETOF(Pike_interpreter, mark_stack_pointer), 1);	\
      sparc_codegen_state &= ~SPARC_CODEGEN_MARK_SP_NEEDS_STORE;	\
    }									\
    if (sparc_codegen_state & SPARC_CODEGEN_SP_NEEDS_STORE) {		\
      /* stw %pike_sp, [ %ip, %offset(Pike_interpreter, stack_pointer) ] */	\
      PIKE_STPTR(SPARC_REG_PIKE_SP, SPARC_REG_PIKE_IP,			\
		 OFFSETOF(Pike_interpreter, stack_pointer), 1);		\
      sparc_codegen_state &= ~SPARC_CODEGEN_SP_NEEDS_STORE;		\
    }									\
  } while(0)

#define SPARC_UNLOAD_CACHED()					\
  (sparc_codegen_state &= ~(SPARC_CODEGEN_FP_IS_SET|		\
			    SPARC_CODEGEN_SP_IS_SET|		\
			    SPARC_CODEGEN_MARK_SP_IS_SET))

void sparc_flush_codegen_state(void)
{
  SPARC_FLUSH_UNSTORED();
  sparc_codegen_state = 0;
}

#define ADJUST_PIKE_PC(NEW_PC)	do {			\
    sparc_last_pc = NEW_PC;				\
    sparc_codegen_state |= SPARC_CODEGEN_PC_IS_SET;	\
  } while(0)

#if 0
static void sparc_trace_fun(PIKE_OPCODE_T *pc)
{
  struct program *prog = NULL;
  if (Pike_fp->current_object && (prog = Pike_fp->current_object->prog)) {
    struct pike_string *fname;
    INT32 lineno;
    if (fname = low_get_line(pc + 4, prog, &lineno)) {
      fprintf(stderr, "TRACE: %p: %s:%d\n",
	      pc, fname->str, lineno);
      free_string(fname);
      return;
    }
  }
  fprintf(stderr, "TRACE: %p: %p, %p\n",
	  pc, Pike_fp->current_object, prog);
}
#endif /* 0 */

/*
 * Allocate a stack frame.
 *
 * Note that the prologue size must be constant.
 */
void sparc_ins_entry(void)
{
#ifdef PIKE_BYTECODE_SPARC64
  /* save	%sp, -224, %sp */
  add_to_program(0x81e02000|(SPARC_REG_SP<<25)|
		 (SPARC_REG_SP<<14)|((-176)&0x1fff));
#else /* !PIKE_BYTECODE_SPARC64 */
  /* save	%sp, -112, %sp */
  add_to_program(0x81e02000|(SPARC_REG_SP<<25)|
		 (SPARC_REG_SP<<14)|((-112)&0x1fff));
#endif /* PIKE_BYTECODE_SPARC64 */
#if 0
  SPARC_OR(SPARC_REG_O0, SPARC_REG_G0, SPARC_REG_O7, 0);
  ADD_CALL(sparc_trace_fun, 1);
#endif /* 0 */
  FLUSH_CODE_GENERATOR_STATE();
}

/* Update Pike_fp->pc */
void sparc_update_pc(void)
{
  LOAD_PIKE_FP();
#ifdef PIKE_BYTECODE_SPARC64
  /* The ASR registers are implementation specific in Sparc V7 and V8. */
  /* rd %pc, %o7 */
  SPARC_RD(SPARC_REG_O7, SPARC_RD_REG_PC);
#else /* !0 */
  /* call .+8 */
  SPARC_CALL(8);
  /* The new %o7 is available in the delay slot. */
#endif /* 0 */
  /* stw %o7, [ %pike_fp + pc ] */
  PIKE_STPTR(SPARC_REG_O7, SPARC_REG_PIKE_FP, OFFSETOF(pike_frame, pc), 1);
}

/*
 * Opcode implementations.
 */

#define MAKE_TYPE_WORD(TYPE, SUB_TYPE)	(((TYPE) << 16)|(SUB_TYPE))

static void sparc_incr_mark_sp(int delta)
{
  LOAD_PIKE_MARK_SP();
  /* add %pike_mark_sp, %pike_mark_sp, 4 * delta */
  SPARC_ADD(SPARC_REG_PIKE_MARK_SP, SPARC_REG_PIKE_MARK_SP,
	    sizeof(void *)*delta, 1);
  sparc_codegen_state |= SPARC_CODEGEN_MARK_SP_NEEDS_STORE;
}    

static void sparc_mark(ptrdiff_t off)
{
  LOAD_PIKE_SP();
  LOAD_PIKE_MARK_SP();
  off *= sizeof(struct svalue);
  off += sparc_pike_sp_bias;
  if (off) {
    if ((-4096 <= off) && (off < 4096)) {
      /* add %i0, %pike_sp, off */
      SPARC_ADD(SPARC_REG_I0, SPARC_REG_PIKE_SP, off, 1);
    } else {
      SET_REG(SPARC_REG_I0, off);
      /* add %i0, %pike_sp, %i0 */
      SPARC_ADD(SPARC_REG_I0, SPARC_REG_PIKE_SP, SPARC_REG_I0, 0);
    }
    /* stw %i0, [ %pike_mark_sp, %g0 ] */
    PIKE_STPTR(SPARC_REG_I0, SPARC_REG_PIKE_MARK_SP, SPARC_REG_G0, 0);
  } else {
    /* stw %pike_sp, [ %pike_mark_sp, %g0 ] */
    PIKE_STPTR(SPARC_REG_PIKE_SP, SPARC_REG_PIKE_MARK_SP, SPARC_REG_G0, 0);
  }
  sparc_incr_mark_sp(1);
}

static void sparc_push_int(INT_TYPE x, int sub_type)
{
  INT32 type_word = MAKE_TYPE_WORD(PIKE_T_INT, sub_type);
  int reg = SPARC_REG_G0;

  LOAD_PIKE_SP();

#ifdef PIKE_DEBUG
  if (sizeof(struct svalue) > 8) {
    size_t e;
    for (e = 4; e < sizeof(struct svalue); e += 4) {
      /* Pad until we reach the anything field. */
      if (e == OFFSETOF(svalue, u.integer)) continue;
      /* stw %g0, [ %pike_sp, e ] */
      SPARC_STW(SPARC_REG_G0, SPARC_REG_PIKE_SP, sparc_pike_sp_bias + e, 1);
    }
  }
#endif /* PIKE_DEBUG */
  if (x) {
    SET_REG(SPARC_REG_I1, x);
    reg = SPARC_REG_I1;
  }
  if (sizeof(INT_TYPE) == 4) {
    SPARC_STW(reg, SPARC_REG_PIKE_SP,
	      sparc_pike_sp_bias + OFFSETOF(svalue, u.integer), 1);
  } else {
    SPARC_STX(reg, SPARC_REG_PIKE_SP,
	      sparc_pike_sp_bias + OFFSETOF(svalue, u.integer), 1);
  }
  if (x != type_word) {
    SET_REG(SPARC_REG_I1, type_word);
  }
  /* This is safe since type_word is never zero. */
  /* stw %i1, [ %pike_sp ] */
  SPARC_STW(SPARC_REG_I1, SPARC_REG_PIKE_SP, sparc_pike_sp_bias, 1);
  sparc_pike_sp_bias += sizeof(struct svalue);
  sparc_codegen_state |= SPARC_CODEGEN_SP_NEEDS_STORE;
}

static void sparc_clear_string_subtype(void)
{
  LOAD_PIKE_SP();
  /* lduh [ %pike_sp, %g0 ], %i0 */
  SPARC_LDUH(SPARC_REG_I0, SPARC_REG_PIKE_SP,
	     sparc_pike_sp_bias + OFFSETOF(svalue, type), 1);
  /* subcc %g0, %i0, 8 */
  SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_I0, PIKE_T_INT, 1);
  /* be,a .+8 */
  SPARC_BE(8, 1);
  /* sth %g0, [ %pike_sp, 2 ] */
  SPARC_STH(SPARC_REG_G0, SPARC_REG_PIKE_SP,
	    sparc_pike_sp_bias + OFFSETOF(svalue, subtype), 1);
}

static void sparc_push_lfun(unsigned int no)
{
  LOAD_PIKE_FP();
  LOAD_PIKE_SP();
  /* lduw [ %pike_fp, %offset(pike_frame, current_object) ], %pike_obj */
  PIKE_LDPTR(SPARC_REG_PIKE_OBJ, SPARC_REG_PIKE_FP,
	     OFFSETOF(pike_frame, current_object), 1);
  /* stw %pike_obj, [ %pike_sp, %offset(svalue, u.object) ] */
  PIKE_STPTR(SPARC_REG_PIKE_OBJ, SPARC_REG_PIKE_SP,
	     sparc_pike_sp_bias + OFFSETOF(svalue, u.object), 1);
  /* lduw [ %pike_obj, %offset(object, refs) ], %i0 */
  SPARC_LDUW(SPARC_REG_I0, SPARC_REG_PIKE_OBJ,
	     OFFSETOF(object, refs), 1);
  /* add %i0, 1, %i0 */
  SPARC_ADD(SPARC_REG_I0, SPARC_REG_I0, 1, 1);
  /* stw %i0, [ %pike_obj, %offset(object, refs) ] */
  SPARC_STW(SPARC_REG_I0, SPARC_REG_PIKE_OBJ,
	    OFFSETOF(object, refs), 1);
  /* lduh [ %pike_fp, %offset(pike_frame, context.identifier_level ], %i1 */
  SPARC_LDUH(SPARC_REG_I1, SPARC_REG_PIKE_FP,
	     OFFSETOF(pike_frame, context.identifier_level), 1);
  SET_REG(SPARC_REG_I2, (no & 0xffff) | (PIKE_T_FUNCTION << 16));
  /* add %i1, %i2, %i1 */
  SPARC_ADD(SPARC_REG_I1, SPARC_REG_I1, SPARC_REG_I2, 0);
  /* stw %i1, [ %pike_sp, %g0 ] */
  SPARC_STW(SPARC_REG_I1, SPARC_REG_PIKE_SP, sparc_pike_sp_bias, 1);
  sparc_pike_sp_bias += sizeof(struct svalue);
  sparc_codegen_state |= SPARC_CODEGEN_SP_NEEDS_STORE;
}

void sparc_local_lvalue(unsigned int no)
{
  LOAD_PIKE_SP();
  LOAD_PIKE_FP();
  SET_REG(SPARC_REG_I0, T_SVALUE_PTR);
  /* sth %i0, [ %pike_sp, %g0 ] */
  SPARC_STH(SPARC_REG_I0, SPARC_REG_PIKE_SP, sparc_pike_sp_bias, 1);
  SET_REG(SPARC_REG_I0, T_VOID);
  no *= sizeof(struct svalue);
  if (no < 4096) {
    /* lduw [ %pike_fp, %offset(pike_frame, locals) ], %i2 */
    PIKE_LDPTR(SPARC_REG_I2, SPARC_REG_PIKE_FP,
	       OFFSETOF(pike_frame, locals), 1);
    /* add %i2, no * sizeof(struct svalue), %i2 */
    SPARC_ADD(SPARC_REG_I2, SPARC_REG_I2, no, 1);
  } else {
    SET_REG(SPARC_REG_I1, no);
    /* lduw [ %pike_fp, %offset(pike_frame, locals) ], %i2 */
    PIKE_LDPTR(SPARC_REG_I2, SPARC_REG_PIKE_FP,
	       OFFSETOF(pike_frame, locals), 1);
    /* add %i2, %i1, %i2 */
    SPARC_ADD(SPARC_REG_I2, SPARC_REG_I2, SPARC_REG_I1, 0);
  }
  /* stw %i2, [ %pike_sp, %offset(svalue, u.lval) ] */
  PIKE_STPTR(SPARC_REG_I2, SPARC_REG_PIKE_SP,
	     sparc_pike_sp_bias + OFFSETOF(svalue, u.lval), 1);
  sparc_pike_sp_bias += sizeof(struct svalue) * 2;
  /* sth %i0, [ %pike_sp , -sizeof(struct svalue) ] */
  SPARC_STH(SPARC_REG_I0, SPARC_REG_PIKE_SP,
	    sparc_pike_sp_bias - sizeof(struct svalue), 1);
  sparc_codegen_state |= SPARC_CODEGEN_SP_NEEDS_STORE;
}

void sparc_escape_catch(void)
{
  LOAD_PIKE_FP();
  SPARC_FLUSH_UNSTORED();
#ifdef PIKE_BYTECODE_SPARC64
  /* The asr registers are implementation specific in Sparc V7 and V8. */
  /* rd %pc, %i0 */
  SPARC_RD(SPARC_REG_I0, SPARC_RD_REG_PC);
  /* add %i0, 20, %i0 */
  SPARC_ADD(SPARC_REG_I0, SPARC_REG_I0, 5*4, 1);
#else /* !0 */
  /* call .+8 */
  SPARC_CALL(8);
  /* The new %o7 is available in the delay slot. */
  /* add %o7, 24, %i0 */
  SPARC_ADD(SPARC_REG_I0, SPARC_REG_O7, 6*4, 1);
#endif /* 0 */
  /* stw %i0, [ %pike_fp, %offset(pike_frame, return_addr) ] */
  PIKE_STPTR(SPARC_REG_I0, SPARC_REG_PIKE_FP,
	     OFFSETOF(pike_frame, return_addr), 1);
#ifdef PIKE_BYTECODE_SPARC64
  /* The following code is Sparc V9 only code. */
  /* return %i7 + 8 */
  SPARC_RETURN(SPARC_REG_I7, 8, 1);
  /* or %g0, -2, %o0 */
  SPARC_OR(SPARC_REG_O0, SPARC_REG_G0, -2, 1);
#else /* ! 0 */
  /* Sparc V7 & V8 code. */
  /* or %g0, -2, %i0 */
  SPARC_OR(SPARC_REG_I0, SPARC_REG_G0, -2, 1);
  /* ret */
  SPARC_RET();
  /* restore */
  SPARC_RESTORE(SPARC_REG_G0, SPARC_REG_G0, SPARC_REG_G0, 0);
#endif /* 0 */
  SPARC_UNLOAD_CACHED();
}

/*
 *
 */

#ifdef PIKE_DEBUG
void sparc_debug_check_registers(int state,
				 struct Pike_interpreter *cached_ip,
				 struct pike_frame *cached_fp,
				 struct svalue *cached_sp,
				 struct svalue **cached_mark_sp)
{
  if (((state & SPARC_CODEGEN_IP_IS_SET) &&
       (cached_ip != &Pike_interpreter)) ||
      ((state & SPARC_CODEGEN_FP_IS_SET) &&
       (cached_fp != Pike_interpreter.frame_pointer)) ||
      ((state & SPARC_CODEGEN_SP_IS_SET) &&
       (cached_sp != Pike_interpreter.stack_pointer)) ||
      ((state & SPARC_CODEGEN_MARK_SP_IS_SET) &&
       (cached_mark_sp != Pike_interpreter.mark_stack_pointer))) {
    Pike_fatal("Bad machine code cache key (0x%04x):\n"
	       "Cached: ip:0x%08x, fp:0x%08x, sp:0x%08x, m_sp:0x%08x\n"
	       "  Real: ip:0x%08x, fp:0x%08x, sp:0x%08x, m_sp:0x%08x\n",
	       state,
	       (INT32)cached_ip, (INT32)cached_fp,
	       (INT32)cached_sp, (INT32)cached_mark_sp,
	       (INT32)&Pike_interpreter, (INT32)Pike_interpreter.frame_pointer,
	       (INT32)Pike_interpreter.stack_pointer,
	       (INT32)Pike_interpreter.mark_stack_pointer);
  }
}

static void ins_sparc_debug()
{
  int state = sparc_codegen_state;
  if (state & SPARC_CODEGEN_SP_NEEDS_STORE) {
    state &= ~SPARC_CODEGEN_SP_IS_SET;
  }
  if (state & SPARC_CODEGEN_MARK_SP_NEEDS_STORE) {
    state &= ~SPARC_CODEGEN_MARK_SP_IS_SET;
  }
  if (state &
      (SPARC_CODEGEN_FP_IS_SET|SPARC_CODEGEN_SP_IS_SET|
       SPARC_CODEGEN_IP_IS_SET|SPARC_CODEGEN_MARK_SP_IS_SET)) {
    SET_REG(SPARC_REG_PIKE_DEBUG, ((ptrdiff_t)(&d_flag)));
    SPARC_LDUW(SPARC_REG_PIKE_DEBUG, SPARC_REG_PIKE_DEBUG, SPARC_REG_G0, 0);
    SPARC_SUBcc(SPARC_REG_G0, SPARC_REG_PIKE_DEBUG, SPARC_REG_G0, 0);
    SET_REG(SPARC_REG_O0, state);
#ifdef PIKE_BYTECODE_SPARC64
    SPARC_BE(8*4, 0);
    SPARC_OR(SPARC_REG_O1, SPARC_REG_PIKE_IP, SPARC_REG_G0, 0);
    SPARC_SETHI(SPARC_REG_O7, ((ptrdiff_t)sparc_debug_check_registers)>>2);
    SPARC_OR(SPARC_REG_O2, SPARC_REG_PIKE_FP, SPARC_REG_G0, 0);
    SPARC_SLL(SPARC_REG_O7, SPARC_REG_O7, 2, 1);
    SPARC_ADD(SPARC_REG_O3, SPARC_REG_PIKE_SP, sparc_pike_sp_bias, 1);
    SPARC_JMPL(SPARC_REG_O7, SPARC_REG_O7,
	       ((ptrdiff_t)sparc_debug_check_registers) & 0xfff, 1);
    SPARC_OR(SPARC_REG_O4, SPARC_REG_PIKE_MARK_SP, SPARC_REG_G0, 0);
#else /* !PIKE_BYTECODE_SPARC64 */
    /* NOTE: Assumes ADD_CALL below is a single instruction. */
    SPARC_BE(6*4, 0);
    SPARC_OR(SPARC_REG_O1, SPARC_REG_PIKE_IP, SPARC_REG_G0, 0);
    SPARC_OR(SPARC_REG_O2, SPARC_REG_PIKE_FP, SPARC_REG_G0, 0);
    SPARC_ADD(SPARC_REG_O3, SPARC_REG_PIKE_SP, sparc_pike_sp_bias, 1);
    SPARC_OR(SPARC_REG_O4, SPARC_REG_PIKE_MARK_SP, SPARC_REG_G0, 0);
    ADD_CALL(sparc_debug_check_registers, 1);
#endif /* PIKE_BYTECODE_SPARC64 */
  }
}
#else /* !PIKE_DEBUG */
#define ins_sparc_debug()
#endif /* PIKE_DEBUG */

static void low_ins_call(void *addr, int delay_ok, int i_flags)
{
  SPARC_FLUSH_UNSTORED();

  {
    static int last_prog_id=-1;
    static size_t last_num_linenumbers=(size_t)~0;
    if(last_prog_id != Pike_compiler->new_program->id ||
       last_num_linenumbers != Pike_compiler->new_program->num_linenumbers)
    {
      last_prog_id=Pike_compiler->new_program->id;
      last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;

      LOAD_PIKE_FP();

      /* Note: We fill the delay slot with the following opcode.
       *       This works since the new %o7 is available immediately.
       *       (Sparc Architecture Manual V9 p149.)
       */

      /* stw %o7, [ %pike_fp, %offsetof(pike_frame, pc) ] */
      PIKE_STPTR(SPARC_REG_O7, SPARC_REG_PIKE_FP, OFFSETOF(pike_frame, pc), 1);

      delay_ok = 1;
    }
  }

  ADD_CALL(addr, delay_ok);

#if 1
  /* Invalidate cached values if needed. */
  if (i_flags & (I_UPDATE_SP|I_UPDATE_M_SP|I_UPDATE_FP)) {
    if (i_flags & I_UPDATE_SP) {
      sparc_codegen_state &= ~SPARC_CODEGEN_SP_IS_SET;
    }
    if (i_flags & I_UPDATE_M_SP) {
      sparc_codegen_state &= ~SPARC_CODEGEN_MARK_SP_IS_SET;
    }
    if (i_flags & I_UPDATE_FP) {
      sparc_codegen_state &= ~SPARC_CODEGEN_FP_IS_SET;
    }
  }
#else /* !1 */
  /* This is probably only needed for some instructions, but... */
  SPARC_UNLOAD_CACHED();
#endif /* 1 */
}

static void low_ins_f_byte(unsigned int b, int delay_ok)
{
  void *addr;

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
    
  addr = instrs[b].address;

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  /* This is not very pretty */
  switch(b)
  {
  case F_MARK2 - F_OFFSET:
    sparc_mark(0);
    /* FALL_THROUGH */
  case F_SYNCH_MARK - F_OFFSET:
  case F_MARK - F_OFFSET:
    sparc_mark(0);
    return;

  case F_POP_MARK - F_OFFSET:
    sparc_incr_mark_sp(-1);
    return;
  case F_UNDEFINED - F_OFFSET:
    sparc_push_int(0, 1);
    return;

  case F_MARK_AND_CONST0 - F_OFFSET:
    sparc_mark(0);
    /* FALL_THROUGH */
  case F_CONST0 - F_OFFSET:
    sparc_push_int(0, 0);
    return;

  case F_MARK_AND_CONST1 - F_OFFSET:
    sparc_mark(0);
    /* FALL_THROUGH */
  case F_CONST1 - F_OFFSET:
    sparc_push_int(1, 0);
    return;

  case F_CONST_1 - F_OFFSET:
    sparc_push_int(-1, 0);
    return;
  case F_BIGNUM - F_OFFSET:
    sparc_push_int(0x7fffffff, 0);
    return;

  case F_EXIT_CATCH - F_OFFSET:
    sparc_push_int(0, 1);
    /* FALL_THROUGH */
  case F_ESCAPE_CATCH - F_OFFSET:
    sparc_escape_catch();
    return;

  case F_MAKE_ITERATOR - F_OFFSET:
    {
      SET_REG(SPARC_REG_O0, 1);
      delay_ok = 1;
      addr = (void *)f_get_iterator;
    }
    break;

  case F_ADD - F_OFFSET:
    SET_REG(SPARC_REG_O0, 2);
    delay_ok = 1;
    addr = (void *)f_add;
    break;

    /* F_ZERO_TYPE? */
  }

  low_ins_call(addr, delay_ok, instrs[b].flags);
}

void ins_f_byte(unsigned int opcode)
{
  ins_sparc_debug();
  low_ins_f_byte(opcode, 0);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  ins_sparc_debug();

  switch(a) {
  case F_NUMBER:
    sparc_push_int(b, 0);
    return;
  case F_NEG_NUMBER:
    sparc_push_int(-(ptrdiff_t)b, 0);
    return;
  case F_LFUN:
    sparc_push_lfun(b);
    return;
  case F_MARK_X:
    sparc_mark(-(ptrdiff_t)b);
    return;
  case F_LOCAL_LVALUE:
    sparc_local_lvalue(b);
    return;
  }
  SET_REG(SPARC_REG_O0, b);
  low_ins_f_byte(a, 1);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
  ins_sparc_debug();

  SET_REG(SPARC_REG_O0, c);
  SET_REG(SPARC_REG_O1, b);
  low_ins_f_byte(a, 1);
  return;
}

#define addstr(s, l) low_my_binary_strcat((s), (l), buf)
#define adddata2(s,l) addstr((char *)(s),(l) * sizeof((s)[0]));

void sparc_encode_program(struct program *p, struct dynamic_buffer_s *buf)
{
  size_t prev = 0, rel;
  /* De-relocate the program... */
  for (rel = 0; rel < p->num_relocations; rel++) {
    size_t off = p->relocations[rel];
    INT32 opcode;
#ifdef PIKE_DEBUG
    if (off < prev) {
      Pike_fatal("Relocations in bad order!\n");
    }
#endif /* PIKE_DEBUG */
    adddata2(p->program + prev, off - prev);

#ifdef PIKE_DEBUG
    if ((p->program[off] & 0xc0000000) != 0x40000000) {
      Pike_fatal("Bad relocation!\n");
    }
#endif /* PIKE_DEBUG */
    /* Relocate to being relative to NULL */
    opcode = 0x40000000 |
      ((p->program[off] + (((INT32)(ptrdiff_t)(p->program)>>2))) & 0x3fffffff);
    adddata2(&opcode, 1);
    prev = off+1;
  }
  adddata2(p->program + prev, p->num_program - prev);
}

void sparc_decode_program(struct program *p)
{
  /* Relocate the program... */
  PIKE_OPCODE_T *prog = p->program;
  INT32 delta = ((INT32)(ptrdiff_t)p->program)>>2;
  size_t rel = p->num_relocations;
  while (rel--) {
#ifdef PIKE_DEBUG
    if ((prog[p->relocations[rel]] & 0xc0000000) != 0x40000000) {
      Pike_error("Bad relocation: %d, off:%d, opcode: 0x%08x\n",
		 rel, p->relocations[rel],
		 prog[p->relocations[rel]]);
    }
#endif /* PIKE_DEBUG */
    prog[p->relocations[rel]] = 0x40000000 |
      (((prog[p->relocations[rel]] & 0x3fffffff) - delta) &
       0x3fffffff);
  }
}

/*
 * Inline machine-code...
 */
const unsigned INT32 sparc_flush_instruction_cache[] = {
  /*
   * On entry:
   *	%o0 : void *ptr
   *	%o1 : size_t nbytes
   *
   *	cmp	%o1, #1
   * .l0:
   *	flush	%o0+%o1
   *	bge,a	.l0
   *	subcc	%o1, 8, %o1
   *	retl
   *	or	%g0,%g0,%o0
   */
  /* 1000 0000 1010 0000 0010 0000 0000 0000 */
  0x80a02000|(SPARC_REG_O1<<14)|1,
  /* 1000 0001 1101 1000 0000 0000 0000 0000 */
  0x81d80000|(SPARC_REG_O0<<14)|(SPARC_REG_O1),
  /* 0011 0110 1000 0000 0000 0000 0000 0000 */
  0x36800000|((-1)&0x3fffff),
  /* 1000 0000 1010 0000 0010 0000 0000 0000 */
  0x80a02000|(SPARC_REG_O1<<25)|(SPARC_REG_O1<<14)|8,
  /* 1000 0001 1100 0000 0010 0000 0000 0000 */
  0x81c02000|(SPARC_REG_O7<<14)|8,
  /* 1000 0000 0001 0000 0000 0000 0000 0000 */
  0x80100000|(SPARC_REG_O0<<25),
};

static void sparc_disass_rd_reg(int reg_no)
{
  switch(reg_no & 0x1f) {
  case SPARC_RD_REG_CCR:	fprintf(stderr, "%%ccr"); break;
  case SPARC_RD_REG_PC:		fprintf(stderr, "%%pc"); break;
  default:	fprintf(stderr, "%%sr(%d)", reg_no & 0x1f); break;
  }
}

static void sparc_disass_reg(int reg_no)
{
  fprintf(stderr, "%%%c%1x", "goli"[(reg_no>>3)&3], reg_no & 7);
}

void sparc_disassemble_code(void *addr, size_t bytes)
{
  unsigned INT32 *code = addr;
  size_t len = (bytes+3)>>2;

  while(len--) {
    unsigned INT32 opcode = *code;
    fprintf(stderr, "%p  %08x      ", code, opcode);
    switch(opcode & 0xc0000000) {
    case 0x00000000:
      {
	/* Sethi etc. */
	int op2 = (opcode >> 22) & 0x7;
	switch(op2) {
	case 4:
	  fprintf(stderr, "sethi %%hi(0x%08x), ", opcode << 10);
	  break;
	}
	sparc_disass_reg(opcode>>25);
	fprintf(stderr, "\n");
      }
      break;
    case 0x40000000:
      /* Call */
      fprintf(stderr, "call 0x%p\n", ((char *)code) + (opcode << 2));
      break;
    case 0x80000000:
      {
	/* ALU operation. */
	int op3 = (opcode >> 19) & 0x3f;
	char buf[16];
	char *mnemonic = NULL;
	if (!(op3 & 0x20)) {
	  switch(op3 & 0xf) {
	  case SPARC_OP3_ADD:	mnemonic = "add"; break;	/* 0 */
	  case SPARC_OP3_AND:	mnemonic = "and"; break;	/* 1 */
	  case SPARC_OP3_OR:	mnemonic = "or"; break;		/* 2 */
	  case SPARC_OP3_XOR:	mnemonic = "xor"; break;	/* 3 */
	  case SPARC_OP3_SUB:	mnemonic = "sub"; break;	/* 4 */
	  case SPARC_OP3_ANDN:	mnemonic = "andn"; break;	/* 5 */
	  case SPARC_OP3_ORN:	mnemonic = "orn"; break;	/* 6 */
	  case SPARC_OP3_XNOR:	mnemonic = "xnor"; break;	/* 7 */
	  case SPARC_OP3_ADDC:	mnemonic = "addc"; break;	/* 8 */

	  case SPARC_OP3_SUBC:	mnemonic = "subc"; break;	/* c */
	  default:
	    sprintf(buf, "op3(0x%02x)", op3 & 0xf);
	    mnemonic = buf;
	    break;
	  }
	  if (op3 & 0x10) {
	    fprintf(stderr, "%scc ", mnemonic);
	  } else {
	    fprintf(stderr, "%s ", mnemonic);
	  }
	} else {
	  switch(op3) {
	  case SPARC_OP3_SLL:	mnemonic = "sll"; break;
	  case SPARC_OP3_SRL:	mnemonic = "srl"; break;
	  case SPARC_OP3_SRA:	mnemonic = "sra"; break;
	  case SPARC_OP3_RD:	mnemonic = "rd"; break;
	  case SPARC_OP3_JMPL:  mnemonic = "jmpl"; break;
	  case SPARC_OP3_RETURN:mnemonic = "return"; break;
	  case SPARC_OP3_SAVE:	mnemonic = "save"; break;
	  case SPARC_OP3_RESTORE:mnemonic = "restore"; break;
	  default:
	    sprintf(buf, "op3(0x%02x)", op3);
	    mnemonic = buf;
	    break;
	  }
	  fprintf(stderr, "%s ", mnemonic);
	}
	if (op3 == SPARC_OP3_RD) {
	  sparc_disass_rd_reg(opcode>>14);
	  fprintf(stderr, ", ");
	} else {
	  sparc_disass_reg(opcode>>14);
	  fprintf(stderr, ", ");
	  if (opcode & 0x00002000) {
	    fprintf(stderr, "0x%04x, ", opcode & 0x1fff);
	  } else {
	    sparc_disass_reg(opcode);
	    fprintf(stderr, ", ");
	  }
	}
	sparc_disass_reg(opcode >> 25);
	fprintf(stderr, "\n");
      }
      break;
    case 0xc0000000:
      {
	/* Memory operations. */
	int op3 = (opcode >> 19) & 0x3f;
	char buf[16];
	char *mnemonic = NULL;
	switch(op3) {
	case 0x00:	mnemonic="lduw"; break;
	case 0x01:	mnemonic="ldub"; break;
	case 0x02:	mnemonic="lduh"; break;
	case 0x03:	mnemonic="ldd"; break;
	case 0x04:	mnemonic="stw"; break;
	case 0x05:	mnemonic="stb"; break;
	case 0x06:	mnemonic="sth"; break;
	case 0x07:	mnemonic="std"; break;
	case 0x08:	mnemonic="ldsw"; break;
	case 0x09:	mnemonic="ldsb"; break;
	case 0x0a:	mnemonic="ldsh"; break;
	case 0x0b:	mnemonic="ldx"; break;
	case 0x0e:	mnemonic="stx"; break;
	default:
	  sprintf(buf, "op3(0x%02x)", op3);
	  mnemonic = buf;
	  break;
	}
	if (op3 & 0x04) {
	  /* Store */
	  fprintf(stderr, "%s ", mnemonic);
	  sparc_disass_reg(opcode >> 25);
	  fprintf(stderr, ", [");
	  sparc_disass_reg(opcode >> 14);
	  if (opcode & 0x00002000) {
	    fprintf(stderr, ", 0x%04x", opcode & 0x1fff);
	  } else {
	    fprintf(stderr, ", ");
	    sparc_disass_reg(opcode);
	  }
	  fprintf(stderr, "]\n");
	} else {
	  /* Load */
	  fprintf(stderr, "%s [", mnemonic);
	  sparc_disass_reg(opcode >> 14);
	  if (opcode & 0x00002000) {
	    fprintf(stderr, ", 0x%04x", opcode & 0x1fff);
	  } else {
	    fprintf(stderr, ", ");
	    sparc_disass_reg(opcode);
	  }
	  fprintf(stderr, "], ");
	  sparc_disass_reg(opcode >> 25);
	  fprintf(stderr, "\n");
	}
      }
      break;
    }
    code++;
  }
}
