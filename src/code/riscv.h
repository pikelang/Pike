/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifdef __riscv_compressed
#define PIKE_OPCODE_ALIGN	2
#else
#define PIKE_OPCODE_ALIGN	4
#endif
#define OPCODE_RETURN_JUMPADDR
#define OPCODE_INLINE_RETURN
#define OPCODE_INLINE_BRANCH

#define PROG_COUNTER ((PIKE_OPCODE_T *)__builtin_return_address(0))
#ifdef __riscv_compressed
  /* The epilogue instruction can be compressed */
  #define JUMP_EPILOGUE_SIZE 1
#else
  /* No compression available */
  #define JUMP_EPILOGUE_SIZE (1*2)
#endif

#define JUMP_SET_TO_PC_AT_NEXT(PC) \
  ((PC) = PROG_COUNTER + JUMP_EPILOGUE_SIZE)

void riscv_flush_codegen_state(void);
#define FLUSH_CODE_GENERATOR_STATE()	riscv_flush_codegen_state()

void riscv_flush_instruction_cache(void *addr, size_t len);
#define FLUSH_INSTRUCTION_CACHE(ADDR,LEN)         riscv_flush_instruction_cache(ADDR,LEN)

/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
/* The prologue instruction can not be compressed */
#define ENTRY_PROLOGUE_SIZE	(1*2)

void riscv_start_function(int no_pc);
void riscv_end_function(int no_pc);

#define START_NEW_FUNCTION riscv_start_function
#define END_FUNCTION       riscv_end_function

void riscv_ins_entry(void);
#define INS_ENTRY()	riscv_ins_entry()

void riscv_update_pc(void);
#define UPDATE_PC()	riscv_update_pc()

void riscv_ins_int(INT32 n);
INT32 riscv_read_int(const PIKE_OPCODE_T *ptr);
void riscv_upd_int(PIKE_OPCODE_T *ptr, INT32 n);

#define ins_pointer(PTR)	riscv_ins_int((INT32)(PTR))
#define read_pointer(OFF)	riscv_read_int(&Pike_compiler->new_program->program[(INT32)(OFF)])
#define upd_pointer(OFF, PTR)	riscv_upd_int(&Pike_compiler->new_program->program[(INT32)(OFF)], (INT32)(PTR))

#define ins_align(ALIGN)	do {				\
    while(Pike_compiler->new_program->num_program % ((ALIGN)/sizeof(PIKE_OPCODE_T))) { \
      add_to_program(0);					\
    }								\
  } while(0)
#if PIKE_OPCODE_ALIGN == 4
/* Need to preserve alignment here? */
#define ins_byte(VAL)	  do{add_to_program((INT16)(VAL));add_to_program(0);}while(0)
#define READ_INCR_BYTE(PC)	(((PC)+=2)[-2])
#else
#define ins_byte(VAL)	  add_to_program((INT16)(VAL))
#define READ_INCR_BYTE(PC)	(((PC)++)[0])
#endif
#define ins_data(VAL)	  riscv_ins_int((INT32)(VAL))
#define read_program_data(PTR, OFF)	riscv_read_int((PTR) + ((sizeof(INT32)/sizeof(PIKE_OPCODE_T))*(OFF)))

#define LOW_GET_JUMP()	riscv_read_int(&PROG_COUNTER[JUMP_EPILOGUE_SIZE])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + JUMP_EPILOGUE_SIZE + 2))

#define INS_F_JUMP      riscv_ins_f_jump
#define INS_F_JUMP_WITH_ARG      riscv_ins_f_jump_with_arg
#define INS_F_JUMP_WITH_TWO_ARGS      riscv_ins_f_jump_with_2_args
#define UPDATE_F_JUMP   riscv_update_f_jump
#define READ_F_JUMP     riscv_read_f_jump
int riscv_ins_f_jump(unsigned int opcode, int backward_jump);
int riscv_ins_f_jump_with_arg(unsigned int opcode, INT32 arg1, int backward_jump);
int riscv_ins_f_jump_with_2_args(unsigned int opcode, INT32 arg1, INT32 arg2, int backward_jump);
void riscv_update_f_jump(INT32 offset, INT32 to_offset);
int riscv_read_f_jump(INT32 offset);

void riscv_ins_f_byte(unsigned int opcode);
void riscv_ins_f_byte_with_arg(unsigned int a, INT32 b);
void riscv_ins_f_byte_with_2_args(unsigned int a, INT32 c, INT32 b);

#define ins_f_byte(opcode) riscv_ins_f_byte(opcode)
#define ins_f_byte_with_arg(a, b) riscv_ins_f_byte_with_arg(a, b)
#define ins_f_byte_with_2_args(a, b, c) riscv_ins_f_byte_with_2_args(a, b, c)

extern void riscv_jumptable(void);

#define CALL_MACHINE_CODE(PC)   do {                                                    \
    return ((int (*)(struct Pike_interpreter_struct *, void *))(pc)) (Pike_interpreter_pointer, (void*)riscv_jumptable); \
} while(0)

#define DISASSEMBLE_CODE        riscv_disassemble_code

void riscv_disassemble_code(PIKE_OPCODE_T *addr, size_t bytes);

#define INIT_INTERPRETER_STATE		riscv_init_interpreter_state
void riscv_init_interpreter_state(void);
