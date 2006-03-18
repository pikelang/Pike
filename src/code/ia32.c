/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ia32.c,v 1.42 2006/03/18 12:52:29 grubba Exp $
*/

/*
 * Machine code generator for IA32.
 */

#include "operators.h"
#include "constants.h"
#include "object.h"
#include "builtin_functions.h"

/* This is defined on windows */
#ifdef REG_NONE
#undef REG_NONE
#endif

enum ia32_reg {REG_EAX = 0, REG_EBX = 3, REG_ECX = 1, REG_EDX = 2, REG_NONE = 4};

#define REG_BITMASK ((1 << REG_NONE) - 1)

/* #define REGISTER_DEBUG */

#ifdef REGISTER_DEBUG
static int alloc_regs = 0, valid_regs = 0;
#define ALLOC_REG(REG) do {						\
    alloc_regs |= (1 << (REG));						\
  } while (0)
#define DEALLOC_REG(REG) do {						\
    alloc_regs &= ~(1 << (REG));					\
    valid_regs &= ~(1 << (REG));					\
  } while (0)
#define CHECK_ALLOC_REG(REG) do {					\
    if (!((1 << (REG)) & alloc_regs))					\
      Pike_fatal ("Attempt to use unallocated register.\n");		\
  } while (0)
#define MAKE_VALID_REG(REG) do {					\
    CHECK_ALLOC_REG (REG);						\
    valid_regs |= (1 << (REG));						\
  } while (0)
#define CHECK_VALID_REG(REG) do {					\
    if (!((1 << (REG)) & valid_regs))					\
      Pike_fatal ("Attempt to use register without valid value.\n");	\
  } while (0)
#define CLEAR_REGS() do {alloc_regs = valid_regs = 0;} while (0)
#else
#define ALLOC_REG(REG) do {} while (0)
#define DEALLOC_REG(REG) do {} while (0)
#define CHECK_ALLOC_REG(REG) do {} while (0)
#define MAKE_VALID_REG(REG) do {} while (0)
#define CHECK_VALID_REG(REG) do {} while (0)
#define CLEAR_REGS() do {} while (0)
#endif

#define PUSH_INT(X) ins_int((INT32)(X), (void (*)(char))add_to_program)
#define PUSH_ADDR(X) PUSH_INT((X))

#define NOP() add_to_program(0x90); /* for alignment */

#define MOV_VAL32_TO_REG(VAL, REG) do {					\
    /* movl $val,%reg */						\
    add_to_program (0xb8 | (REG)); /* Move imm32 to r32. */		\
    PUSH_INT (VAL); /* Assumed to be added last. */			\
    MAKE_VALID_REG (REG);						\
  } while (0)

#define MOV_ABSADDR_TO_REG(ADDR, REG) do {				\
    MAKE_VALID_REG (REG);						\
    /* movl addr,%reg */						\
    if ((REG) == REG_EAX)						\
      add_to_program (0xa1); /* Move dword at address to EAX. */	\
    else {								\
      add_to_program (0x8b); /* Move r/m32 to r32. */			\
      add_to_program (0x5 | ((REG) << 3));				\
    }									\
    PUSH_ADDR (&(ADDR));						\
  } while (0)

#define MOV_REG_TO_ABSADDR(REG, ADDR) do {				\
    CHECK_VALID_REG (REG);						\
    /* movl %reg,addr */						\
    if ((REG) == REG_EAX)						\
      add_to_program (0xa3); /* Move EAX to dword at address. */	\
    else {								\
      add_to_program (0x89); /* Move r32 to r/m32. */			\
      add_to_program (0x5 | ((REG) << 3));				\
    }									\
    PUSH_ADDR (&(ADDR));						\
  } while (0)

#define MOV_RELADDR_TO_REG(OFFSET, REG, DSTREG) do {			\
    INT32 off_ = (OFFSET);						\
    CHECK_VALID_REG (REG);						\
    MAKE_VALID_REG (DSTREG);						\
    /* movl offset(%reg),%dstreg */					\
    add_to_program (0x8b); /* Move r/m32 to r32. */			\
    if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x80 | ((DSTREG) << 3) | (REG));			\
      PUSH_INT (off_);							\
    }									\
    else if (off_) {							\
      add_to_program (0x40 | ((DSTREG) << 3) | (REG));			\
      add_to_program (off_);						\
    }									\
    else								\
      add_to_program (((DSTREG) << 3) | (REG));				\
  } while (0)

