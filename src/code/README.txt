Code generation templates for Pike.

These paired files should all implement the following functions/macros:

PIKE_OPCODE_T
	Type with opcode granularity. This is defined in ../program.h.

PIKE_OPCODE_T *PROG_COUNTER;
	Return the current program counter.

void ins_pointer(INT32 ptr);
	Store a 32bit pointer at the current offset.

INT32 read_pointer(INT32 off);
	Read a 32bit pointer from the specified offset.

void upd_pointer(INT32 off, INT32 ptr);
	Store a 32bit pointer at the specified offset.

INT32 LOW_GET_JUMP(void);
	Extract a 32bit pointer from the position following this
	instruction. Note that if OPCODE_RETURN_JUMPADDR is set, the
	value in PROG_COUNTER typically needs to be offset to
	compensate for machine code that is after the opcode function
	call (see JUMP_SET_TO_PC_AT_NEXT).

void LOW_SKIPJUMP(void);
	Advance PROG_COUNTER past a 32bit pointer. Note that if
	OPCODE_RETURN_JUMPADDR is set, the value in PROG_COUNTER
	typically needs to be offset to compensate for machine code
	that is after the opcode function call (see
	JUMP_SET_TO_PC_AT_NEXT).

void ins_align(INT32 align);
	Align the current offset to the specified alignment.

void ins_byte(INT32 val);
	Insert an 8bit unsigned value at the current offset.

void ins_data(INT32 val);
	Insert a 32bit value at the current offset.

INT32 read_data(PIKE_OPCODE_T *origin, int offset)
	Read a data item stored by ins_data. Note that the offset
	is in number of data units.

void ins_f_byte(unsigned int op);
	Insert the opcode 'op' at the current offset.

	Code to update Pike_fp->pc to point to the current offset
	should be inserted first, but it's only necessary if the
	source line or file has changed since the previous opcode.

	Also, if PIKE_DEBUG is defined and the opcode is completely
	inlined, then a call to simple_debug_instr_prologue_0 should
	be inserted before the opcode itself but after the Pike_fp->pc
	update (if there is any).

void ins_f_byte_with_arg(unsigned int op, unsigned INT32 arg);
	Insert the opcode 'op' with the primary argument 'arg' at
	the current offset. See ins_f_byte for further details.

void ins_f_byte_with_2_args(unsigned int op,
			    unsigned INT32 arg1,
			    unsigned INT32 arg2);
	Insert the opcode 'op' with the primary argument 'arg1' and
	the secondary argument 'arg2' at the current offset. See
	ins_f_byte for further details.

void UPDATE_PC(void)
	Insert code to update Pike_fp->pc to the current position.

INT32 READ_INCR_BYTE(PIKE_OPCODE_T *pc);
	Return the byte stored at 'pc' by ins_byte(), and increment
	'pc' to the next legal position.

Optional macros:

void CALL_MACHINE_CODE(PIKE_OPCODE_T *pc)
	Start execution of the machine-code located at 'pc'.
	NOTE: This macro does not return, but instead contains
	code that returns from the calling context. The value
	returned in the macro should be one of -1 (inter return),
	or -2 (inter escape catch).

void EXIT_MACHINE_CODE()
	Clean up from CALL_MACHINE_CODE.

void SET_PROG_COUNTER(PIKE_OPCODE_T *newpc)
	Set PROG_COUNTER to a new value.

GLOBAL_DEF_PROG_COUNTER;
	Declare stuff that is needed for PROG_COUNTER at the global
	level in the interpreter.

DEF_PROG_COUNTER;
	Declare stuff that is needed for PROG_COUNTER in each opcode
	function.

int PIKE_OPCODE_ALIGN;
	Alignment restriction for PIKE_OPCODE_T (debug).

void INS_ENTRY(void)
	Mark the entry point from eval_instruction().
	Useful to add startup code.

int ENTRY_PROLOGUE_SIZE
	Size (in opcodes) of the prologue to be skipped if tail-recursing.

void RELOCATE_program(struct program *p, PIKE_OPCODE_T *new);
	Relocate the copy of 'p'->program at 'new' to be able
	to execute at the new position.

void FLUSH_INSTRUCTION_CACHE(void *addr, size_t len);
	Flush the memory at 'addr' from the instruction cache.

