/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sparc.c,v 1.15 2002/11/05 19:29:38 grubba Exp $
*/

/*
 * Machine code generator for sparc.
 *
 * Henrik Grubbström 20010720
 */

#include "operators.h"

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
 * Logical operations.
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
#define SPARC_OP3_SLL		0x25
#define SPARC_OP3_SRL		0x26
#define SPARC_OP3_SRA		0x27

#define SPARC_LOG_OP(OP3, D, S1, S2, I)	\
    add_to_program(0x80000000|((D)<<25)|((OP3)<<19)|((S1)<<14)|((I)<<13)| \
		   ((S2)&((I)?0x1fff:0x1f)))

#define SPARC_OR(D,S1,S2,I)	SPARC_LOG_OP(SPARC_OP3_OR, D, S1, S2, I)

#define SPARC_SRA(D,S1,S2,I)	SPARC_LOG_OP(SPARC_OP3_SRA, D, S1, S2, I)

#define SPARC_SETHI(D, VAL) \
    add_to_program(0x01000000|((D)<<25)|(((VAL)>>10)&0x3fffff))

#define SET_REG(REG, X) do {						\
    INT32 val_ = X;							\
    INT32 reg_ = REG;							\
    if ((-4096 <= val_) && (val_ <= 4095)) {				\
      /* or %g0, val_, reg */						\
      SPARC_OR(SPARC_REG_G0, reg_, val_, 1);				\
    } else {								\
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
    }									\
  } while(0)

#define ADD_CALL(X, DELAY_OK) do {					\
    INT32 delta_;							\
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
    /* call X	*/							\
    delta_ = ((PIKE_OPCODE_T *)(X)) - (p_->program + off_);		\
    p_->program[off_] = 0x40000000 | (delta_ & 0x3fffffff);		\
    add_to_relocations(off_);						\
    add_to_program(delay_);						\
  } while(0)

/*
 * Allocate a stack frame.
 *
 * Note that the prologue size must be constant.
 */
void sparc_ins_entry(void)
{
  /* save	%sp, -112, %sp */
  add_to_program(0x81e02000|(SPARC_REG_O6<<25)|
		 (SPARC_REG_O6<<14)|((-112)&0x1fff));
}

/* Update Pike_fp->pc */
void sparc_update_pc(void)
{
  /* call .+8 */
  add_to_program(0x40000002);
  SET_REG(SPARC_REG_O3, ((INT32)(&Pike_interpreter.frame_pointer)));
  /* lduw [ %o3 ], %o3 */
  add_to_program(0xc0000000|(SPARC_REG_O3<<25)|(SPARC_REG_O3<<14));
  /* stw %o7, [ %o3 + pc ] */
  add_to_program(0xc0202000|(SPARC_REG_O7<<25)|(SPARC_REG_O3<<14)|
		 OFFSETOF(pike_frame, pc));
}



static void low_ins_f_byte(unsigned int b, int delay_ok)
{
  void *addr;

#ifdef PIKE_DEBUG
  if(store_linenumbers && b<F_MAX_OPCODE)
    ADD_COMPILED(b);
#endif /* PIKE_DEBUG */

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
    
  {
    static int last_prog_id=-1;
    static int last_num_linenumbers=-1;
    if(last_prog_id != Pike_compiler->new_program->id ||
       last_num_linenumbers != Pike_compiler->new_program->num_linenumbers)
    {
      last_prog_id=Pike_compiler->new_program->id;
      last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
      UPDATE_PC();
      delay_ok = 1;
    }
  }

  addr = instrs[b].address;

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  /* This is not very pretty */
  switch(b)
  {
  case F_MAKE_ITERATOR - F_OFFSET:
    {
      extern void f_Iterator(INT32);
      SET_REG(SPARC_REG_O0, 1);
      delay_ok = 1;
      addr = (void *)f_Iterator;
    }
    break;
  case F_ADD - F_OFFSET:
    SET_REG(SPARC_REG_O0, 2);
    delay_ok = 1;
    addr = (void *)f_add;
    break;
  }

  ADD_CALL(addr, delay_ok);
}

void ins_f_byte(unsigned int opcode)
{
  low_ins_f_byte(opcode, 0);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  SET_REG(SPARC_REG_O0, b);
  low_ins_f_byte(a, 1);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
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
      ((p->program[off] + (((INT32)(p->program)>>2))) & 0x3fffffff);
    adddata2(&opcode, 1);
    prev = off+1;
  }
  adddata2(p->program + prev, p->num_program - prev);
}

void sparc_decode_program(struct program *p)
{
  /* Relocate the program... */
  PIKE_OPCODE_T *prog = p->program;
  INT32 delta = ((INT32)p->program)>>2;
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