#define MOV_RELADDR16_TO_REG(OFFSET, REG, DSTREG) do {			\
    INT32 off_ = (OFFSET);						\
    CHECK_VALID_REG (REG);						\
    MAKE_VALID_REG (DSTREG);						\
    /* movswl offset(%reg),%dstreg */					\
    add_to_program (0x0f); /* Move r/m16 to r32, sign-extension. */	\
    add_to_program (0xbf);						\
    if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x80 | ((DSTREG) << 3) | (REG));			\
      PUSH_INT (off_);							\
    }									\
    else if (off_) {							\
      add_to_program (0x40 | ((DSTREG) << 3) | (REG));			\
      add_to_program (off_);						\
    }									\
    else								\
      add_to_program (((DSTREG) << 3) | (REG));				\
  } while (0)

#define MOV_REG_TO_RELADDR(SRCREG, OFFSET, REG) do {			\
    INT32 off_ = (OFFSET);						\
    CHECK_VALID_REG (SRCREG);						\
    CHECK_VALID_REG (REG);						\
    /* movl %srcreg,offset(%reg) */					\
    add_to_program (0x89); /* Move r32 to r/m32. */			\
    if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x80 | ((SRCREG) << 3) | (REG));			\
      PUSH_INT (off_);							\
    }									\
    else if (off_) {							\
      add_to_program (0x40 | ((SRCREG) << 3) | (REG));			\
      add_to_program (off_);						\
    }									\
    else								\
      add_to_program (((SRCREG) << 3) | (REG));				\
  } while (0)

#define MOV_VAL_TO_RELADDR(VALUE, OFFSET, REG) do {			\
    INT32 off_ = (OFFSET);						\
    CHECK_VALID_REG (REG);						\
    /* movl $value,offset(%reg) */					\
    add_to_program(0xc7); /* Move imm32 to r/m32. */			\
    if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x80 | (REG));					\
      PUSH_INT (off_);							\
    }									\
    else if (off_) {							\
      add_to_program(0x40 | (REG));					\
      add_to_program(off_);						\
    }else								\
      add_to_program(REG);						\
    PUSH_INT (VALUE);							\
  }while(0)

#define MOV_VAL_TO_RELSTACK(VALUE, OFFSET) do {				\
    INT32 off_ = (OFFSET);						\
    /* movl $value,offset(%esp) */					\
    add_to_program (0xc7); /* Move imm32 to r/m32. */			\
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
    PUSH_INT (VALUE);							\
  } while (0)

#define MOV_REG_TO_RELSTACK(REG, OFFSET) do {				\
    INT32 off_ = (OFFSET);						\
    CHECK_VALID_REG (REG);						\
    /* movl %reg,offset(%esp) */					\
    add_to_program (0x89); /* Move r32 to r/m32. */			\
    if (off_ < -128 || off_ > 127) {					\
      add_to_program (0x84 | ((REG) << 3));				\
      add_to_program (0x24);						\
      PUSH_INT (off_);							\
    }									\
    else if (off_) {							\
      add_to_program (0x44 | ((REG) << 3));				\
      add_to_program (0x24);						\
      add_to_program (off_);						\
    }									\
    else {								\
      add_to_program (0x04 | ((REG) << 3));				\
      add_to_program (0x24);						\
    }									\
  } while (0)

#define ADD_VAL_TO_REG(VAL, REG) do {					\
    INT32 val_ = (VAL);							\
    CHECK_VALID_REG (REG);						\
    if (val_ == 1)							\
      /* incl %reg */							\
      add_to_program (0x40 | (REG)); /* Increment r32. */		\
    else if (val_ == -1)						\
      /* decl %reg */							\
      add_to_program (0x48 | (REG)); /* Decrement r32. */		\
    else if (val_ < -128 || val_ > 127) {				\
      /* addl $val,%reg */						\
      if ((REG) == REG_EAX)						\
	add_to_program (0x05); /* Add imm32 to EAX. */			\
      else {								\
	add_to_program (0x81); /* Add imm32 to r/m32. */		\
	add_to_program (0xc0 | (REG));					\
      }									\
      PUSH_INT (val_);							\
    }									\
    else if (val_) {							\
      /* addl $val,%reg */						\
      add_to_program (0x83); /* Add sign-extended imm8 to r/m32. */	\
      add_to_program (0xc0 | (REG));					\
      add_to_program (val_);						\
    }									\
  } while (0)

