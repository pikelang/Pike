/*
 * $Id: ia32.c,v 1.3 2001/07/20 16:27:27 grubba Exp $
 *
 * Machine code generator for IA32.
 *
 */

void ins_pointer(INT32 ptr)
{
  ins_int(ptr, add_to_program);
}

INT32 read_pointer(INT32 off)
{
  return read_int(off);
}

void upd_pointer(INT32 off, INT32 ptr)
{
  upd_int(off, ptr);
}

void ins_align(INT32 align)
{
  while(Pike_compiler->new_program->num_program % align) {
    add_to_program(0);
  }
}

void ins_byte(INT32 val)
{
  add_to_program(val);
}

void ins_data(INT32 val)
{
  ins_int(val, add_to_program);
}

#define PUSH_INT(X) ins_int((INT32)(X), add_to_program)
#define PUSH_ADDR(X) PUSH_INT((X))

/* This is ugly, but since the code may be moved we cannot use
 * relative addressing :(
 */
#define CALL_ABSOLUTE(X) do{			\
  add_to_program(0xb8);	/* mov $xxx,%eax */	\
  PUSH_INT(X);					\
  add_to_program(0xff);	/* jsr *(%eax) */	\
  add_to_program(0xd0);				\
}while(0)

#define UPDATE_PC() do {						\
    INT32 tmp=PC;							\
    add_to_program(0xa1 /* mov $xxxxx, %eax */);			\
    ins_int((INT32)(&Pike_interpreter.frame_pointer), add_to_program);	\
									\
    add_to_program(0xc7); /* movl $xxxxx, yy%(eax) */			\
    add_to_program(0x40);						\
    add_to_program(OFFSETOF(pike_frame, pc));				\
    ins_int((INT32)tmp, add_to_program);				\
}while(0)

void ins_f_byte(unsigned int b)
{
#ifdef PIKE_DEBUG
  if(store_linenumbers && b<F_MAX_OPCODE)
    ADD_COMPILED(b);
#endif /* PIKE_DEBUG */

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif

  do{
    static int last_prog_id=-1;
    static int last_num_linenumbers=-1;
    if(last_prog_id != Pike_compiler->new_program->id ||
       last_num_linenumbers != Pike_compiler->new_program->num_linenumbers)
    {
      last_prog_id=Pike_compiler->new_program->id;
      last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
      UPDATE_PC();
    }
  }while(0);
  
  CALL_ABSOLUTE(instrs[b].address);
  return;
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  add_to_program(0xc7);  /* movl $xxxx, (%esp) */
  add_to_program(0x04); 
  add_to_program(0x24); 
  PUSH_INT(b);
  ins_f_byte(a);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
  add_to_program(0xc7);  /* movl $xxxx, (%esp) */
  add_to_program(0x04); 
  add_to_program(0x24); 
  PUSH_INT(c);
  add_to_program(0xc7);  /* movl $xxxx, 4(%esp) */
  add_to_program(0x44);
  add_to_program(0x24);
  add_to_program(0x04);
  PUSH_INT(b);
  ins_f_byte(a);
  return;
}

