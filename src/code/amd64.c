/*
 * Machine code generator for AMD64.
 */

#include "operators.h"
#include "constants.h"
#include "object.h"
#include "builtin_functions.h"


/* This is defined on windows */
#ifdef REG_NONE
#undef REG_NONE
#endif


/* Register encodings */
enum amd64_reg {REG_RAX = 0, REG_RBX = 3, REG_RCX = 1, REG_RDX = 2,
		REG_RSP = 4, REG_RBP = 5, REG_RSI = 6, REG_RDI = 7,
		REG_R8 = 8, REG_R9 = 9, REG_R10 = 10, REG_R11 = 11,
		REG_R12 = 12, REG_R13 = 13, REG_R14 = 14, REG_R15 = 15,
		REG_NONE = 4};

/* We reserve register r12 and above (as well as RSP, RBP and RBX). */
#define REG_BITMASK	((1 << REG_MAX) - 1)
#define REG_RESERVED	(REG_RSP|REG_RBP|REG_RBX)
#define REG_MAX			REG_R12
#define PIKE_MARK_SP_REG	REG_R12
#define PIKE_SP_REG		REG_R13
#define PIKE_FP_REG		REG_R14
#define Pike_interpreter_reg	REG_R15

#ifdef __NT__
/* From http://software.intel.com/en-us/articles/introduction-to-x64-assembly/
 *
 * Note: Space for the arguments needs to be allocated on the stack as well.
 */
#define ARG1_REG	REG_RCX
#define ARG2_REG	REG_RDX
#define ARG3_REG	REG_R8
#define ARG4_REG	REG_R9
#else
/* From SysV ABI for AMD64 draft 0.99.5. */
#define ARG1_REG	REG_RDI
#define ARG2_REG	REG_RSI
#define ARG3_REG	REG_RDX
#define ARG4_REG	REG_RCX
#define ARG5_REG	REG_R8
#define ARG6_REG	REG_R9
#endif

#define PUSH_INT(X) ins_int((INT32)(X), (void (*)(char))add_to_program)

#define AMD64_RET() add_to_program(0xc3)

#define AMD64_NOP() add_to_program(0x90)

#define AMD64_PUSH(REG) do {			\
    enum amd64_reg reg__ = (REG);		\
    if (reg__ & 0x08) {				\
      add_to_program(0x41);			\
    }						\
    add_to_program(0x50 + (reg__ & 0x07));	\
  } while(0)

#define AMD64_POP(REG) do {			\
    enum amd64_reg reg__ = (REG);		\
    if (reg__ & 0x08) {				\
      add_to_program(0x41);			\
    }						\
    add_to_program(0x58 + (reg__ & 0x07));	\
  } while(0)

#define AMD64_MOV_REG(FROM_REG, TO_REG) do {	\
    enum amd64_reg from_reg__ = (FROM_REG);	\
    enum amd64_reg to_reg__ = (TO_REG);		\
    int rex__ = 0x48;				\
    if (from_reg__ & 0x08) {			\
      rex__ |= 0x04;				\
      from_reg__ &= 0x07;			\
    }						\
    if (to_reg__ & 0x08) {			\
      rex__ |= 0x01;				\
      to_reg__ &= 0x07;				\
    }						\
    add_to_program(rex__);			\
    add_to_program(0x89);			\
    add_to_program(0xc0|(from_reg__<<3)|	\
		   to_reg__);			\
  } while (0)

#define AMD64_MOVE_RELADDR_TO_REG(FROM_REG, FROM_OFFSET, TO_REG) do {	\
    enum amd64_reg from_reg__ = (FROM_REG);				\
    enum amd64_reg to_reg__ = (TO_REG);					\
    int off32__ = (FROM_OFFSET);					\
    int rex__ = 0x48;							\
    if (from_reg__ & 0x08) {						\
      rex__ |= 0x04;							\
      from_reg__ &= 0x07;						\
    }									\
    if (to_reg__ & 0x08) {						\
      rex__ |= 0x01;							\
      to_reg__ &= 0x07;							\
    }									\
    add_to_program(rex__);						\
    add_to_program(0x8b);						\
    add_to_program(0x80|(to_reg__<<3)| from_reg__);			\
    PUSH_INT(off32__);							\
  } while(0)