#define ADD_VAL_TO_RELADDR(VAL, OFFSET, REG) do {			\
    INT32 val_ = (VAL);							\
    INT32 off_ = (OFFSET);						\
    CHECK_VALID_REG (REG);						\
    if (val_) {								\
      int opcode_extra_ = 0;						\
      if (val_ == 1)							\
	/* incl offset(%reg) */						\
	add_to_program (0xff); /* Increment r/m32 */			\
      else if (val_ == -1) {						\
	/* decl offset(%reg) */						\
	add_to_program (0xff); /* Decrement r/m32 */			\
	opcode_extra_ = 1 << 3;						\
      }									\
      else if (-128 <= val_ && val_ <= 127)				\
	/* addl $val,offset(%reg) */					\
	add_to_program (0x83); /* Add sign-extended imm8 to r/m32. */	\
      else								\
	/* addl $val,offset(%reg) */					\
	add_to_program (0x81); /* Add imm32 to r/m32. */		\
      if (off_ < -128 || off_ > 127) {					\
	add_to_program (0x80 | opcode_extra_ | (REG));			\
	PUSH_INT (off_);						\
      }									\
      else if (off_) {							\
	add_to_program (0x40 | opcode_extra_ | (REG));			\
	add_to_program (off_);						\
      }									\
      else								\
	add_to_program (opcode_extra_ | (REG));				\
      if (val_ != 1 && val_ != -1) {					\
	if (-128 <= val_ && val_ <= 127)				\
	  add_to_program (val_);					\
	else								\
	  PUSH_INT (val_);						\
      }									\
    }									\
  } while (0)

#define ADD_VAL_TO_ABSADDR(VAL, ADDR) do {				\
    int val_ = (VAL);							\
    if (val_ == 1) {							\
      /* incl addr */							\
      add_to_program (0xff); /* Increment r/m32. */			\
      add_to_program (0x05);						\
      PUSH_ADDR (&(ADDR));						\
    }									\
    else if (val_ == -1) {						\
      /* decl addr */							\
      add_to_program (0xff); /* Decrement r/m32. */			\
      add_to_program (0x0d);						\
      PUSH_ADDR (&(ADDR));						\
    }									\
    else if (val_) {							\
      /* addl $val,addr */						\
      if (-128 <= val_ && val_ < 127)					\
	add_to_program (0x83); /* Add sign-extended imm8 to r/m32. */	\
      else								\
	add_to_program (0x81); /* Add imm32 to r/m32. */		\
      add_to_program (0x05);						\
      PUSH_ADDR (&(ADDR));						\
      if (-128 <= val_ && val_ <= 127)					\
	add_to_program (val_);						\
      else								\
	PUSH_INT (val_);						\
    }									\
  } while (0)

#define CALL_RELATIVE(X) do{						\
  struct program *p_=Pike_compiler->new_program;			\
  add_to_program(0xe8);							\
  add_to_program(0);                                                    \
  add_to_program(0);                                                    \
  add_to_program(0);                                                    \
  add_to_program(0);                                                    \
  add_to_relocations(p_->num_program-4);				\
  *(INT32 *)(p_->program + p_->num_program - 4)=                        \
    ((INT32)(X)) - (INT32)(p_->program + p_->num_program);              \
}while(0)

static void update_arg1(INT32 value)
{
  MOV_VAL_TO_RELSTACK (value, 0);
}

static void update_arg2(INT32 value)
{
  MOV_VAL_TO_RELSTACK (value, 4);
}

#define PIKE_CPU_VENDOR_UNKNOWN	0
#define PIKE_CPU_VENDOR_INTEL	1
#define PIKE_CPU_VENDOR_AMD	2
static int cpu_vendor = PIKE_CPU_VENDOR_UNKNOWN;

static enum ia32_reg next_reg;
static enum ia32_reg sp_reg, fp_reg, mark_sp_reg;
ptrdiff_t ia32_prev_stored_pc; /* PROG_PC at the last point Pike_fp->pc was updated. */

void ia32_flush_code_generator(void)
{
  next_reg = REG_EAX;
  sp_reg = fp_reg = mark_sp_reg = REG_NONE;
  CLEAR_REGS();
  ia32_prev_stored_pc = -1;
}

static enum ia32_reg alloc_reg (int avoid_regs)
{
  enum ia32_reg reg;
  int used_regs =
    (avoid_regs | (1 << sp_reg) | (1 << fp_reg) | (1 << mark_sp_reg)) &
    REG_BITMASK;

  if (used_regs != REG_BITMASK) {
    /* There's a free register. */

    for (reg = next_reg; (1 << reg) & used_regs;) {
      reg = (reg + 1) % REG_NONE;
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
      reg = (reg + 1) % REG_NONE;
#ifdef PIKE_DEBUG
      if (reg == next_reg) Pike_fatal ("Failed to find a non-excluded register.\n");
#endif
    }

    if (sp_reg == reg)			{sp_reg = REG_NONE; DEALLOC_REG (reg);}
    else if (fp_reg == reg)		{fp_reg = REG_NONE; DEALLOC_REG (reg);}
    else if (mark_sp_reg == reg)	{mark_sp_reg = REG_NONE; DEALLOC_REG (reg);}
  }

#ifdef REGISTER_DEBUG
  if ((1 << reg) & alloc_regs) Pike_fatal ("Clobbering allocated register.\n");
  alloc_regs &= avoid_regs;
#endif
  ALLOC_REG (reg);
  return reg;
}

