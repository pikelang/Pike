/*
 * $Id: ppc32.c,v 1.9 2001/08/16 01:06:49 marcus Exp $
 *
 * Machine code generator for 32 bit PowerPC
 *
 * Marcus Comstedt 20010726
 */

#include "operators.h"
#include "constants.h"

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
    LWZ(0, 2, delta_);							\
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

int ppc32_codegen_state = 0;

void ppc32_flush_code_generator_state()
{
  if(ppc32_codegen_state & PPC_CODEGEN_SP_NEEDSSTORE) {
    /* stw pike_sp,stack_pointer(pike_interpreter) */
    STW(PPC_REG_PIKE_SP, PPC_REG_PIKE_INTERP,
	OFFSETOF(Pike_interpreter, stack_pointer));
  }
  if(ppc32_codegen_state & PPC_CODEGEN_MARK_SP_NEEDSSTORE) {
    /* stw pike_mark_sp,mark_stack_pointer(pike_interpreter) */
    STW(PPC_REG_PIKE_MARK_SP, PPC_REG_PIKE_INTERP,
	OFFSETOF(Pike_interpreter, mark_stack_pointer));
  }
  ppc32_codegen_state = 0;
}

INT32 ppc32_get_local_addr(int reg, INT32 arg)
{
  INT32 offs = arg * sizeof(struct svalue);

  LOAD_FP_REG();
  /* lwz reg,locals(pike_fp) */
  LWZ(reg, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, locals));

  if(offs > 32767) {
    /* addis reg,reg,%hi(offs) */
    ADDIS(reg, reg, (offs+32768)>>16);
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  return offs;
}

void ppc32_push_svalue(int reg, INT32 offs)
{
  int e;

  LOAD_SP_REG();

  if(sizeof(struct svalue) > 8)
  {
    for(e=4;e<(int)sizeof(struct svalue);e+=4)
    {
      if( e ==  OFFSETOF(svalue,u.refs)) continue;

      /* lwz r0,e+offs(r) */
      LWZ(0, reg, e+offs);
      /* stw r0,e(pike_sp) */
      STW(0, PPC_REG_PIKE_SP, e);
    }
  }
 
  /* lwz r0,offs(r) */
  LWZ(0, reg, offs);
  /* lwz r11,refs+offs(r) */
  LWZ(11, reg, offs+OFFSETOF(svalue,u.refs));
  /* stw r0,0(pike_sp) */
  STW(0, PPC_REG_PIKE_SP, 0);
#if PIKE_BYTEORDER == 1234
  /* rlwinm r0,r0,0,16,31 */
  RLWINM(0, 0, 0, 16, 31);
#else
  /* rlwinm r0,r0,16,16,31 */
  RLWINM(0, 0, 16, 16, 31);
#endif
  /* stw r11,refs(pike_sp) */
  STW(11, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.refs));
  /* cmplwi r0,MAX_REF_TYPE */
  CMPLI(0, 0, MAX_REF_TYPE);
  INCR_SP_REG(sizeof(struct svalue));
  /* bgt bork */
  BC(12, 1, 4);
  /* lwz r0,0(r11) */
  LWZ(0, 11, 0);
  /* addic r0,r0,1 */
  ADDIC(0, 0, 1);
  /* stw r0,0(r11) */
  STW(0, 11, 0);
  /* bork: */
}

void ppc32_push_constant(INT32 arg)
{
  INT32 offs;
  LOAD_FP_REG();
  /* lwz r3,context.prog(pike_fp) */
  LWZ(PPC_REG_ARG1, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, context.prog));
  /* lwz r3,constants(r3) */
  LWZ(PPC_REG_ARG1, PPC_REG_ARG1, OFFSETOF(program, constants));

  offs = arg*sizeof(struct program_constant)+OFFSETOF(program_constant, sval);

  if(offs > 32767) {
    /* addis r3,r3,%hi(offs) */
    ADDIS(PPC_REG_ARG1, PPC_REG_ARG1, (offs+32768)>>16);
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  ppc32_push_svalue(PPC_REG_ARG1, offs);
}

void ppc32_push_local(INT32 arg)
{
  INT32 offs;
  offs = ppc32_get_local_addr(PPC_REG_ARG1, arg);
  ppc32_push_svalue(PPC_REG_ARG1, offs);
}