#define AMD64_MOVE_REG_TO_RELADDR(FROM_REG, TO_REG, TO_OFFSET) do {	\
    enum amd64_reg from_reg__ = (FROM_REG);				\
    enum amd64_reg to_reg__ = (TO_REG);					\
    int off32__ = (TO_OFFSET);						\
    int rex__ = 0x48;							\
    if (from_reg__ & 0x08) {						\
      rex__ |= 0x04;							\
      from_reg__ &= 0x07;						\
    }									\
    if (to_reg__ & 0x08) {						\
      rex__ |= 0x01;							\
      to_reg__ &= 0x07;							\
    }									\
    add_to_program(rex__);						\
    add_to_program(0x89);						\
    if (!off32__ && (to_reg__ != REG_RBP)) {				\
      add_to_program((from_reg__<<3)| to_reg__);			\
      if (to_reg__ == REG_RSP) {					\
	add_to_program(0x24);						\
      }									\
    } else if ((-0x80 <= off32__) && (0x7f >= off32__)) {		\
      add_to_program(0x40|(from_reg__<<3)| to_reg__);			\
      if (to_reg__ == REG_RSP) {					\
	add_to_program(0x24);						\
      }									\
      add_to_program(off32__);						\
    } else {								\
      add_to_program(0x80|(from_reg__<<3)| to_reg__);			\
      PUSH_INT(off32__);						\
    }									\
  } while(0)

#define AMD64_SHL_IMM8(REG, IMM8) do {	\
    enum amd64_reg reg__ = (REG);	\
    int imm8__ = (IMM8);		\
    if (reg__ & 0x08) {			\
      add_to_program(0x49);		\
      reg__ &= 0x07;			\
    } else {				\
      add_to_program(0x48);		\
    }					\
    if (imm8__ == 1) {			\
      add_to_program(0xd1);		\
      add_to_program(0xe0|reg__);	\
    } else {				\
      add_to_program(0xc1);		\
      add_to_program(0xe0|reg__);	\
      add_to_program(imm8__);		\
    }					\
  } while(0)

#define AMD64_AND_IMM32(REG, IMM32) do {		\
    enum amd64_reg reg__ = (REG);			\
    int imm32__ = (IMM32);				\
    if (reg__ & 0x08) {					\
      add_to_program(0x49);				\
      reg__ &= 0x07;					\
    } else {						\
      add_to_program(0x48);				\
    }							\
    add_to_program(0x83);				\
    if ((imm32__ >= -0x80) && (imm32__ <= 0x7f)) {	\
      add_to_program(0xe0|reg__);			\
      add_to_program(imm32__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
      PUSH_INT(imm32__);				\
    }							\
  } while(0)

#define AMD64_ADD_IMM32(REG, IMM32) do {		\
    enum amd64_reg reg__ = (REG);			\
    int imm32__ = (IMM32);				\
    if (reg__ & 0x08) {					\
      add_to_program(0x49);				\
      reg__ &= 0x07;					\
    } else {						\
      add_to_program(0x48);				\
    }							\
    add_to_program(0x83);				\
    if ((imm32__ >= -0x80) && (imm32__ <= 0x7f)) {	\
      add_to_program(0xc0|reg__);			\
      add_to_program(imm32__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
      PUSH_INT(imm32__);				\
    }							\
  } while(0)

#define AMD64_SUB_IMM32(REG, IMM32) do {		\
    enum amd64_reg reg__ = (REG);			\
    int imm32__ = (IMM32);				\
    if (reg__ & 0x08) {					\
      add_to_program(0x49);				\
      reg__ &= 0x07;					\
    } else {						\
      add_to_program(0x48);				\
    }							\
    add_to_program(0x83);				\
    if ((imm32__ >= -0x80) && (imm32__ <= 0x7f)) {	\
      add_to_program(0xe8|reg__);			\
      add_to_program(imm32__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
      PUSH_INT(imm32__);				\
    }							\
  } while(0)

#define AMD64_TEST_REG(REG) do {		\
    enum amd64_reg reg__ = (REG);		\
    if (reg__ & 0x08) {				\
      add_to_program(0x4d);			\
      reg__ &= 0x07;				\
    } else {					\
      add_to_program(0x48);			\
    }						\
    add_to_program(0x85);			\
    add_to_program(0xc0|(reg__ <<3)|reg__);	\
  } while(0)

#define AMD64_CMP_REG_IMM32(REG, IMM32) do {		\
    enum amd64_reg reg__ = (REG);			\
    int imm32__ = (IMM32);				\
    if (reg__ & 0x08) {					\
      add_to_program(0x49);				\
      reg__ &= 0x07;					\
    } else {						\
      add_to_program(0x48);				\
    }							\
    add_to_program(0x83);				\
    if ((imm32__ >= -0x80) && (imm32__ <= 0x7f)) {	\
      add_to_program(0xf8|reg__);			\
      add_to_program(imm32__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
      PUSH_INT(imm32__);				\
    }							\
  } while(0)