#define DEF_LOAD_REG(REG, SET)						\
  static void PIKE_CONCAT(load_,REG) (int avoid_regs)			\
  {									\
    if (REG == REG_NONE) {						\
      REG = alloc_reg (avoid_regs);					\
      /* Update the round robin pointer here so that we disregard */	\
      /* the direct calls to alloc_reg for temporary registers. */	\
      next_reg = (REG + 1) % REG_NONE;					\
      {SET;}								\
    }									\
    else								\
      ALLOC_REG (REG);							\
  }

DEF_LOAD_REG (sp_reg, {
    MOV_ABSADDR_TO_REG (Pike_interpreter.stack_pointer, sp_reg);
  });
DEF_LOAD_REG (fp_reg, {
    MOV_ABSADDR_TO_REG (Pike_interpreter.frame_pointer, fp_reg);
  });
DEF_LOAD_REG (mark_sp_reg, {
    MOV_ABSADDR_TO_REG (Pike_interpreter.mark_stack_pointer, mark_sp_reg);
  });

static void ia32_call_c_function(void *addr)
{
  CALL_RELATIVE(addr);
  next_reg = REG_EAX;
  sp_reg = fp_reg = mark_sp_reg = REG_NONE;
  CLEAR_REGS();
}

/* NOTE: This code is not safe for generic constants, since they
 * can be overridden by inherit. */
static void ia32_push_constant(struct svalue *tmp)
{
  int e;
  if(tmp->type <= MAX_REF_TYPE)
    ADD_VAL_TO_ABSADDR (1, tmp->u.refs);

  load_sp_reg (0);

  for(e=0;e<(int)sizeof(*tmp)/4;e++)
    MOV_VAL_TO_RELADDR (((INT32 *)tmp)[e], e*4, sp_reg);

  ADD_VAL_TO_REG (sizeof(*tmp), sp_reg);
  MOV_REG_TO_ABSADDR (sp_reg, Pike_interpreter.stack_pointer);
}

/* The given register holds the address of the svalue we wish to push. */
static void ia32_push_svalue (enum ia32_reg svalue_ptr_reg)
{
  enum ia32_reg tmp_reg = alloc_reg ((1 << svalue_ptr_reg) | (1 << sp_reg));
  load_sp_reg ((1 << svalue_ptr_reg) | (1 << tmp_reg));

  if(sizeof(struct svalue) > 8)
  {
    size_t e;
    for(e=4;e<sizeof(struct svalue);e += 4)
    {
      if( e ==  OFFSETOF(svalue,u.refs)) continue;
      MOV_RELADDR_TO_REG (e, svalue_ptr_reg, tmp_reg);
      MOV_REG_TO_RELADDR (tmp_reg, e, sp_reg);
    }
  }

  MOV_RELADDR_TO_REG (OFFSETOF (svalue, u.refs), svalue_ptr_reg, tmp_reg);
  MOV_REG_TO_RELADDR (tmp_reg, OFFSETOF (svalue, u.refs), sp_reg);
  /* tmp_reg now holds the pointer if it's a ref type. */

  MOV_RELADDR_TO_REG (0, svalue_ptr_reg, svalue_ptr_reg);
  MOV_REG_TO_RELADDR (svalue_ptr_reg, 0, sp_reg);
  /* svalue_ptr_reg now holds the type and subtype. */

  /* Compare the type which is in the lower 16 bits of svalue_ptr_reg. */
  CHECK_VALID_REG (svalue_ptr_reg);
  add_to_program(0x66);		/* Switch to 16 bit operand mode. */
  add_to_program(0x83);		/* cmp $xx,svalue_ptr_reg */
  add_to_program(0xf8 | svalue_ptr_reg);
  add_to_program(MAX_REF_TYPE);
  DEALLOC_REG (svalue_ptr_reg);

  add_to_program(0x77); /* ja bork */
  add_to_program(0x02);

  CHECK_VALID_REG (tmp_reg);
  add_to_program(0xff); /* incl (tmp_reg) */
  add_to_program(tmp_reg);
  DEALLOC_REG (tmp_reg);

  /* bork: */
  ADD_VAL_TO_REG (sizeof(struct svalue), sp_reg);
  MOV_REG_TO_ABSADDR (sp_reg, Pike_interpreter.stack_pointer);
}

/* A register holding &Pike_fp->locals[arg] is returned. */
static enum ia32_reg ia32_get_local_addr(INT32 arg)
{
  enum ia32_reg result_reg = alloc_reg (1 << fp_reg);
  load_fp_reg (1 << result_reg);

  MOV_RELADDR_TO_REG (OFFSETOF (pike_frame, locals), fp_reg, result_reg);
  ADD_VAL_TO_REG (arg * sizeof (struct svalue), result_reg);
  return result_reg;
}

