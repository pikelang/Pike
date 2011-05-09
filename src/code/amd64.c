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

/* We reserve register r13 and above (as well as RSP and RBP). */
#define REG_BITMASK	((1 << REG_MAX) - 1)
#define REG_RESERVED	(REG_RSP|REG_RBP)
#define REG_MAX			REG_R13
#define Pike_fp_reg		REG_R13
#define Pike_interpreter_reg	REG_R14
#define imm32_offset_reg	REG_R15

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
    add_to_program(0x80|(from_reg__<<3)| to_reg__);			\
    PUSH_INT(off32__);							\
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
    add_to_program(0x21);			\
    add_to_program(0xc0|(reg__ <<3)|reg__);	\
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

/* CALL *(addr)(%r15) */
#define CALL_ABSOLUTE(X)						\
    AMD64_CALL_REL32(imm32_offset_reg, ((char *)(X)) - ((char *)0x80000000LL));

#define AMD64_CLEAR_REG(REG) do {		\
    enum amd64_reg reg__ = (REG);		\
    if (reg__ & 0x08) {				\
      add_to_program(0x45);			\
    }						\
    add_to_program(0x31);			\
    add_to_program((reg__<<3)|0xc0);		\
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
      add_to_program(0x80|(to_reg__<<3)|from_reg__);		\
      PUSH_INT(imm32__);					\
    }								\
  } while(0)

#define AMD64_LOAD_IMM32(REG, IMM32) do {	\
    enum amd64_reg reg__ = (REG);		\
    int imm32__ = (IMM32);			\
    if (!imm32__) {				\
      AMD64_CLEAR_REG(reg__);			\
    } else {					\
      if (reg__ & 0x08) {			\
	add_to_program(0x4c);			\
	reg__ &= 0x07;				\
      } else {					\
	add_to_program(0x48);			\
      }						\
      add_to_program(0x8d);			\
      add_to_program((reg__<<3)|0x04);		\
      add_to_program(0x25);			\
      PUSH_INT(imm32__);			\
    }						\
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
 *   R13: Pike_fp
 *   R14: Pike_interpreter
 *   R15: 0x0000000080000000
 */
void amd64_ins_entry(void)
{
  /* Push all registers that the ABI requires to be preserved. */
  AMD64_PUSH(REG_R15);
  AMD64_PUSH(REG_R14);
  AMD64_PUSH(REG_R13);
  AMD64_PUSH(REG_R12);
  AMD64_PUSH(REG_RBP);
  AMD64_PUSH(REG_RBX);
  AMD64_SUB_IMM32(REG_RSP, 8);	/* Align on 16 bytes. */

  AMD64_LOAD_IMM32(imm32_offset_reg, 0x40000000);

  /* REX.W SHL %r15, 1 */ /* FIXME: imm32_offset_reg */
  add_to_program(0x49);
  add_to_program(0xd1);
  add_to_program(0xe7);

  AMD64_MOV_REG(ARG1_REG, Pike_interpreter_reg);

  AMD64_MOVE_RELADDR_TO_REG(Pike_interpreter_reg,
			    OFFSETOF(Pike_interpreter, frame_pointer),
			    Pike_fp_reg);
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

static enum amd64_reg next_reg;
static enum amd64_reg sp_reg, mark_sp_reg;
ptrdiff_t amd64_prev_stored_pc = -1; /* PROG_PC at the last point Pike_fp->pc was updated. */

static enum amd64_reg alloc_reg (int avoid_regs)
{
  enum amd64_reg reg;
  int used_regs = (avoid_regs | REG_RESERVED | \
		   (1 << sp_reg) | (1 << mark_sp_reg)) & REG_BITMASK;

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

    if (sp_reg == reg)			{sp_reg = REG_NONE; DEALLOC_REG (reg);}
    else if (mark_sp_reg == reg)	{mark_sp_reg = REG_NONE; DEALLOC_REG (reg);}
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



static void amd64_call_c_function(void *addr)
{
  CALL_ABSOLUTE(addr);
  next_reg = REG_RAX;
  sp_reg = mark_sp_reg = REG_NONE;
  CLEAR_REGS();
}

void amd64_update_pc(void)
{
  INT32 tmp = PIKE_PC, disp;
	
  if (amd64_prev_stored_pc < 0) {
    enum amd64_reg tmp_reg = alloc_reg (1 << Pike_fp_reg);
    AMD64_MOV_REG(REG_RSP, tmp_reg);
    AMD64_ADD_REG_IMM32(Pike_fp_reg, OFFSETOF(pike_frame, pc) + 0x08, REG_RSP);
    /* CALL .+0 */
    add_to_program(0xe8);
    PUSH_INT(0x00000000);
    AMD64_MOV_REG(tmp_reg, REG_RSP);
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
    AMD64_ADD_VAL_TO_RELADDR (disp, OFFSETOF (pike_frame, pc), Pike_fp_reg);
  }
  else {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc - already up-to-date\n", tmp);
#endif
  }
  amd64_prev_stored_pc = tmp;
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

void ins_f_byte(unsigned int b)
{
  void *addr;
  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
  maybe_update_pc();
  addr=instrs[b].address;
  amd64_call_c_function(addr);
  if (instrs[b].flags & I_BRANCH) {
    AMD64_TEST_REG(REG_RAX);
    AMD64_JNE(0x02);
    AMD64_JUMP_REG(REG_RAX);
  } else if (instrs[b].flags & I_JUMP) {
    AMD64_JUMP_REG(REG_RAX);
  }
}

void ins_f_byte_with_arg(unsigned int a, INT32 b)
{
  maybe_update_pc();
  update_arg1(b);
  ins_f_byte(a);
}

void ins_f_byte_with_2_args(unsigned int a, INT32 b, INT32 c)
{
  maybe_update_pc();
  update_arg2(c);
  update_arg1(b);
  ins_f_byte(a);
}