#define AMD64_CMP_REG(REG1, REG2) do {			\
    enum amd64_reg reg1__ = (REG1);			\
    enum amd64_reg reg2__ = (REG2);			\
    int rex = 0x48;					\
    if (reg1__ & 0x08) {				\
      rex |= 0x04;					\
      reg1__ &= 0x07;					\
    }							\
    if (reg2__ & 0x08) {				\
      rex |= 0x01;					\
      reg2__ &= 0x07;					\
    }							\
    add_to_program(rex);				\
    add_to_program(0x39);				\
    add_to_program(0xc0|(reg1__<<3)|reg2__);		\
  } while(0)

#define AMD64_JUMP_REG(REG) do {		\
    enum amd64_reg reg__ = (REG);		\
    if (reg__ & 0x08) {				\
      add_to_program(0x41);			\
      reg__ &= 0x07;				\
    }						\
    add_to_program(0xff);			\
    add_to_program(0xe0|reg__);			\
  } while(0)

#define AMD64_CALL_REL32(REG, REL32) do {	\
    AMD64_ADD_REG_IMM32(REG, REL32, REG_RAX);	\
    add_to_program(0xff);			\
    add_to_program(0xd0);			\
  } while(0)

/* CALL *addr */
#define CALL_ABSOLUTE(X) do {			\
    size_t addr__ = (size_t)(void *)(X);	\
    if (addr__ & ~0x7fffffffLL) {		\
      /* Apple in their wisdom has the text	\
       * segment in the second 4GB block...	\
       *					\
       * Fortunately function entry points	\
       * are at least 32-bit aligned.		\
       */					\
      if (!(addr__ & ~0x3fffffff8LL)) {		\
	AMD64_LOAD_IMM32(REG_RAX, addr__>>3);	\
	AMD64_SHL_IMM8(REG_RAX, 3);		\
      } else {					\
	/* Catch all. */			\
	AMD64_LOAD_IMM(REG_RAX, addr__);	\
      }						\
    } else {					\
      /* Low 31-bit block.			\
       * Linux, Solaris, etc... */		\
      AMD64_LOAD_IMM32(REG_RAX, addr__);	\
    }						\
    add_to_program(0xff);			\
    add_to_program(0xd0);			\
  } while(0)

#define AMD64_CLEAR_REG(REG) do {		\
    enum amd64_reg creg__ = (REG);		\
    if (creg__ & 0x08) {			\
      add_to_program(0x4d);			\
      creg__ &= 0x07;				\
    } else {					\
      add_to_program(0x48);			\
    }						\
    add_to_program(0x31);			\
    add_to_program(0xc0|(creg__<<3)|creg__);	\
  } while(0)

#define AMD64_ADD_REG_IMM32(FROM_REG, IMM32, TO_REG) do {	\
    enum amd64_reg from_reg__ = (FROM_REG);			\
    enum amd64_reg to_reg__ = (TO_REG);				\
    int imm32__ = (IMM32);					\
    if (!imm32__) {						\
      if (from_reg__ != to_reg__) {				\
	AMD64_MOV_REG(from_reg__, to_reg__);			\
      }								\
    } else {							\
      int rex = 0x48;						\
      if (from_reg__ & 0x08) {					\
	rex |= 0x01;						\
	from_reg__ &= 0x07;					\
      }								\
      if (to_reg__ & 0x08) {					\
	rex |= 0x04;						\
	to_reg__ &= 0x07;					\
      }								\
      add_to_program(rex);					\
      add_to_program(0x8d);					\
      if ((-0x80 <= imm32__) && (0x7f >= imm32__)) {		\
	add_to_program(0x40|(to_reg__<<3)|from_reg__);		\
	add_to_program(imm32__);				\
      } else {							\
	add_to_program(0x80|(to_reg__<<3)|from_reg__);		\
	PUSH_INT(imm32__);					\
      }								\
    }								\
  } while(0)

#define AMD64_LOAD_RIP32(RIP32, REG) do {	\
    enum amd64_reg reg__ = (REG);				\
    int rip32__ = (RIP32);					\
    if (reg__ & 0x08) {						\
      add_to_program(0x4c);					\
      reg__ &= 0x07;						\
    } else {							\
      add_to_program(0x48);					\
    }								\
    add_to_program(0x8d);					\
    add_to_program(0x05|(reg__<<3));				\
    PUSH_INT(rip32__);						\
  } while(0)