static void ia32_push_local(INT32 arg)
{
  ia32_push_svalue (ia32_get_local_addr (arg));
}

static void ia32_local_lvalue(INT32 arg)
{
  size_t e;
  struct svalue tmp[2];
  enum ia32_reg addr_reg = ia32_get_local_addr(arg);

  MEMSET(tmp, 0, sizeof(tmp));
  tmp[0].type=T_SVALUE_PTR;
  tmp[1].type=T_VOID;

  load_sp_reg (1 << addr_reg);

  for(e=0;e<sizeof(tmp)/4;e++)
  {
    if(((INT32 *)tmp) + e == (INT32 *) &tmp[0].u.lval)
    {
      MOV_REG_TO_RELADDR (addr_reg, e*4, sp_reg);
    }else{
      MOV_VAL_TO_RELADDR (((INT32 *)tmp)[e], e*4, sp_reg);
    }
  }
  DEALLOC_REG (addr_reg);

  ADD_VAL_TO_REG (sizeof(struct svalue)*2, sp_reg);
  MOV_REG_TO_ABSADDR (sp_reg, Pike_interpreter.stack_pointer);
}

static void ia32_push_global (INT32 arg)
{
  enum ia32_reg tmp_reg = alloc_reg ((1 << fp_reg) | (1 << sp_reg));
  load_fp_reg ((1 << tmp_reg) | (1 << sp_reg));

  MOV_RELADDR16_TO_REG (OFFSETOF (pike_frame, context.identifier_level), fp_reg, tmp_reg);
  ADD_VAL_TO_REG (arg, tmp_reg);
  MOV_REG_TO_RELSTACK (tmp_reg, 8);

  MOV_RELADDR_TO_REG (OFFSETOF (pike_frame, current_object), fp_reg, tmp_reg);
  MOV_REG_TO_RELSTACK (tmp_reg, 4);
  DEALLOC_REG (tmp_reg);

  load_sp_reg (0);
  MOV_REG_TO_RELSTACK (sp_reg, 0);

  ia32_call_c_function (low_object_index_no_free);

  /* Could do an add directly to memory here, but this isn't much
   * slower and it's not unlikely that sp_reg will be used later
   * on. */
  load_sp_reg (0);
  ADD_VAL_TO_REG (sizeof (struct svalue), sp_reg);
  MOV_REG_TO_ABSADDR (sp_reg, Pike_interpreter.stack_pointer);
}

static void ia32_mark(void)
{
  load_mark_sp_reg (1 << sp_reg);
  load_sp_reg (1 << mark_sp_reg);

  MOV_REG_TO_RELADDR (sp_reg, 0, mark_sp_reg);
  ADD_VAL_TO_REG (sizeof (struct svalue *), mark_sp_reg);
  MOV_REG_TO_ABSADDR (mark_sp_reg, Pike_interpreter.mark_stack_pointer);
}

static void ia32_push_int(INT32 x)
{
  struct svalue tmp;
  tmp.type=PIKE_T_INT;
  tmp.subtype=0;
  tmp.u.integer=x;
  ia32_push_constant(&tmp);
}

static void ia32_push_string (INT32 x, int subtype)
{
  size_t e;
  struct svalue tmp = {PIKE_T_STRING, subtype,
#ifdef HAVE_UNION_INIT
		       {0}
#endif
		      };

  enum ia32_reg tmp_reg = alloc_reg ((1 << fp_reg) | (1 << sp_reg));
  load_fp_reg ((1 << tmp_reg) | (1 << sp_reg));

  MOV_RELADDR_TO_REG (OFFSETOF (pike_frame, context.prog), fp_reg, tmp_reg);
  MOV_RELADDR_TO_REG (OFFSETOF (program, strings), tmp_reg, tmp_reg);
  MOV_RELADDR_TO_REG (x * sizeof (struct pike_string *), tmp_reg, tmp_reg);
  /* tmp_reg is now Pike_fp->context.prog->strings[x] */

  load_sp_reg (1 << tmp_reg);

  for (e = 0; e < sizeof (tmp) / 4; e++) {
    if (((INT32 *) &tmp) + e == (INT32 *) &tmp.u.string)
      MOV_REG_TO_RELADDR (tmp_reg, e * 4, sp_reg);
    else
      MOV_VAL_TO_RELADDR (((INT32 *) &tmp)[e], e * 4, sp_reg);
  }

  ADD_VAL_TO_RELADDR (1, 0, tmp_reg);
  DEALLOC_REG (tmp_reg);

  ADD_VAL_TO_REG (sizeof (struct svalue), sp_reg);
  MOV_REG_TO_ABSADDR (sp_reg, Pike_interpreter.stack_pointer);
}