void ppc32_local_lvalue(INT32 arg)
{
  INT32 offs;
  int e;

  offs = ppc32_get_local_addr(PPC_REG_ARG1, arg);
  LOAD_SP_REG();
  SET_REG(0, 0);
  for(e=4;e<(int)(2*sizeof(struct svalue));e+=4)
  {
    if( e == OFFSETOF(svalue,u.lval) || e == sizeof(struct svalue) ) continue;
    /* stw r0,e(pike_sp) */
    STW(0, PPC_REG_PIKE_SP, e);
  }
  if(offs) {
    /* addi r3,r3,offs */
    ADDI(PPC_REG_ARG1, PPC_REG_ARG1, offs);
  }
#if PIKE_BYTEORDER == 1234
  SET_REG(0, T_LVALUE);
  STW(0, PPC_REG_PIKE_SP, 0);
  SET_REG(0, T_VOID);
  STW(0, PPC_REG_PIKE_SP, sizeof(struct svalue));
#else
  SET_REG(0, T_LVALUE<<16);
  STW(0, PPC_REG_PIKE_SP, 0);
  SET_REG(0, T_VOID<<16);
  STW(0, PPC_REG_PIKE_SP, sizeof(struct svalue));
#endif
  /* stw r3,u.lval(pike_sp) */
  STW(PPC_REG_ARG1, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.lval));
  INCR_SP_REG(sizeof(struct svalue)*2);  
}

void ppc32_push_int(INT32 x)
{
  LOAD_SP_REG();

  if(sizeof(struct svalue) > 8)
  {
    int e;
    SET_REG(0, 0);
    for(e=4;e<(int)sizeof(struct svalue);e+=4)
    {
      if( e == OFFSETOF(svalue,u.integer)) continue;
      /* stw r0,e(pike_sp) */
      STW(0, PPC_REG_PIKE_SP, e);
    }
  }
  if(sizeof(struct svalue) <= 8 || x != 0)
    SET_REG(0, x);
  STW(0, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.integer));
#if PIKE_BYTEORDER == 1234
  if(x != PIKE_T_INT)
    SET_REG(0, PIKE_T_INT);
#else
  if(x != (PIKE_T_INT << 16))
    SET_REG(0, (PIKE_T_INT << 16));
#endif
  STW(0, PPC_REG_PIKE_SP, 0);
  INCR_SP_REG(sizeof(struct svalue));  
}

void ppc32_mark(void)
{
  LOAD_SP_REG();
  LOAD_MARK_SP_REG();
  /* stw pike_sp, 0(pike_mark_sp) */
  STW(PPC_REG_PIKE_SP, PPC_REG_PIKE_MARK_SP, 0);
  INCR_MARK_SP_REG(sizeof(struct svalue *));
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
   case F_MARK2 - F_OFFSET:
     ppc32_mark();
   case F_MARK - F_OFFSET:
     ppc32_mark();
     return;

   case F_CONST0 - F_OFFSET:
     ppc32_push_int(0);
     return;
     
   case F_CONST1 - F_OFFSET:
     ppc32_push_int(1);
     return;
     
   case F_CONST_1 - F_OFFSET:
     ppc32_push_int(-1);
     return;
     
  case F_MAKE_ITERATOR - F_OFFSET:
    {
      extern void f_Iterator(INT32);
      SET_REG(PPC_REG_ARG1, 1);
      addr = (void *)f_Iterator;
    }
    break;
  case F_ADD - F_OFFSET:
    SET_REG(PPC_REG_ARG1, 2);
    addr = (void *)f_add;
    break;
  }
#endif
  
  FLUSH_CODE_GENERATOR_STATE();
  ADD_CALL(addr);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
#ifndef PIKE_DEBUG
  switch(a)
  {
   case F_MARK_AND_LOCAL:
     ppc32_mark();

   case F_LOCAL:
     ppc32_push_local(b);
     return;

   case F_LOCAL_LVALUE:
     ppc32_local_lvalue(b);
     return;

   case F_NUMBER:
     ppc32_push_int(b);
     return;

   case F_NEG_NUMBER:
     ppc32_push_int(-b);
     return;

   case F_CONSTANT:
     ppc32_push_constant(b);
     return;

     /*
      * And this would work nicely for all non-dynamically loaded
      * functions. Unfortunately there is no way to know if it is
      * dynamically loaded or not at this point...
      */
   case F_MARK_CALL_BUILTIN:
   case F_CALL_BUILTIN1:
     if(Pike_compiler->new_program->constants[b].sval.u.efun->internal_flags & CALLABLE_DYNAMIC)
       break;
     if(a == F_CALL_BUILTIN1)
       SET_REG(PPC_REG_ARG1, 1);
     else
       SET_REG(PPC_REG_ARG1, 0);
     FLUSH_CODE_GENERATOR_STATE();
     ADD_CALL(Pike_compiler->new_program->constants[b].sval.u.efun->function);
     return;
  }
#endif
  SET_REG(PPC_REG_ARG1, b);
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
  SET_REG(PPC_REG_ARG1, b);
  SET_REG(PPC_REG_ARG2, c);
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