#define AMD64_MOVE_RIP32_TO_REG(RIP32, REG) do {		\
    enum amd64_reg reg__ = (REG);				\
    int rip32__ = (RIP32);					\
    if (reg__ & 0x08) {						\
      add_to_program(0x4c);					\
      reg__ &= 0x07;						\
    } else {							\
      add_to_program(0x48);					\
    }								\
    add_to_program(0x8b);					\
    add_to_program(0x05|(reg__<<3));				\
    PUSH_INT(rip32__);						\
  } while(0)

#define AMD64_MOVE32_RIP32_TO_REG(RIP32, REG) do {		\
    enum amd64_reg reg__ = (REG);				\
    int rip32__ = (RIP32);					\
    if (reg__ & 0x08) {						\
      add_to_program(0x44);					\
      reg__ &= 0x07;						\
    }								\
    add_to_program(0x8b);					\
    add_to_program(0x05|(reg__<<3));				\
    PUSH_INT(rip32__);						\
  } while(0)

#define AMD64_LOAD_IMM32(REG, IMM32) do {	\
    enum amd64_reg reg__ = (REG);		\
    int imm32__ = (IMM32);			\
    if (!imm32__) {				\
      AMD64_CLEAR_REG(reg__);			\
    } else {					\
      if (reg__ & 0x08) {			\
	add_to_program(0x49);			\
	reg__ &= 0x07;				\
      } else {					\
	add_to_program(0x48);			\
      }						\
      add_to_program(0xc7);			\
      add_to_program(0xc0|reg__);		\
      PUSH_INT(imm32__);			\
    }						\
  } while(0)

#define AMD64_LOAD_IMM(REG, IMM) do {			\
    enum amd64_reg reg64__ = (REG);			\
    INT64 imm__ = (IMM);				\
    if ((-0x80000000LL <= imm__) &&			\
	(0x7fffffffLL >= imm__)) {			\
      AMD64_LOAD_IMM32(reg64__, imm__);			\
    } else {						\
      if (imm__ & 0x80000000LL) {			\
	/* 32-bit negative. */				\
	AMD64_LOAD_IMM32(reg64__, (imm__>>32) + 1);	\
      } else {						\
	AMD64_LOAD_IMM32(reg64__, (imm__>>32));		\
      }							\
      AMD64_SHL_IMM8(reg64__, 32);			\
      AMD64_ADD_REG_IMM32(reg64__, imm__, reg64__);	\
    }							\
  } while(0)

#define AMD64_ADD_VAL_TO_RELADDR(VAL, OFFSET, REG) do {			\
    INT32 val_ = (VAL);							\
    INT32 off_ = (OFFSET);						\
    enum amd64_reg reg_ = (REG);					\
    if (val_) {								\
      if (reg_ & 0x08) {						\
	add_to_program(0x41);						\
	reg_ &= 0x07;							\
      }									\
      if (val_ == 1)							\
	/* incl offset(%reg) */						\
	add_to_program (0xff); /* Increment r/m32 */			\
      else if (val_ == -1) {						\
	/* decl offset(%reg) */						\
	add_to_program (0xff); /* Decrement r/m32 */			\
	reg_ |= 1 << 3;							\
      }									\
      else if (-128 <= val_ && val_ <= 127)				\
	/* addl $val,offset(%reg) */					\
	add_to_program (0x83); /* Add sign-extended imm8 to r/m32. */	\
      else								\
	/* addl $val,offset(%reg) */					\
	add_to_program (0x81); /* Add imm32 to r/m32. */		\
      if (off_ < -128 || off_ > 127) {					\
	add_to_program (0x80 | reg_);					\
	PUSH_INT (off_);						\
      }									\
      else if (off_) {							\
	add_to_program (0x40 | reg_);					\
	add_to_program (off_);						\
      }									\
      else								\
	add_to_program (reg_);						\
      if (val_ != 1 && val_ != -1) {					\
	if (-128 <= val_ && val_ <= 127)				\
	  add_to_program (val_);					\
	else								\
	  PUSH_INT (val_);						\
      }									\
    }									\
  } while (0)

#define AMD64_JMP(SKIP) do {\
    int skip__ = (SKIP);    \
    if ((skip__ >= -0x80) && (skip__ <= 0x7f)) {	\
      add_to_program(0xeb);				\
      add_to_program(skip__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
    }							\
  } while(0)

#define AMD64_JNE(SKIP) do {\
    int skip__ = (SKIP);    \
    if ((skip__ >= -0x80) && (skip__ <= 0x7f)) {	\
      add_to_program(0x75);				\
      add_to_program(skip__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
    }							\
  } while(0)