void ia32_update_pc(void)
{
  INT32 tmp = PIKE_PC, disp;

  if (ia32_prev_stored_pc < 0) {
    enum ia32_reg tmp_reg = alloc_reg (1 << fp_reg);
    load_fp_reg (1 << tmp_reg);
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc absolute\n", tmp);
#endif
    /* Store the negated pointer to make the relocation displacements
     * work in the right direction. */
    MOV_VAL32_TO_REG (0, tmp_reg);
    add_to_relocations(PIKE_PC - 4);
    upd_pointer(PIKE_PC - 4, - (INT32) (tmp + Pike_compiler->new_program->program));
    CHECK_VALID_REG (tmp_reg);
    add_to_program(0xf7);		/* neg tmp_reg */
    add_to_program(0xd8 | tmp_reg);
    MOV_REG_TO_RELADDR (tmp_reg, OFFSETOF (pike_frame, pc), fp_reg);
    DEALLOC_REG (tmp_reg);
  }

  else if ((disp = tmp - ia32_prev_stored_pc)) {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc relative: %d\n", tmp, disp);
#endif
    load_fp_reg (0);
    ADD_VAL_TO_RELADDR (disp, OFFSETOF (pike_frame, pc), fp_reg);
  }

  else {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc - already up-to-date\n", tmp);
#endif
  }

  ia32_prev_stored_pc = tmp;
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
     (ia32_prev_stored_pc == -1) ||
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
    MOV_VAL_TO_RELSTACK (arg2, 8);
  if (flags & I_HASARG)
    MOV_VAL_TO_RELSTACK (arg1, 4);
  MOV_VAL_TO_RELSTACK (instr, 0);

  if (flags & I_HASARG2)
    ia32_call_c_function (simple_debug_instr_prologue_2);
  else if (flags & I_HASARG)
    ia32_call_c_function (simple_debug_instr_prologue_1);
  else
    ia32_call_c_function (simple_debug_instr_prologue_0);
}
#else  /* !PIKE_DEBUG */
#define ins_debug_instr_prologue(instr, arg1, arg2)
#endif

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

#ifndef DEBUG_MALLOC
#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  switch(b)
  {
    case F_MARK2 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      ia32_mark();
      return;

    case F_MARK - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      return;

    case F_MARK_AND_CONST0 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      ia32_push_int(0);
      return;

    case F_CONST0 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_push_int(0);
      return;

    case F_MARK_AND_CONST1 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      ia32_push_int(1);
      return;

    case F_CONST1 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_push_int(1);
      return;

    case F_CONST_1 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_push_int(-1);
      return;

    case F_ADD - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      update_arg1(2);
      addr=(void *)f_add;
      break;

    case F_MAKE_ITERATOR - F_OFFSET:
      {
	ins_debug_instr_prologue (b, 0, 0);
	update_arg1(1);
	addr = (void *)f_get_iterator;
      }
      break;
  }
#endif /* !DEBUG_MALLOC */

  ia32_call_c_function(addr);

#ifdef OPCODE_RETURN_JUMPADDR
  if (instrs[b].flags & I_JUMP) {
    /* This is the code that JUMP_EPILOGUE_SIZE compensates for. */
    add_to_program (0xff);	/* jmp *%eax */
    add_to_program (0xe0);
  }
#endif
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  maybe_update_pc();

#ifndef DEBUG_MALLOC
#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  switch(a)
  {
    case F_MARK_AND_LOCAL:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_mark();
      ia32_push_local(b);
      return;

    case F_LOCAL:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_push_local(b);
      return;

    case F_LOCAL_LVALUE:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_local_lvalue(b);
      return;

    case F_MARK_AND_GLOBAL:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_mark();
      ia32_push_global (b);
      return;

    case F_GLOBAL:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_push_global (b);
      return;

    case F_MARK_AND_STRING:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_mark();
      ia32_push_string (b, 0);
      return;

    case F_STRING:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_push_string (b, 0);
      return;

    case F_ARROW_STRING:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_push_string (b, 1);
      return;

    case F_NUMBER:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_push_int(b);
      return;

    case F_NEG_NUMBER:
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      ia32_push_int(-b);
      return;

    case F_CONSTANT:
      /* 
       * This would work nicely for all pike types, but we would
       * have to augment dumping
       *
       * Note: The constants table may contain UNDEFINED in case of being
       *       called through decode_value() in PORTABLE_BYTECODE mode.
       *
       * /grubba 2003-12-11
       */
      if((Pike_compiler->new_program->constants[b].sval.type > MAX_REF_TYPE) &&
	 !Pike_compiler->new_program->constants[b].sval.subtype)
      {
	ins_debug_instr_prologue (a - F_OFFSET, b, 0);
	ia32_push_constant(& Pike_compiler->new_program->constants[b].sval);
	return;
      }
      break;

      /*
       * And this would work nicely for all non-dynamically loaded
       * functions. Unfortunately there is no way to know if it is
       * dynamically loaded or not at this point...
       */
    case F_MARK_CALL_BUILTIN:
      if(Pike_compiler->new_program->constants[b].sval.u.efun->internal_flags & CALLABLE_DYNAMIC)
	break;
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      update_arg1(0);
      ia32_call_c_function(Pike_compiler->new_program->constants[b].sval.u.efun->function);
      return;

    case F_CALL_BUILTIN1:
      if(Pike_compiler->new_program->constants[b].sval.u.efun->internal_flags & CALLABLE_DYNAMIC)
	break;
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      update_arg1(1);
      ia32_call_c_function(Pike_compiler->new_program->constants[b].sval.u.efun->function);
      return;
  }
