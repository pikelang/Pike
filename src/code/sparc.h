/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: sparc.h,v 1.14 2002/10/08 20:22:28 nilsson Exp $
\*/

#define PIKE_OPCODE_ALIGN	4

#define LOW_GET_JUMP()	(PROG_COUNTER[0])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + 1))
#define DEF_PROG_COUNTER	register unsigned INT32 *reg_pc __asm__ ("%i7")
#define PROG_COUNTER		(reg_pc + 2)
#define SET_PROG_COUNTER(X)	(reg_pc = ((unsigned INT32 *)(X))-2)


#define SPARC_REG_O0	8
#define SPARC_REG_O1	9
#define SPARC_REG_O2	10
#define SPARC_REG_O3	11
#define SPARC_REG_O4	12
#define SPARC_REG_O5	13
#define SPARC_REG_O6	14
#define SPARC_REG_O7	15
#define SPARC_REG_L0	16
#define SPARC_REG_L1	17
#define SPARC_REG_L2	18
#define SPARC_REG_L3	19
#define SPARC_REG_L4	20
#define SPARC_REG_L5	21
#define SPARC_REG_L6	22
#define SPARC_REG_L7	23

#define SET_REG(REG, X) do {						\
    INT32 val_ = X;							\
    INT32 reg_ = REG;							\
    if ((-4096 <= val_) && (val_ <= 4095)) {				\
      /* or %g0, val_, reg */						\
      add_to_program(0x80102000|(reg_<<25)|(val_ & 0x1fff));		\
    } else {								\
      /* sethi %hi(val_), reg */					\
      add_to_program(0x01000000|(reg_<<25)|((val_ >> 10)&0x3fffff));	\
      if (val_ & 0x3ff) {						\
	/* or reg, %lo(val_), reg */					\
	add_to_program(0x80102000|(reg_<<25)|(reg_<<14)|(val_ & 0x3ff)); \
      }									\
      if (val_ < 0) {							\
	/* sra reg, %g0, reg */						\
	add_to_program(0x81380000|(reg_<<25)|(reg_<<14));		\
      }									\
    }									\
  } while(0)

/*
 * Allocate a stack frame.
 *
 * Note that the prologue size must be constant.
 */
#define INS_ENTRY()	do {						\
    /* save	%sp, -112, %sp */					\
    add_to_program(0x81e02000|(SPARC_REG_O6<<25)|			\
		   (SPARC_REG_O6<<14)|((-112)&0x1fff));			\
  } while(0)

#define ENTRY_PROLOGUE_SIZE	1

#define UPDATE_PC() do {						\
    /* call .+8 */							\
    add_to_program(0x40000002);						\
    SET_REG(SPARC_REG_O3, ((INT32)(&Pike_interpreter.frame_pointer)));	\
    /* lduw [ %o3 ], %o3 */						\
    add_to_program(0xc0000000|(SPARC_REG_O3<<25)|(SPARC_REG_O3<<14));	\
    /* stw %o7, [ %o3 + pc ] */						\
    add_to_program(0xc0202000|(SPARC_REG_O7<<25)|(SPARC_REG_O3<<14)|	\
		   OFFSETOF(pike_frame, pc));				\
  } while(0)

#define ins_pointer(PTR)  add_to_program((INT32)(PTR))
#define read_pointer(OFF) (Pike_compiler->new_program->program[(INT32)(OFF)])
#define upd_pointer(OFF,PTR) (Pike_compiler->new_program->program[(INT32)(OFF)] = (INT32)(PTR))
#define ins_align(ALIGN)
#define ins_byte(VAL)	  add_to_program((INT32)(VAL))
#define ins_data(VAL)	  add_to_program((INT32)(VAL))

#define READ_INCR_BYTE(PC)	(((PC)++)[0])

#define RELOCATE_program(P, NEW)	do {			\
    PIKE_OPCODE_T *op_ = NEW;					\
    struct program *p_ = P;					\
    size_t rel_ = p_->num_relocations;				\
    INT32 delta_ = p_->program - op_;				\
    while (rel_--) {						\
      DO_IF_DEBUG(						\
        if ((op_[p_->relocations[rel_]] & 0xc0000000) !=	\
	    0x40000000) {					\
          Pike_fatal("Bad relocation: %d, off:%d, opcode: 0x%08x\n",	\
		rel_, p_->relocations[rel_],			\
		op_[p_->relocations[rel_]]);			\
	}							\
      );							\
      op_[p_->relocations[rel_]] = 0x40000000|			\
	(((op_[p_->relocations[rel_]] & 0x3fffffff) + delta_) &	\
	 0x3fffffff);						\
    }								\
  } while(0)

extern const unsigned INT32 sparc_flush_instruction_cache[];
#define FLUSH_INSTRUCTION_CACHE(ADDR, LEN)			\
  (((void (*)(void *,size_t))sparc_flush_instruction_cache)	\
    (ADDR, (LEN)*sizeof(PIKE_OPCODE_T)))

struct dynamic_buffer_s;

void sparc_encode_program(struct program *p, struct dynamic_buffer_s *buf);
void sparc_decode_program(struct program *p);

#define ENCODE_PROGRAM(P, BUF)	sparc_encode_program(P, BUF)
#define DECODE_PROGRAM(P)	sparc_decode_program(p)