#define AMD64_JE(SKIP) do {\
    int skip__ = (SKIP);    \
    if ((skip__ >= -0x80) && (skip__ <= 0x7f)) {	\
      add_to_program(0x74);				\
      add_to_program(skip__);				\
    } else {						\
      Pike_error("Not supported yet.\n");		\
    }							\
  } while(0)

/* Machine code entry prologue.
 *
 * On entry:
 *   RDI: Pike_interpreter	(ARG1_REG)
 *
 * During interpreting:
 *   R15: Pike_interpreter
 */
void amd64_ins_entry(void)
{
  /* Push all registers that the ABI requires to be preserved. */
  AMD64_PUSH(REG_RBP);
  AMD64_MOV_REG(REG_RSP, REG_RBP);
  AMD64_PUSH(REG_R15);
  AMD64_PUSH(REG_R14);
  AMD64_PUSH(REG_R13);
  AMD64_PUSH(REG_R12);
  AMD64_PUSH(REG_RBX);
  AMD64_SUB_IMM32(REG_RSP, 8);	/* Align on 16 bytes. */

  AMD64_MOV_REG(ARG1_REG, Pike_interpreter_reg);

  amd64_flush_code_generator_state();
}

#define ALLOC_REG(REG) do {} while (0)
#define DEALLOC_REG(REG) do {} while (0)
#define CLEAR_REGS() do {} while (0)

/*
 * Copies a 32 bit immidiate value to stack
 * 0xc7 = mov
 */
#define MOV_VAL_TO_RELSTACK(VALUE, OFFSET) do {				\
    INT32 off_ = (OFFSET);						\
    add_to_program(0xc7);						\
    if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x84);						\
      add_to_program (0x24);						\
      PUSH_INT (off_);							\
    }									\
    else if (off_) {							\
      add_to_program (0x44);						\
      add_to_program (0x24);						\
      add_to_program (off_);						\
    }									\
    else {								\
      add_to_program (0x04);						\
      add_to_program (0x24);						\
    }									\
    PUSH_INT(VALUE);							\
  } while(0)								\

static enum amd64_reg next_reg = 0;
static enum amd64_reg sp_reg = 0, fp_reg = 0, mark_sp_reg = 0;
static int dirty_regs = 0;
ptrdiff_t amd64_prev_stored_pc = -1; /* PROG_PC at the last point Pike_fp->pc was updated. */

void amd64_flush_code_generator_state(void)
{
  next_reg = 0;
  sp_reg = 0;
  fp_reg = 0;
  mark_sp_reg = 0;
  dirty_regs = 0;
  amd64_prev_stored_pc = -1;
}

static enum amd64_reg alloc_reg (int avoid_regs)
{
  enum amd64_reg reg;
  int used_regs = (avoid_regs | REG_RESERVED) & REG_BITMASK;

  avoid_regs |= REG_RESERVED;

  if (used_regs != REG_BITMASK) {
    /* There's a free register. */

    for (reg = next_reg; (1 << reg) & used_regs;) {
      reg = (reg + 1);
      if (reg >= REG_MAX) reg = 0;
#ifdef PIKE_DEBUG
      if (reg == next_reg) Pike_fatal ("Failed to find a free register.\n");
#endif
    }
  }

  else {
    /* Choose a register with simple round robin. If we get more
     * things to hold than there are registers then this should
     * probably be replaced with an LRU strategy. */

    for (reg = next_reg; (1 << reg) & avoid_regs;) {
      reg = (reg + 1);
      if (reg >= REG_MAX) reg = 0;
#ifdef PIKE_DEBUG
      if (reg == next_reg) Pike_fatal ("Failed to find a non-excluded register.\n");
#endif
    }
  }

#ifdef REGISTER_DEBUG
  if ((1 << reg) & alloc_regs) Pike_fatal ("Clobbering allocated register.\n");
  alloc_regs &= avoid_regs;
#endif
  ALLOC_REG (reg);

  next_reg = reg+1;
  if (next_reg >= REG_MAX) next_reg = 0;

  return reg;
}

/* NB: We load Pike_fp et al into registers that
 *     are persistent across function calls.
 */
void amd64_load_fp_reg(void)
{
  if (!fp_reg) {
    AMD64_MOVE_RELADDR_TO_REG(Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, frame_pointer),
			      PIKE_FP_REG);
    fp_reg = PIKE_FP_REG;
  }
}

void amd64_load_sp_reg(void)
{
  if (!sp_reg) {
    AMD64_MOVE_RELADDR_TO_REG(Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, stack_pointer),
			      PIKE_SP_REG);
    sp_reg = PIKE_SP_REG;
  }
}

