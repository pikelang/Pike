/*
 * $Id: ppc32.c,v 1.5 2001/08/14 01:26:09 marcus Exp $
 *
 * Machine code generator for 32 bit PowerPC
 *
 * Marcus Comstedt 20010726
 */

#include "operators.h"

#ifdef _AIX
#define ADD_CALL(X) do {						\
    INT32 delta_;							\
    void *toc_, *func_=(X);						\
									\
    __asm__("\tmr %0,"PPC_REGNAME(2) : "=r" (toc_));			\
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

INT32 ppc32_get_local_addr(int reg, INT32 arg)
{
  INT32 offs = arg * sizeof(struct svalue);

  /* lwz 11,frame_pointer(31) */
  add_to_program(0x80000000|(11<<21)|(31<<16)|
		 OFFSETOF(Pike_interpreter, frame_pointer));
  /* lwz reg,locals(11) */
  add_to_program(0x80000000|(reg<<21)|(11<<16)|
		 OFFSETOF(pike_frame, locals));

  if(offs > 32767) {
    /* addis reg,reg,%hi(offs) */
    add_to_program(0x3c000000|(reg<<21)|(reg<<16)|
		   (((offs+32768)>>16)&0xffff));
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  return offs;
}

void ppc32_push_svalue(int reg, INT32 offs)
{
  int e;

  /* lwz 11,stack_pointer(31) */
  add_to_program(0x80000000|(11<<21)|(31<<16)|
		 OFFSETOF(Pike_interpreter, stack_pointer));

  if(sizeof(struct svalue) > 8)
  {
    for(e=4;e<(int)sizeof(struct svalue);e++)
    {
      if( e ==  OFFSETOF(svalue,u.refs)) continue;

      /* lwz 0,e+offs(r) */
      add_to_program(0x80000000|(0<<21)|(reg<<16)|(e+offs));
      /* stw 0,e(11) */
      add_to_program(0x90000000|(0<<21)|(11<<16)|(e));
    }
  }
 
  /* lwz 0,offs(r) */
  add_to_program(0x80000000|(0<<21)|(reg<<16)|(offs));
  /* lwz r,refs+offs(r) */
  add_to_program(0x80000000|(reg<<21)|(reg<<16)|
		 (offs+OFFSETOF(svalue,u.refs)));
  /* stw 0,0(11) */
  add_to_program(0x90000000|(0<<21)|(11<<16)|0);
#if PIKE_BYTEORDER == 1234
  /* rlwinm 0,0,0,16,31 */
  add_to_program(0x54000000|(0<<21)|(0<<16)|(0<<11)|(16<<6)|(31<<1));
#else
  /* rlwinm 0,0,16,16,31 */
  add_to_program(0x54000000|(0<<21)|(0<<16)|(16<<11)|(16<<6)|(31<<1));
#endif
  /* stw r,refs(11) */
  add_to_program(0x90000000|(reg<<21)|(11<<16)|
		 OFFSETOF(svalue,u.refs));
  /* cmplwi 0,7 */
  add_to_program(0x28000000|(0<<16)|(7));
  /* addi 11,11,sizeof(struct svalue) */
  add_to_program(0x38000000|(11<<21)|(11<<16)|(sizeof(struct svalue)));
  /* bgt bork */
  add_to_program(0x40000000|(12<<21)|(1<<16)|(4<<2));
  /* lwz 0,0(r) */
  add_to_program(0x80000000|(0<<21)|(reg<<16)|0);
  /* addic 0,0,1 */
  add_to_program(0x30000000|(0<<21)|(0<<16)|1);
  /* stw 0,0(r) */
  add_to_program(0x90000000|(0<<21)|(reg<<16)|0);
  /* bork: */
  
  /* stw 11,stack_pointer(31) */
  add_to_program(0x90000000|(11<<21)|(31<<16)|
		 OFFSETOF(Pike_interpreter, stack_pointer));
}

void ppc32_push_local(INT32 arg)
{
  INT32 offs;
  offs = ppc32_get_local_addr(3, arg);
  ppc32_push_svalue(3, offs);
}

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
#ifndef PIKE_DEBUG
  switch(a)
  {
   case F_LOCAL:
     ppc32_push_local(b);
     return;
  }
#endif
  SET_REG(3, b);
  ins_f_byte(a);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 b,
			    unsigned INT32 c)
{
#ifndef PIKE_DEBUG
  switch(a)
  {
   case F_2_LOCALS:
     ppc32_push_local(b);
     ppc32_push_local(c);
     return;
  }
#endif
  SET_REG(3, b);
  SET_REG(4, c);
  ins_f_byte(a);
  return;
}

void ppc32_flush_instruction_cache(void *addr, size_t len)
{
  INT32 a;

#ifdef _AIX
  __asm__(".machine \"ppc\"");
#endif
  len >>= 2;
  for(a = (INT32)addr; len>0; --len) {
    __asm__("icbi 0,%0" : : "r" (a));
    __asm__("dcbst 0,%0" : : "r" (a));
    a += 4;
  }
  __asm__("sync");
  __asm__("isync");
}