#endif /* !DEBUG_MALLOC */

  update_arg1(b);
  ins_f_byte(a);
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 b,
			    unsigned INT32 c)
{
  maybe_update_pc();

#ifndef DEBUG_MALLOC
#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  switch(a)
  {
    case F_2_LOCALS:
      ins_debug_instr_prologue (a - F_OFFSET, b, c);
      /* We could optimize this by delaying the sp+=8  -hubbe  */
      ia32_push_local(b);
      ia32_push_local(c);
      return;
  }
#endif /* !DEBUG_MALLOC */

  update_arg1(b);
  update_arg2(c);
  ins_f_byte(a);
}

static INT32 do_ins_jump (unsigned int op, int backward_jump)
{
  INT32 ret = -1;

  if(op == F_BRANCH) {
    ins_debug_instr_prologue (op, 0, 0);
    if (backward_jump) {
      ia32_call_c_function(branch_check_threads_etc);
    }
    add_to_program(0xe9);
    ret=DO_NOT_WARN( (INT32) PIKE_PC );
    PUSH_INT(0);
  }

#ifdef OPCODE_INLINE_BRANCH
  else {
    ia32_call_c_function (instrs[op - F_OFFSET].address);
    add_to_program (0x85);	/* test %eax, %eax */
    add_to_program (0xc0);
    if (backward_jump) {
      add_to_program (0x74);	/* jz rel8 */
      add_to_program (5 + 1 + 4);
      ia32_call_c_function (branch_check_threads_etc); /* 5 bytes */
      add_to_program (0xe9);	/* jmp rel32 */
      ret = DO_NOT_WARN ((INT32) PIKE_PC);
      PUSH_INT (0);		/* 4 bytes */
    }
    else {
      if ((cpu_vendor == PIKE_CPU_VENDOR_AMD) &&
	  (op == F_LOOP || op == F_FOREACH)) {
	/* Slows down Intel PIII by 7%, speeds up Athlon XP by 22%. :P */
	add_to_program (0x3e);	/* Branch taken hint. */
      }
      add_to_program (0x0f);	/* jnz rel32 */
      add_to_program (0x85);
      ret = DO_NOT_WARN ((INT32) PIKE_PC);
      PUSH_INT (0);
    }
  }
#endif

  return ret;
}

INT32 ia32_ins_f_jump (unsigned int op, int backward_jump)
{
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;
  maybe_update_pc();
  return do_ins_jump (op, backward_jump);
}

INT32 ia32_ins_f_jump_with_arg (unsigned int op, unsigned INT32 a,
				int backward_jump)
{
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;
  maybe_update_pc();
  update_arg1 (a);
  return do_ins_jump (op, backward_jump);
}

INT32 ia32_ins_f_jump_with_two_args (unsigned int op,
				     unsigned INT32 a, unsigned INT32 b,
				     int backward_jump)
{
  if (!(instrs[op - F_OFFSET].flags & I_BRANCH)) return -1;
  maybe_update_pc();
  update_arg1 (a);
  update_arg2 (b);
  return do_ins_jump (op, backward_jump);
}

void ia32_update_f_jump(INT32 offset, INT32 to_offset)
{
  upd_pointer(offset, to_offset - offset - 4);
}

INT32 ia32_read_f_jump(INT32 offset)
{
  return read_pointer(offset) + offset + 4;
}

#define addstr(s, l) low_my_binary_strcat((s), (l), buf)
#define adddata2(s,l) addstr((char *)(s),(l) * sizeof((s)[0]));

void ia32_encode_program(struct program *p, struct dynamic_buffer_s *buf)
{
  size_t prev = 0, rel;
  /* De-relocate the program... */
  for (rel = 0; rel < p->num_relocations; rel++)
  {
    size_t off = p->relocations[rel];
    INT32 opcode;
#ifdef PIKE_DEBUG
    if (off < prev) {
      Pike_fatal("Relocations in bad order!\n");
    }
#endif /* PIKE_DEBUG */
    adddata2(p->program + prev, off - prev);

    /* Relocate to 0 */
    opcode = *(INT32 *) (p->program + off);
    opcode += (INT32)p->program;
    adddata2(&opcode, 1);
    prev = off+4;
  }
  adddata2(p->program + prev, p->num_program - prev);
}

