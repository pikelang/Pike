/*
 * $Id: sparc.h,v 1.2 2001/07/20 13:16:50 grubba Exp $
 */

#define REG_O0	8
#define REG_O1	9
#define REG_O2	10
#define REG_O3	11
#define REG_O4	12
#define REG_O5	13
#define REG_O6	14
#define REG_O7	15

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

#define UPDATE_PC() do {						\
    INT32 tmp = PC;							\
    SET_REG(REG_O3, ((INT32)(&Pike_interpreter.frame_pointer)));	\
    /* lduw %o3, %o3 */							\
    add_to_program(0xc0000000|(REG_O3<<25)|(REG_O3<<14));		\
    SET_REG(REG_O4, tmp);						\
    /* stw %o4, yy(%o3) */						\
    add_to_program(0xc0202000|(REG_O4<<25)|(REG_O3<<14)|		\
		   OFFSETOF(pike_frame, pc));				\
  } while(0)

#define ins_pointer(PTR)  add_to_program((INT32)(PTR))
#define read_pointer(OFF) (Pike_compiler->new_program->program[(INT32)(OFF)])
#define upd_pointer(OFF,PTR) (Pike_compiler->new_program->program[(INT32)(OFF)] = (INT32)(PTR))
#define ins_align(ALIGN)
#define ins_byte(VAL)	  add_to_program((INT32)(VAL))
#define ins_data(VAL)	  add_to_program((INT32)(VAL))
