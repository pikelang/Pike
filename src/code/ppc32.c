/*
 * $Id: ppc32.c,v 1.1 2001/07/26 21:04:14 marcus Exp $
 *
 * Machine code generator for 32 bit PowerPC
 *
 * Marcus Comstedt 20010726
 */

#include "operators.h"

#define ADD_CALL(X) do {						\
    INT32 delta_;							\
    struct program *p_ = Pike_compiler->new_program;			\
    INT32 off_ = p_->num_program;					\
									\
    /* call X	*/							\
    delta_ = ((PIKE_OPCODE_T *)(X)) - (p_->program + off_);		\
    p_->program[off_] = 0x48000001 | ((delta_<<2) & 0x03fffffc);	\
    add_to_relocations(off_);						\
  } while(0)

void ins_f_byte(unsigned int b)
{
  void *addr;

#ifdef PIKE_DEBUG
  if(store_linenumbers && b<F_MAX_OPCODE)
    ADD_COMPILED(b);
#endif /* PIKE_DEBUG */

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
    
  {
    static int last_prog_id=-1;
    static int last_num_linenumbers=-1;
    if(last_prog_id != Pike_compiler->new_program->id ||
       last_num_linenumbers != Pike_compiler->new_program->num_linenumbers)
    {
      last_prog_id=Pike_compiler->new_program->id;
      last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
      UPDATE_PC();
    }
  }

  addr = instrs[b].address;

#ifndef PIKE_DEBUG
  /* This is not very pretty */
  switch(b)
  {
  case F_MAKE_ITERATOR - F_OFFSET:
    {
      extern void f_Iterator(INT32);
      SET_REG(3, 1);
      addr = (void *)f_Iterator;
    }
    break;
  case F_ADD - F_OFFSET:
    SET_REG(3, 2);
    addr = (void *)f_add;
    break;
  }
#endif
  
  ADD_CALL(addr);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  SET_REG(3, b);
  ins_f_byte(a);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
  SET_REG(3, c);
  SET_REG(4, b);
  ins_f_byte(a);
  return;
}

