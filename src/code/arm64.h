/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#define PIKE_OPCODE_ALIGN	4
#define OPCODE_RETURN_JUMPADDR
#define OPCODE_INLINE_RETURN
#define OPCODE_INLINE_BRANCH

/* FIXME: wtf? */
#define PROG_COUNTER ((PIKE_OPCODE_T *)__builtin_return_address(0))
#define JUMP_EPILOGUE_SIZE 1
#define JUMP_SET_TO_PC_AT_NEXT(PC) \
  ((PC) = PROG_COUNTER + JUMP_EPILOGUE_SIZE)

void arm64_flush_codegen_state(void);
#define FLUSH_CODE_GENERATOR_STATE()	arm64_flush_codegen_state()

void arm64_flush_instruction_cache(void *addr, size_t len);
#define FLUSH_INSTRUCTION_CACHE(ADDR,LEN)         arm64_flush_instruction_cache(ADDR,LEN)

/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
#define ENTRY_PROLOGUE_SIZE	5

void arm64_start_function(int no_pc);
void arm64_end_function(int no_pc);

#define START_NEW_FUNCTION arm64_start_function
#define END_FUNCTION       arm64_end_function

void arm64_ins_entry(void);
#define INS_ENTRY()	arm64_ins_entry()

void arm64_update_pc(void);
#define UPDATE_PC()	arm64_update_pc()

#ifdef HAVE_PTHREAD_JIT_WRITE_PROTECT_NP
#define BEFORE_CODE_WRITE() pthread_jit_write_protect_np(0)
#define AFTER_CODE_WRITE() pthread_jit_write_protect_np(1)
#else
#define BEFORE_CODE_WRITE() do{}while(0)
#define AFTER_CODE_WRITE() do{}while(0)
#endif

#define ins_pointer(PTR)  add_to_program((INT32)(PTR))
#define read_pointer(OFF) (Pike_compiler->new_program->program[(INT32)(OFF)])
#define upd_pointer(OFF,PTR) do{BEFORE_CODE_WRITE();(Pike_compiler->new_program->program[(INT32)(OFF)] = (INT32)(PTR));AFTER_CODE_WRITE();}while(0)

#define ins_align(ALIGN)
#define ins_byte(VAL)	  add_to_program((INT32)(VAL))
#define ins_data(VAL)	  add_to_program((INT32)(VAL))
#define read_program_data(PTR, OFF)	((INT32)((PTR)[OFF]))

#define LOW_GET_JUMP()	((INT32)PROG_COUNTER[JUMP_EPILOGUE_SIZE])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + JUMP_EPILOGUE_SIZE + 1))

#define INS_F_JUMP      arm64_ins_f_jump
#define INS_F_JUMP_WITH_ARG      arm64_ins_f_jump_with_arg
#define INS_F_JUMP_WITH_TWO_ARGS      arm64_ins_f_jump_with_2_args
#define UPDATE_F_JUMP   arm64_update_f_jump
#define READ_F_JUMP     arm64_read_f_jump
int arm64_ins_f_jump(unsigned int opcode, int backward_jump);
int arm64_ins_f_jump_with_arg(unsigned int opcode, INT32 arg1, int backward_jump);
int arm64_ins_f_jump_with_2_args(unsigned int opcode, INT32 arg1, INT32 arg2, int backward_jump);
void arm64_update_f_jump(INT32 offset, INT32 to_offset);
int arm64_read_f_jump(INT32 offset);

void ins_f_byte(unsigned int opcode);
void ins_f_byte_with_arg(unsigned int a, INT32 b);
void ins_f_byte_with_2_args(unsigned int a, INT32 c, INT32 b);

#define READ_INCR_BYTE(PC)	(((PC)++)[0])
#define CALL_MACHINE_CODE(PC)   do {                                                    \
    return ((int (*)(struct Pike_interpreter_struct *))(pc)) (Pike_interpreter_pointer);	\
} while(0)

#define DISASSEMBLE_CODE        arm64_disassemble_code

void arm64_disassemble_code(PIKE_OPCODE_T *addr, size_t bytes);

#define INIT_INTERPRETER_STATE		arm64_init_interpreter_state
void arm64_init_interpreter_state(void);
