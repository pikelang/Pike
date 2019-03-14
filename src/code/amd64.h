
#define OPCODE_INLINE_BRANCH
#define OPCODE_RETURN_JUMPADDR
#define OPCODE_INLINE_RETURN

#if defined(_M_X64) && !defined(__GNUC__)

#define DEF_PROG_COUNTER	void *amd64_pc; \
				_asm {	_asm mov amd64_pc, rbp }
#define PROG_COUNTER		(((unsigned char **)amd64_pc)[1])

#else /* _M_X64_ && !__GNUC__ */

#ifdef OPCODE_RETURN_JUMPADDR
/* Don't need an lvalue in this case. */
#define PROG_COUNTER ((unsigned char *)__builtin_return_address(0))
#else
#define PROG_COUNTER (((unsigned char **)__builtin_frame_address(0))[1])
#endif

#endif

#define CALL_MACHINE_CODE(pc)						\
  do {									\
    ((int (*)(struct Pike_interpreter_struct *))(pc)) (Pike_interpreter_pointer);	\
  } while(0)


void amd64_start_function(int no_pc);
void amd64_end_function(int no_pc);

#define START_NEW_FUNCTION amd64_start_function
#define END_FUNCTION       amd64_end_function

void amd64_ins_entry(void);
#define INS_ENTRY()	amd64_ins_entry()
/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
#define ENTRY_PROLOGUE_SIZE	0x14

void amd64_flush_code_generator_state(void);
#define FLUSH_CODE_GENERATOR_STATE()	amd64_flush_code_generator_state()

int amd64_ins_f_jump(unsigned int op, int backward_jump);
int amd64_ins_f_jump_with_arg(unsigned int op, INT32 a, int backward_jump);
int amd64_ins_f_jump_with_2_args(unsigned int op, INT32 a, INT32 b,
				 int backward_jump);
void amd64_update_f_jump(INT32 offset, INT32 to_offset);
INT32 amd64_read_f_jump(INT32 offset);
#define INS_F_JUMP			amd64_ins_f_jump
#define INS_F_JUMP_WITH_ARG		amd64_ins_f_jump_with_arg
#define INS_F_JUMP_WITH_TWO_ARGS	amd64_ins_f_jump_with_2_args
#define UPDATE_F_JUMP			amd64_update_f_jump
#define READ_F_JUMP			amd64_read_f_jump

void amd64_init_interpreter_state(void);
#define INIT_INTERPRETER_STATE		amd64_init_interpreter_state

#ifdef OPCODE_RETURN_JUMPADDR
/* Adjust for the machine code inserted after the call for I_JUMP opcodes. */
#define JUMP_EPILOGUE_SIZE 2
#define JUMP_SET_TO_PC_AT_NEXT(PC) \
  ((PC) = PROG_COUNTER + JUMP_EPILOGUE_SIZE)
#else
#define JUMP_EPILOGUE_SIZE 0
#endif

#define LOW_GET_JUMP()							\
  EXTRACT_INT(PROG_COUNTER + JUMP_EPILOGUE_SIZE)
#define LOW_SKIPJUMP()							\
  (SET_PROG_COUNTER(PROG_COUNTER + JUMP_EPILOGUE_SIZE + sizeof(INT32)))


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
#define read_program_data(PTR, OFF)	EXTRACT_INT((PTR) + (sizeof(INT32)*(OFF)))

void amd64_update_pc(void);

#define UPDATE_PC() 		amd64_update_pc()

extern ptrdiff_t amd64_prev_stored_pc;

#define READ_INCR_BYTE(PC)	EXTRACT_UCHAR((PC)++)



