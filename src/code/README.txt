Code generation templates for Pike.

These paired files should all implement the following functions/macros:

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
