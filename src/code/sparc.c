/*
 * $Id: sparc.c,v 1.1 2001/07/20 12:44:55 grubba Exp $
 *
 * Machine code generator for sparc.
 *
 * Henrik Grubbström 20010720
 */

void ins_pointer(INT32 ptr)
{
  add_to_program(ptr);
}

INT32 read_pointer(INT32 off)
{
  return Pike_compiler->new_program->program[off];
}

void upd_pointer(INT32 off, INT32 ptr)
{
  Pike_compiler->new_program->program[off] = ptr;
}

void ins_align(INT32 align)
{
  /* Already aligned... */
}

void ins_byte(INT32 val)
{
  add_to_program(val);
}

void ins_data(INT32 val)
{
  add_to_program(val);
}


#define CALL_ABSOLUTE(X) do {						\
    INT32 delta_;							\
    struct program *p_ = Pike_compiler->new_program;			\
    INT32 off_ = p_->num_program;					\
    /* call X	*/							\
    add_to_program(0); /* Placeholder... */				\
    delta_ = ((PIKE_OPCODE_T *)(X)) - (p_->program + off_);		\
    p_->program[off_] = 0x40000000 | (delta_ & 0x3fffffff);		\
    add_to_relocations(off_);						\
    /* noop		*/						\
    add_to_program(0x01000000);						\
  } while(0)

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
  SET_REG(REG_O0, b);
  ins_f_byte(a);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
  SET_REG(REG_O0, c);
  SET_REG(REG_O1, b);
  ins_f_byte(a);
  return;
}

