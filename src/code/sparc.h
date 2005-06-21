/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sparc.h,v 1.26 2005/06/21 09:58:56 grubba Exp $
*/

#define PIKE_OPCODE_ALIGN	4

#if defined(__arch64__) || defined(__sparcv9)
#define PIKE_BYTECODE_SPARC64
#endif

#define DEF_PROG_COUNTER	register unsigned INT32 *reg_pc __asm__ ("%i7")
#define GLOBAL_DEF_PROG_COUNTER	DEF_PROG_COUNTER
#define PROG_COUNTER		(reg_pc + 2)
#define SET_PROG_COUNTER(X)	(reg_pc = ((unsigned INT32 *)(X))-2)

#define LOW_GET_JUMP()	(PROG_COUNTER[0])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + 1))

/*
 * Code generator state.
 */
extern unsigned INT32 sparc_codegen_state;
extern ptrdiff_t sparc_last_pc;

#define SPARC_CODEGEN_FP_IS_SET			1
#define SPARC_CODEGEN_SP_IS_SET			2
#define SPARC_CODEGEN_IP_IS_SET 		4
#define SPARC_CODEGEN_PC_IS_SET			8
#define SPARC_CODEGEN_MARK_SP_IS_SET		16
#define SPARC_CODEGEN_SP_NEEDS_STORE		32
#define SPARC_CODEGEN_MARK_SP_NEEDS_STORE	64

void sparc_flush_codegen_state(void);
#define FLUSH_CODE_GENERATOR_STATE()	sparc_flush_codegen_state()

/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
#define ENTRY_PROLOGUE_SIZE	1

void sparc_ins_entry(void);
#define INS_ENTRY()	sparc_ins_entry()

void sparc_update_pc(void);
#define UPDATE_PC()	sparc_update_pc()

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
   (ADDR, (LEN)+sizeof(PIKE_OPCODE_T)))

struct dynamic_buffer_s;

void sparc_encode_program(struct program *p, struct dynamic_buffer_s *buf);
void sparc_decode_program(struct program *p);

#define ENCODE_PROGRAM(P, BUF)	sparc_encode_program(P, BUF)
#define DECODE_PROGRAM(P)	sparc_decode_program(p)

void sparc_disassemble_code(void *addr, size_t bytes);
#define DISASSEMBLE_CODE(ADDR, BYTES)	sparc_disassemble_code(ADDR, BYTES)
