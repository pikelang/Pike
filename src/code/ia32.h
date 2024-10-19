/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/* #define ALIGN_PIKE_JUMPS 8 */

#include "pike_cpulib.h"
#include "pike_memory.h"

#define OPCODE_INLINE_BRANCH
#define OPCODE_RETURN_JUMPADDR

#if !defined (CL_IA32_ASM_STYLE) && !defined (GCC_IA32_ASM_STYLE)
#error Dont know how to inline assembler with this compiler
#endif

#ifdef CL_IA32_ASM_STYLE

#define DEF_PROG_COUNTER void *ia32_pc; \
                         _asm { _asm mov ia32_pc,ebp }
#define PROG_COUNTER  (((unsigned char **)ia32_pc)[1])

#else  /* GCC_IA32_ASM_STYLE */

#define OPCODE_INLINE_RETURN
void ia32_ins_entry(void);
#define INS_ENTRY()	ia32_ins_entry()
/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
#define ENTRY_PROLOGUE_SIZE	0x09

#ifdef OPCODE_RETURN_JUMPADDR
/* Don't need an lvalue in this case. */
#define PROG_COUNTER ((unsigned char *)__builtin_return_address(0))
#else
#error This method to tweak the jump address does not work with gcc >= 4.x
#define PROG_COUNTER (((unsigned char **)__builtin_frame_address(0))[1])
#endif

#endif	/* GCC_IA32_ASM_STYLE */

#ifdef OPCODE_RETURN_JUMPADDR
/* Adjust for the machine code inserted after the call for I_JUMP opcodes. */
#define JUMP_EPILOGUE_SIZE 2
#define JUMP_SET_TO_PC_AT_NEXT(PC) \
  ((PC) = PROG_COUNTER + JUMP_EPILOGUE_SIZE)
#else
#define JUMP_EPILOGUE_SIZE 0
#endif

#define LOW_GET_JUMP()							\
  (INT32)get_unaligned32(PROG_COUNTER + JUMP_EPILOGUE_SIZE)
#define LOW_SKIPJUMP()							\
  (SET_PROG_COUNTER(PROG_COUNTER + JUMP_EPILOGUE_SIZE + sizeof(INT32)))


#define ins_pointer(PTR)	ins_int((PTR), add_to_program)
#define read_pointer(OFF)	read_int(OFF)
#define upd_pointer(OFF, PTR)	upd_int(OFF, PTR)
#define ins_align(ALIGN)	do {				\
    while(Pike_compiler->new_program->num_program % (ALIGN)) {	\
      add_to_program(0);					\
    }								\
  } while(0)
#define ins_byte(VAL)		add_to_program(VAL)
#define ins_data(VAL)		ins_int((VAL), add_to_program)
#define read_program_data(PTR, OFF)	\
  (INT32)get_unaligned32((PTR) + (((ptrdiff_t)sizeof(INT32)) * (OFF)))

void ia32_update_pc(void);

#define UPDATE_PC() ia32_update_pc()

extern ptrdiff_t ia32_prev_stored_pc;

#define ADJUST_PIKE_PC(pc) do {						\
    ia32_prev_stored_pc = pc;						\
    DO_IF_DEBUG(							\
      if (a_flag >= 60)							\
	fprintf (stderr, "pc %d  adjusted\n", ia32_prev_stored_pc);	\
    );									\
  } while (0)


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


struct byte_buffer;
struct program;
void ia32_encode_program(struct program *p, struct byte_buffer *buf);
void ia32_decode_program(struct program *p);

#define ENCODE_PROGRAM(P, BUF)	ia32_encode_program(P, BUF)
#define DECODE_PROGRAM(P)	ia32_decode_program(p)

INT32 ia32_ins_f_jump(unsigned int op, int backward_jump);
INT32 ia32_ins_f_jump_with_arg(unsigned int op, unsigned INT32 a, int backward_jump);
INT32 ia32_ins_f_jump_with_two_args(unsigned int op,
				    unsigned INT32 a, unsigned INT32 b,
				    int backward_jump);
void ia32_update_f_jump(INT32 offset, INT32 to_offset);
INT32 ia32_read_f_jump(INT32 offset);

#define INS_F_JUMP ia32_ins_f_jump
#define INS_F_JUMP_WITH_ARG ia32_ins_f_jump_with_arg
#define INS_F_JUMP_WITH_TWO_ARGS ia32_ins_f_jump_with_two_args
#define UPDATE_F_JUMP ia32_update_f_jump
#define READ_F_JUMP ia32_read_f_jump

#if 0
/* This is apparently not necessary. */
void ia32_flush_instruction_cache(void *start, size_t len);
#define FLUSH_INSTRUCTION_CACHE ia32_flush_instruction_cache
void ia32_init_interpreter_state(void);
#define INIT_INTERPRETER_STATE ia32_init_interpreter_state
#endif

void ia32_flush_code_generator(void);
#define FLUSH_CODE_GENERATOR_STATE ia32_flush_code_generator

void ia32_flush_instruction_cache(void *addr, size_t len);
#define FLUSH_INSTRUCTION_CACHE	ia32_flush_instruction_cache

void ia32_init_interpreter_state(void);
#define INIT_INTERPRETER_STATE	ia32_init_interpreter_state

#ifdef INS_ENTRY

#define CALL_MACHINE_CODE(pc)                                           \
  do {                                                                  \
    ((int (*)(void))(pc)) ();						\
  } while(0)

#else /* !INS_ENTRY */

#ifdef CL_IA32_ASM_STYLE

#define CALL_MACHINE_CODE(pc)                                   \
  __asm {                                                       \
    __asm sub esp,12                                            \
    __asm inc ebx /* dummy: forces the compiler to save ebx */  \
    __asm jmp pc                                                \
  }

#define EXIT_MACHINE_CODE()                                     \
  __asm { __asm add esp,12 }

#else  /* GCC_IA32_ASM_STYLE */

#define CALL_MACHINE_CODE(pc)						\
  /* This code does not clobber %eax, %ebx, %ecx & %edx, but		\
   * the code jumped to does.						\
   */									\
  __asm__ __volatile__( "	sub $16,%%esp\n"			\
			"	jmp *%0"				\
			: "=m" (pc)					\
			:						\
			: "cc", "memory", "eax", "ebx", "ecx", "edx" )

#define EXIT_MACHINE_CODE()						\
  __asm__ __volatile__( "add $16,%%esp\n" : : )

#endif /* INS_ENTRY */

#endif /* GCC_IA32_ASM_STYLE */