void amd64_load_mark_sp_reg(void)
{
  if (!mark_sp_reg) {
    AMD64_MOVE_RELADDR_TO_REG(Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, mark_stack_pointer),
			      PIKE_MARK_SP_REG);
    mark_sp_reg = PIKE_MARK_SP_REG;
  }
}



static void update_arg1(INT32 value)
{
  AMD64_LOAD_IMM32(ARG1_REG, value);
  /* FIXME: Alloc stack space on NT. */
}

static void update_arg2(INT32 value)
{
  AMD64_LOAD_IMM32(ARG2_REG, value);
  /* FIXME: Alloc stack space on NT. */
}

static void amd64_push_int(INT64 value, int subtype)
{
  amd64_load_sp_reg();
  AMD64_LOAD_IMM32(REG_RAX, (subtype<<16) + PIKE_T_INT);
  AMD64_MOVE_REG_TO_RELADDR(REG_RAX, sp_reg, OFFSETOF(svalue, type));
  AMD64_LOAD_IMM(REG_RAX, value);
  AMD64_MOVE_REG_TO_RELADDR(REG_RAX, sp_reg, OFFSETOF(svalue, u.integer));
  AMD64_ADD_IMM32(sp_reg, sizeof(struct svalue));
  dirty_regs |= 1 << sp_reg;
  /* FIXME: Deferred writing of Pike_sp doen't seem to work reliably yet. */
  if (dirty_regs & (1 << PIKE_SP_REG)) {
    AMD64_MOVE_REG_TO_RELADDR(PIKE_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, stack_pointer));
    dirty_regs &= ~(1 << PIKE_SP_REG);
  }
}

static void amd64_call_c_function(void *addr)
{
  CALL_ABSOLUTE(addr);
  next_reg = REG_RAX;
  CLEAR_REGS();
}

static void amd64_stack_error(void)
{
  Pike_fatal("Stack error\n");
}

void amd64_update_pc(void)
{
  INT32 tmp = PIKE_PC, disp;
	
  if (amd64_prev_stored_pc == -1) {
    enum amd64_reg tmp_reg = alloc_reg(0);
    amd64_load_fp_reg();
    AMD64_LOAD_RIP32(tmp - (PIKE_PC + 7), tmp_reg);
    AMD64_MOVE_REG_TO_RELADDR(tmp_reg, fp_reg, OFFSETOF(pike_frame, pc));
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc via call\n", tmp);
#endif
    DEALLOC_REG (tmp_reg);
  }
  else if ((disp = tmp - amd64_prev_stored_pc)) {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc relative: %d\n", tmp, disp);
#endif
    amd64_load_fp_reg();
    AMD64_ADD_VAL_TO_RELADDR (disp, OFFSETOF (pike_frame, pc), fp_reg);
  }
  else {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc - already up-to-date\n", tmp);
#endif
  }
  amd64_prev_stored_pc = tmp;
#ifdef PIKE_DEBUG
  if (d_flag) {
    /* Check that the stack keeps being 16 byte aligned. */
    AMD64_MOV_REG(REG_RSP, REG_RAX);
    AMD64_AND_IMM32(REG_RAX, 0x08);
    AMD64_JE(0x09);
    CALL_ABSOLUTE(amd64_stack_error);
  }
#endif
}


static void maybe_update_pc(void)
{
  static int last_prog_id=-1;
  static size_t last_num_linenumbers=-1;
	
  if(
#ifdef PIKE_DEBUG
    /* Update the pc more often for the sake of the opcode level trace. */
     d_flag ||
#endif
     (amd64_prev_stored_pc == -1) ||
     last_prog_id != Pike_compiler->new_program->id ||
     last_num_linenumbers != Pike_compiler->new_program->num_linenumbers
  ) {
    last_prog_id=Pike_compiler->new_program->id;
    last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
    UPDATE_PC();
  }
}

#ifdef PIKE_DEBUG
static void ins_debug_instr_prologue (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2)
{
  int flags = instrs[instr].flags;

  maybe_update_pc();

  if (flags & I_HASARG2)
    AMD64_LOAD_IMM32(ARG3_REG, arg2);
  if (flags & I_HASARG)
    AMD64_LOAD_IMM32(ARG2_REG, arg1);
  AMD64_LOAD_IMM32(ARG1_REG, instr);

  if (flags & I_HASARG2)
    amd64_call_c_function (simple_debug_instr_prologue_2);
  else if (flags & I_HASARG)
    amd64_call_c_function (simple_debug_instr_prologue_1);
  else
    amd64_call_c_function (simple_debug_instr_prologue_0);
}
#else  /* !PIKE_DEBUG */
#define ins_debug_instr_prologue(instr, arg1, arg2)
#endif

