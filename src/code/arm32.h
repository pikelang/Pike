/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#define PIKE_OPCODE_ALIGN	4
#define OPCODE_RETURN_JUMPADDR
#define OPCODE_INLINE_RETURN

/* FIXME: wtf? */
#define PROG_COUNTER ((PIKE_OPCODE_T *)__builtin_return_address(0))
#define JUMP_EPILOGUE_SIZE 1
#define JUMP_SET_TO_PC_AT_NEXT(PC) \
  ((PC) = PROG_COUNTER + JUMP_EPILOGUE_SIZE)

void arm_flush_codegen_state(void);
#define FLUSH_CODE_GENERATOR_STATE()	arm_flush_codegen_state()

void arm_flush_instruction_cache(void *addr, size_t len);
#define FLUSH_INSTRUCTION_CACHE(ADDR,LEN)         arm_flush_instruction_cache(ADDR,LEN)

/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
#define ENTRY_PROLOGUE_SIZE	4

void arm32_start_function(int no_pc);
void arm32_end_function(int no_pc);

//#define START_NEW_FUNCTION arm32_start_function
//#define END_FUNCTION       arm32_end_function

void arm_ins_entry(void);
#define INS_ENTRY()	arm_ins_entry()

void arm_update_pc(void);
#define UPDATE_PC()	arm_update_pc()

#define ins_pointer(PTR)  add_to_program((INT32)(PTR))
#define read_pointer(OFF) (Pike_compiler->new_program->program[(INT32)(OFF)])
#define upd_pointer(OFF,PTR) (Pike_compiler->new_program->program[(INT32)(OFF)] = (INT32)(PTR))

#define ins_align(ALIGN)
#define ins_byte(VAL)	  add_to_program((INT32)(VAL))
#define ins_data(VAL)	  add_to_program((INT32)(VAL))
#define read_program_data(PTR, OFF)	((INT32)((PTR)[OFF]))

#define LOW_GET_JUMP()	((INT32)PROG_COUNTER[JUMP_EPILOGUE_SIZE])
#define LOW_SKIPJUMP()	(SET_PROG_COUNTER(PROG_COUNTER + JUMP_EPILOGUE_SIZE + 1))

void ins_f_byte(unsigned int opcode);
void ins_f_byte_with_arg(unsigned int a, INT32 b);
void ins_f_byte_with_2_args(unsigned int a, INT32 c, INT32 b);

#define READ_INCR_BYTE(PC)	(((PC)++)[0])
#define CALL_MACHINE_CODE(PC)   do {                                                    \
    return ((int (*)(struct Pike_interpreter_struct *))(pc)) (Pike_interpreter_pointer);	\
} while(0)
