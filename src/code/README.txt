Code generation templates for Pike.

These paired files should all implement the following functions/macros:

PIKE_OPCODE_T
	Type with opcode granularity.

void ins_pointer(INT32 ptr);
	Store a 32bit pointer at the current offset.

INT32 read_pointer(INT32 off);
	Read a 32bit pointer from the specified offset.

void upd_pointer(INT32 off, INT32 ptr);
	Store a 32bit pointer at the specified offset.

void ins_align(INT32 align);
	Align the current offset to the specified alignment.

void ins_byte(INT32 val);
	Insert an 8bit unsigned value at the current offset.

void ins_data(INT32 val);
	Insert a 32bit value at the current offset.

void ins_f_byte(unsigned int op);
	Insert the opcode 'op' at the current offset.

void ins_f_byte_with_arg(unsigned int op, unsigned INT32 arg);
	Insert the opcode 'op' with the primary argument 'arg' at
	the current offset.

void ins_f_byte_with_2_args(unsigned int op,
			    unsigned INT32 arg1,
			    unsigned INT32 arg2);
	Insert the opcode 'op' with the primary argument 'arg1' and
	the secondary argument 'arg2' at the current offset.

void UPDATE_PC(void)
	Insert code to update the runtime linenumber information.

INT32 READ_INCR_BYTE(PIKE_OPCODE_T *pc);
	Return the byte stored at 'pc' by ins_byte(), and increment
	'pc' to the next legal position.

Optional macros:

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

void FLUSH_CODE_GENERATOR_STATE()
        Called at labels and beginning of functions to notify
	the code generator that registers and other states
	must be updated at this point.

        