void amd64_init_interpreter_state(void)
{
  instrs[F_CATCH - F_OFFSET].address = inter_return_opcode_F_CATCH;
}

void ins_f_byte(unsigned int b)
{
  int flags;
  void *addr;
  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
  maybe_update_pc();

  flags = instrs[b].flags;

  addr=instrs[b].address;
  switch(b + F_OFFSET) {
  case F_CATCH:
    {
      /* Special argument for the F_CATCH instruction. */
      int ptr_offset = 0x20 - 0x03;
      size_t int_addr;
      addr = inter_return_opcode_F_CATCH;
      int_addr = (size_t)addr;
      /* We need to compensate for the size of the code to call
       * inter_return_opcode_F_CATCH.
       */
      if (int_addr & ~0x7fffffffLL) {
	/* Adjust for an additional SHL. */
	ptr_offset += 0x04;
	if (int_addr & ~0x3fffffff8LL) {
	  Pike_error("Not supported yet.\n");
	}
      }
      AMD64_LOAD_RIP32(ptr_offset, ARG1_REG);	/* Address for the POINTER. */
    }
    break;
  case F_UNDEFINED:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(0, 1);
    return;
  case F_CONST0:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(0, 0);
    return;
  case F_CONST1:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(1, 0);
    return;
  case F_CONST_1:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(-1, 0);
    return;
  case F_BIGNUM:
    ins_debug_instr_prologue(b, 0, 0);
    amd64_push_int(0x7fffffff, 0);
    return;
  case F_RETURN_1:
    ins_f_byte(F_CONST1);
    ins_f_byte(F_RETURN);
    return;
  case F_RETURN_0:
    ins_f_byte(F_CONST0);
    ins_f_byte(F_RETURN);
    return;
  case F_ADD:
    ins_debug_instr_prologue(b, 0, 0);
    update_arg1(2);
    addr = f_add;
    break;
  }

  /* NB: PIKE_FP_REG is currently never dirty. */
  if (dirty_regs & (1 << PIKE_SP_REG)) {
    AMD64_MOVE_REG_TO_RELADDR(PIKE_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, stack_pointer));
    dirty_regs &= ~(1 << PIKE_SP_REG);
  }
  if (dirty_regs & (1 << PIKE_MARK_SP_REG)) {
    AMD64_MOVE_REG_TO_RELADDR(PIKE_MARK_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, mark_stack_pointer));
    dirty_regs &= ~(1 << PIKE_MARK_SP_REG);
  }
  if (flags & I_UPDATE_SP) sp_reg = 0;
  if (flags & I_UPDATE_M_SP) mark_sp_reg = 0;
  if (flags & I_UPDATE_FP) fp_reg = 0;

  amd64_call_c_function(addr);
  if (instrs[b].flags & I_RETURN) {
    if ((b + F_OFFSET) == F_RETURN_IF_TRUE) {
      /* Kludge. We must check if the ret addr is
       * orig_addr + JUMP_EPILOGUE_SIZE. */
      AMD64_LOAD_RIP32(JUMP_EPILOGUE_SIZE - 7, REG_RCX);
    }
    AMD64_CMP_REG_IMM32(REG_RAX, -1);
    AMD64_JNE(0x0f - 0x03);
    AMD64_POP(REG_RBX);	// Stack padding.
    AMD64_POP(REG_RBX);
    AMD64_POP(REG_R12);
    AMD64_POP(REG_R13);
    AMD64_POP(REG_R14);
    AMD64_POP(REG_R15);
    AMD64_POP(REG_RBP);
    AMD64_RET();
    if ((b + F_OFFSET) == F_RETURN_IF_TRUE) {
      /* Kludge. We must check if the ret addr is
       * orig_addr + JUMP_EPILOGUE_SIZE. */
      AMD64_CMP_REG(REG_RAX, REG_RCX);
      AMD64_JE(0x02);
      AMD64_JUMP_REG(REG_RAX);
      return;
    }
  }
  if (flags & I_JUMP) {
    AMD64_JUMP_REG(REG_RAX);
  }
}