void ia32_decode_program(struct program *p)
{
  /* Relocate the program... */
  PIKE_OPCODE_T *prog = p->program;
  INT32 delta = (INT32)prog;
  size_t rel = p->num_relocations;
  while (rel--) {
    *(INT32*)(prog + p->relocations[rel]) -= delta;
  }
}

static size_t ia32_clflush_size = 0;

void ia32_flush_instruction_cache(void *addr, size_t len)
{
  if (ia32_clflush_size) {
    char *ptr;
    char *end_ptr;
    /* We assume even multiple of 2. */
    len += ((size_t)addr) & (ia32_clflush_size-1);
    ptr = (char *)(((size_t)addr) & ~(ia32_clflush_size-1));
    end_ptr = ptr + len;
    while (ptr < end_ptr) {
#ifdef USE_CL_IA32_ASM_STYLE
      __asm {
	__asm clflush ptr
      };
#elif defined(USE_GCC_IA32_ASM_STYLE)
      __asm__ __volatile__("clflush %0" :: "m" (*ptr));
#endif
      ptr += ia32_clflush_size;
    }
  }
}

static void ia32_get_cpuid(int oper, int *cpuid)
{
  static int cpuid_supported = 0;
  if (!cpuid_supported) {
    int fbits;
#ifdef USE_CL_IA32_ASM_STYLE
    __asm {
      __asm pushf
      __asm pop  eax
      __asm mov  ecx, eax
      __asm xor  eax, 00200000h
      __asm push eax
      __asm popf
      __asm pushf
      __asm pop  eax
      __asm xor  ecx, eax
      __asm mov  fbits, ecx
    };
#elif defined(USE_GCC_IA32_ASM_STYLE)
    /* Note: gcc swaps the argument order... */
    __asm__("pushf\n\t"
	    "pop  %%eax\n\t"
	    "movl %%eax, %%ecx\n\t"
	    "xorl $0x00200000, %%eax\n\t"
	    "push %%eax\n\t"
	    "popf\n\t"
	    "pushf\n\t"
	    "pop  %%eax\n\t"
	    "xorl %%eax, %%ecx\n\t"
	    "movl %%ecx, %0"
	    : "=m" (fbits)
	    :
	    : "cc", "eax", "ecx");
#endif
    if (fbits & 0x00200000) {
      cpuid_supported = 1;
    } else {
      cpuid_supported = -1;
    }
  }

  if (cpuid_supported > 0) {
#ifdef USE_CL_IA32_ASM_STYLE
    __asm {
      __asm mov eax, oper
      __asm cpuid
      __asm mov [cpuid], eax
      __asm mov [cpuid+1], ebx
      __asm mov [cpuid+2], edx
      __asm mov [cpuid+3], ecx
    };
#elif defined(USE_GCC_IA32_ASM_STYLE)
    __asm__ __volatile__("cpuid"
			 : "=a" (cpuid[0]),
			   "=b" (cpuid[1]),
			   "=d" (cpuid[2]),
			   "=c" (cpuid[3])
			 : "0" (oper));
#endif
  } else {
    cpuid[0] = cpuid[1] = cpuid[2] = cpuid[3] = 0;
  }
}

void ia32_init_interpreter_state(void)
{
  /* Note: One extra zero for nul-termination. */
  int cpuid[5] = { 0, 0, 0, 0, 0 };

  /* fprintf(stderr, "Calling ia32_get_cpuid()...\n"); */

  ia32_get_cpuid(0, cpuid);

  /* fprintf(stderr, "CPUID: %d, \"%.12s\"\n", cpuid[0], (char *)(cpuid+1)); */

  if (!memcmp(cpuid+1, "AuthenticAMD", 12) &&
      (cpuid[0] > 0)) {
    cpu_vendor = PIKE_CPU_VENDOR_AMD;
    struct amd_info {
      int signature;
      unsigned char brand_id;
      unsigned char clflush_size;
      unsigned char reserved0;
      unsigned char apid_id;
      int standard_features;
      int reserved1;
    } amd_info;
    ia32_get_cpuid(1, &amd_info.signature);
#if 0
    fprintf(stderr,
	    "features: 0x%08x\n"
	    "CLFLUSH size: %d\n",
	    amd_info.standard_features,
	    amd_info.clflush_size);
#endif /* 0 */
    if (amd_info.standard_features & 0x00080000) {
      /* CLFLUSH present. */
      ia32_clflush_size = amd_info.clflush_size;
    }
  } else {
  }
}
