/*
 * $Id: ppc32.h,v 1.4 2001/08/13 23:39:11 mast Exp $
 */

#define PIKE_OPCODE_T	unsigned INT32

#define LOW_GET_JUMP()	(PROG_COUNTER[0])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + 1))
#define PROG_COUNTER (((INT32 **)__builtin_frame_address(1))[2])

#define SET_REG(REG, X) do {						  \
    INT32 val_ = X;							  \
    INT32 reg_ = REG;							  \
    if ((-32768 <= val_) && (val_ <= 32767)) {				  \
      /* addi reg,0,val */						  \
      add_to_program(0x38000000|(reg_<<21)|(val_ & 0xffff));		  \
    } else {								  \
      /* addis reg,0,%hi(val) */					  \
      add_to_program(0x3c000000|(reg_<<21)|((val_ >> 16) & 0xffff));	  \
      if (val_ & 0xffff) {						  \
	/* ori reg,reg,%lo(val) */					  \
	add_to_program(0x60000000|(reg_<<21)|(reg_<<16)|(val_ & 0xffff)); \
      }									  \
    }									  \
  } while(0)

#define UPDATE_PC() do {						\
    INT32 tmp = PIKE_PC;						\
    SET_REG(11, ((INT32)(&Pike_interpreter.frame_pointer)));		\
    /* lwz 11,0(11) */							\
    add_to_program(0x80000000|(11<<21)|(11<<16));			\
    SET_REG(0, tmp);							\
    /* stw 0,pc(11) */							\
    add_to_program(0x90000000|(0<<21)|(11<<16)|				\
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
        if ((op_[p_->relocations[rel_]] & 0xfc000002) !=	\
	    0x48000000) {					\
          fatal("Bad relocation: %d, off:%d, opcode: 0x%08x\n",	\
		rel_, p_->relocations[rel_],			\
		op_[p_->relocations[rel_]]);			\
	}							\
      );							\
      op_[p_->relocations[rel_]] = 0x48000000 |			\
	((op_[p_->relocations[rel_]] + (delta_<<2)) &		\
	 0x03ffffff);						\
    }								\
  } while(0)

extern void ppc32_flush_instruction_cache(void *addr, size_t len);
#define FLUSH_INSTRUCTION_CACHE ppc32_flush_instruction_cache

/*
struct dynamic_buffer_s;

void ppc32_encode_program(struct program *p, struct dynamic_buffer_s *buf);
void ppc32_decode_program(struct program *p);

#define ENCODE_PROGRAM(P, BUF)	ppc32_encode_program(P, BUF)
#define DECODE_PROGRAM(P)	ppc32_decode_program(p)
*/

#define CALL_MACHINE_CODE(pc)    do { if(pc) goto *(pc); } while(0)