int amd64_ins_f_jump(unsigned int op, int backward_jump)
{
  int flags;
  void *addr;
  int off = op - F_OFFSET;
  int ret = -1;
#ifdef PIKE_DEBUG
  if(off>255)
    Pike_error("Instruction too big %d\n",off);
#endif
  flags = instrs[off].flags;
  if (!(flags & I_BRANCH)) return -1;

  maybe_update_pc();

  /* NB: PIKE_FP_REG is currently never dirty. */
  if (dirty_regs & (1 << PIKE_SP_REG)) {
    amd64_load_fp_reg();
    AMD64_MOVE_REG_TO_RELADDR(PIKE_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, stack_pointer));
    dirty_regs &= ~(1 << PIKE_SP_REG);
  }
  if (dirty_regs & (1 << PIKE_MARK_SP_REG)) {
    amd64_load_fp_reg();
    AMD64_MOVE_REG_TO_RELADDR(PIKE_MARK_SP_REG, Pike_interpreter_reg,
			      OFFSETOF(Pike_interpreter, mark_stack_pointer));
    dirty_regs &= ~(1 << PIKE_MARK_SP_REG);
  }

  if (flags & I_UPDATE_SP) sp_reg = 0;
  if (flags & I_UPDATE_M_SP) mark_sp_reg = 0;
  if (flags & I_UPDATE_FP) fp_reg = 0;

  if (op == F_BRANCH) {
    ins_debug_instr_prologue(off, 0, 0);
    if (backward_jump) {
      amd64_call_c_function(branch_check_threads_etc);
    }
    add_to_program(0xe9);
    ret=DO_NOT_WARN( (INT32) PIKE_PC );
    PUSH_INT(0);
    return ret;
  }

  addr=instrs[off].address;
  amd64_call_c_function(addr);
  AMD64_TEST_REG(REG_RAX);

  if (backward_jump) {
    add_to_program (0x74);	/* jz rel8 */
    add_to_program (9 + 1 + 4);
    amd64_call_c_function (branch_check_threads_etc); /* 9 bytes */
    add_to_program (0xe9);	/* jmp rel32 */
    ret = DO_NOT_WARN ((INT32) PIKE_PC);
    PUSH_INT (0);		/* 4 bytes */
  }
  else {
#if 0
    if ((cpu_vendor == PIKE_CPU_VENDOR_AMD) &&
	(op == F_LOOP || op == F_FOREACH)) {
      /* FIXME: Slows down Intel PIII/x86 by 7%, speeds up Athlon XP by 22%. :P */
      add_to_program (0x3e);	/* Branch taken hint. */
    }
#endif
    add_to_program (0x0f);	/* jnz rel32 */
    add_to_program (0x85);
    ret = DO_NOT_WARN ((INT32) PIKE_PC);
    PUSH_INT (0);
  }

  return ret;
}

void ins_f_byte_with_arg(unsigned int a, INT32 b)
{
  maybe_update_pc();
  switch(a) {
  case F_NUMBER:
    ins_debug_instr_prologue(a-F_OFFSET, 0, 0);
    amd64_push_int(b, 0);
    return;
  case F_NEG_NUMBER:
    ins_debug_instr_prologue(a-F_OFFSET, 0, 0);
    amd64_push_int(-(INT64)b, 0);
    return;
  case F_POS_INT_INDEX:
    ins_f_byte_with_arg(F_NUMBER, b);
    ins_f_byte(F_INDEX);
    return;
  case F_NEG_INT_INDEX:
    ins_f_byte_with_arg(F_NEG_NUMBER, b);
    ins_f_byte(F_INDEX);
    return;
  }
  update_arg1(b);
  ins_f_byte(a);
}

int amd64_ins_f_jump_with_arg(unsigned int op, INT32 a, int backward_jump)
{
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;
  maybe_update_pc();
  update_arg1(a);
  return amd64_ins_f_jump(op, backward_jump);
}

void ins_f_byte_with_2_args(unsigned int a, INT32 b, INT32 c)
{
  maybe_update_pc();
  switch(a) {
  case F_NUMBER64:
    ins_debug_instr_prologue(a-F_OFFSET, 0, 0);
    amd64_push_int((((unsigned INT64)b)<<32)|(unsigned INT32)c, 0);
    return;
  }
  update_arg2(c);
  update_arg1(b);
  ins_f_byte(a);
}

int amd64_ins_f_jump_with_2_args(unsigned int op, INT32 a, INT32 b,
				 int backward_jump)
{
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;
  maybe_update_pc();
  update_arg2(b);
  update_arg1(a);
  return amd64_ins_f_jump(op, backward_jump);
}

void amd64_update_f_jump(INT32 offset, INT32 to_offset)
{
  upd_pointer(offset, to_offset - offset - 4);
}

INT32 amd64_read_f_jump(INT32 offset)
{
  return read_pointer(offset) + offset + 4;
}

