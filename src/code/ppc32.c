/*
 * $Id: ppc32.c,v 1.3 2001/07/30 22:29:19 marcus Exp $
 *
 * Machine code generator for 32 bit PowerPC
 *
 * Marcus Comstedt 20010726
 */

#include "operators.h"

#if 0
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
#else

#ifdef _AIX
#define ADD_CALL(X) do {						\
    INT32 delta_;							\
    void *toc_, *func_=(X);						\
									\
    __asm__("\tmr %0,2" : "=r" (toc_));				\
    delta_ = ((char *)func_) - ((char *)toc_);				\
    if(delta_ < -32768 || delta_ > 32767)				\
      fatal("Function pointer %p out of range for TOC @ %p!\n",		\
	    func_, toc_);						\
    /* lwz r0,delta(r2)	*/						\
    add_to_program(0x80000000|(2<<16)|(delta_&0xffff));			\
    /* mtlr r0 */							\
    add_to_program(0x7c0803a6);						\
    /* blrl */								\
    add_to_program(0x4e800021);						\
  } while(0)
#else
#define ADD_CALL(X) do {						\
    INT32 func_=(INT32)(void*)(X);					\
									\
    if(func_ < -33554432 || func_ > 33554431)				\
      fatal("Function pointer %p out of range absolute addressing!\n",	\
	    func_);							\
    /* bla func	*/							\
    add_to_program(0x48000003|(func_&0x03fffffc));			\
  } while(0)
#endif
#endif

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

void ppc32_flush_instruction_cache(void *addr, size_t len)
{
  INT32 a = (INT32)addr;
#ifdef _AIX
  __asm__(".machine \"ppc\"");
#endif
  len >>= 2;
  while(len>0) {
    __asm__("dcbst 0,%0" : : "r" (a));
    __asm__("icbi 0,%0" : : "r" (a));
    a += 4;
    --len;
  }
  __asm__("sync");
}