void ENCODE_PROGRAM(struct program *p, struct dynamic_buffer_s *buf);
	Encode 'p'->program in a way accepted by DECODE_PROGRAM().
	NOTE: The encoded data MUST have the length p->num_program *
	      sizeof(PIKE_OPCODE_T).

void DECODE_PROGRAM(struct program *p)
	Decode 'p'->program as encoded by ENCODE_PROGRAM().
	NOTE: 'p'->relocations is valid at this point.

void FLUSH_CODE_GENERATOR_STATE(void)
        Called at labels and beginning of functions to notify
	the code generator that registers and other states
	must be updated at this point.

void ADJUST_PIKE_PC(PIKE_OPCODE_T *pc)
	Called after opcodes that modify Pike_fp->pc. The passed
	argument is the pc they will put there. Useful when UPDATE_PC
	inserts code that update Pike_fp->pc relatively. (Note: Not
	used anymore.)

int ALIGN_PIKE_JUMPS
        This can be defined to a number which will cause Pike to
	insert zeroes in the code after instructions which do not
        return to permit better alignment of code. Please note that
        this is not guaranteed and should only be used for optimization.

int ALIGN_PIKE_FUNCTION_BEGINNINGS
	Similar to ALIGN_PIKE_JUMPS, but only for the beginning
	of a function.

int INS_F_JUMP(unsigned int op, int backward_jump)
	Similar to ins_f_byte, but is only called for jump and branch
	instructions that take a constant target address. The return
	value should be the offset in the program to the empty address
	field of the jump instruction, which will be filled in by
	UPDATE_F_JUMP. You can also return -1 to make the code use the
	same behaviour as if INS_F_JUMP was not defined.

	backward_jump is only relevant for branch opcodes if
	OPCODE_INLINE_BRANCH is defined. If it's set, a call to
	branch_check_threads_etc should be compiled in whenever the
	branch is taken.

int INS_F_JUMP_WITH_ARG(unsigned int op, unsigned INT32 arg, int backward_jump)
	Similar to INS_F_JUMP(), but called for instructions that take
	one parameter.

int INS_F_JUMP_WITH_TWO_ARGS(unsigned int op,
			     unsigned INT32 arg1,
			     unsigned INT32 arg2,
			     int backward_jump)
	Similar to INS_F_JUMP(), but called for instructions that take
	two parameters.

void UPDATE_F_JUMP(INT32 offset, INT32 to_offset)
	If you define any of the INS_F_JUMP* functions you must also
	define this one. It's called when the compiler knows where to
	jump. (See ia32.c for an example of this and INS_F_JUMP.)

INT32 READ_F_JUMP(INT32 offset)
	If you define any of the INS_F_JUMP* functions you must also
	define this one. It's called when the compiler needs to read
	back the to_offset that was passed to UPDATE_F_JUMP.

OPCODE_INLINE_BRANCH
	If defined, test functions that return nonzero when the branch
	is to be taken will be generated for I_BRANCH instructions.
	The machine code generated by INS_F_JUMP* must test the return
	value for those opcodes and jump iff it's nonzero. This is to
	facilitate easier inlining of branches in the machine code.

OPCODE_RETURN_JUMPADDR
	If defined, jump functions that return the address to jump to
	will be generated for I_JUMP instructions, so the ins_f_byte*
	must generate machine code that (unconditionally) jumps to the
	return value for those opcodes. If this isn't defined, they
	will instead use SET_PROG_COUNTER to change the address they
	return to. This macro allows faster code on cpus where setting
	the return address wreaks havoc in the instruction pipelines.

JUMP_SET_TO_PC_AT_NEXT(PIKE_OPCODE_T *PC)
	Used in I_JUMP opcodes to store the pc to the next
	instruction, to compensate for any machine code that is
	inserted after the call. PC is the lvalue where it should be
	stored. Must be defined if OPCODE_RETURN_JUMPADDR is.

void CHECK_RELOC(size_t reloc, size_t program_size)
	Check if a relocation is valid for the program.
	Should throw an error on bad relocations.

void DISASSEMBLE_CODE(void *adrr, size_t bytes)
	Debug function that dumps the generated code on stderr.
