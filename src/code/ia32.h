/*
 * $Id: ia32.h,v 1.6 2001/07/21 09:31:23 hubbe Exp $
 */

#define PIKE_OPCODE_T	unsigned INT8

#define ins_pointer(PTR)	ins_int((PTR), (void (*)(char))add_to_program)
#define read_pointer(OFF)	read_int(OFF)
#define upd_pointer(OFF, PTR)	upd_int(OFF, PTR)
#define ins_align(ALIGN)	do {				\
    while(Pike_compiler->new_program->num_program % (ALIGN)) {	\
      add_to_program(0);					\
    }								\
  } while(0)
#define ins_byte(VAL)		add_to_program(VAL)
#define ins_data(VAL)		ins_int((VAL), (void (*)(char))add_to_program)

#define UPDATE_PC() do {						\
    INT32 tmp=PC;							\
    add_to_program(0xa1 /* mov $xxxxx, %eax */);			\
    ins_int((INT32)(&Pike_interpreter.frame_pointer), add_to_program);	\
									\
    add_to_program(0xc7); /* movl $xxxxx, yy%(eax) */			\
    add_to_program(0x40);						\
    add_to_program(OFFSETOF(pike_frame, pc));				\
    ins_int((INT32)tmp, add_to_program);				\
  }while(0)

#define READ_INCR_BYTE(PC)	EXTRACT_UCHAR((PC)++)


/* We know that x86 handles unaligned memory access, we might
 * as well use it.
 */
#define RELOCATE_program(P, NEW)	do {				 \
    PIKE_OPCODE_T *op_ = NEW;						 \
    struct program *p_ = P;						 \
    size_t rel_ = p_->num_relocations;					 \
    INT32 delta_ = p_->program - op_;					 \
    if(delta_) {							 \
      while (rel_--) {							 \
        *((INT32 *)(op_ + p_->relocations[rel_]))+=delta_;		 \
      }									 \
    }									 \
  } while(0)


struct dynamic_buffer_s;
struct program;
void ia32_encode_program(struct program *p, struct dynamic_buffer_s *buf);
void ia32_decode_program(struct program *p);

#define ENCODE_PROGRAM(P, BUF)	ia32_encode_program(P, BUF)
#define DECODE_PROGRAM(P)	ia32_decode_program(p)
