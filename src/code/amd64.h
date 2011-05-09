
#define OPCODE_INLINE_BRANCH
#define OPCODE_RETURN_JUMPADDR

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

#define CALL_MACHINE_CODE(pc)					\
  do {								\
    /* The test is needed to get the labels to work... */	\
    if (pc) {							\
      return ((int (*)(struct Pike_interpreter *))(pc))		\
	(&Pike_interpreter);					\
    }								\
  } while(0)

void amd64_ins_entry(void);
#define INS_ENTRY()	amd64_ins_entry()
/* Size of the prologue added by INS_ENTRY() (in PIKE_OPCODE_T's). */
#define ENTRY_PROLOGUE_SIZE	0x23

#if 0
int ins_f_jump(unsigned int op, int backward_jump);
int ins_f_jump_with_arg(unsigned int op, unsigned INT32 arg,
			int backward_jump);
int ins_f_jump_with_two_args(unsigned int op, unsigned INT32 arg1,
			     unsigned INT32 arg2, int backward_jump);
#endif

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



