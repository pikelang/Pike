/*\
||| This file is part of Pike. For copyright information see COPYRIGHT.
||| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
||| for more information.
||| $Id: ppc32.c,v 1.22 2002/10/08 20:22:28 nilsson Exp $
\*/

/*
 * Machine code generator for 32 bit PowerPC
 *
 * Marcus Comstedt 20010726
 */

#include "operators.h"
#include "constants.h"
#include "object.h"

#if PIKE_BYTEORDER == 1234
#define MAKE_TYPE_WORD(t,st) ((t)|((st)<<16))
#else
#define MAKE_TYPE_WORD(t,st) ((st)|((t)<<16))
#endif

#ifdef _AIX
#define ADD_CALL(X) do {						\
    INT32 delta_;							\
    void *toc_, *func_=(X);						\
									\
    __asm__("\tmr %0,"PPC_REGNAME(2) : "=r" (toc_));			\
    delta_ = ((char *)func_) - ((char *)toc_);				\
    if(delta_ < -32768 || delta_ > 32767) {				\
      /* addis r11,r2,%hi(delta) */					\
      ADDIS(11, 2, (delta_+32768)>>16);					\
      if ((delta_ &= 0xffff) > 32767)					\
	delta_ -= 65536;						\
      /* lwz r0,%lo(delta)(r11) */					\
      LWZ(0, 11, delta_);						\
    } else {								\
      /* lwz r0,delta(r2)	*/					\
      LWZ(0, 2, delta_);						\
    }									\
    /* mtlr r0 */							\
    add_to_program(0x7c0803a6);						\
    /* blrl */								\
    add_to_program(0x4e800021);						\
  } while(0)
#else
#define ADD_CALL(X) do {				\
    INT32 func_=(INT32)(void*)(X);			\
							\
    if(func_ < -33554432 || func_ > 33554431) {		\
      /* sigh.  this is so inefficient.  damn linux. */	\
      SET_REG(0, func_);				\
      /* mtlr r0 */					\
      MTSPR(0, 8);					\
      /* blrl */					\
      add_to_program(0x4e800021);			\
    } else {						\
      /* bla func */					\
      add_to_program(0x48000003|(func_&0x03fffffc));	\
    }							\
  } while(0)
#endif

int ppc32_codegen_state = 0, ppc32_codegen_last_pc = 0;

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

void ppc32_push_svalue(int reg, INT32 offs, int force_reftype)
{
  int e;

  LOAD_SP_REG();

  if(offs > (INT32)(32768-sizeof(struct svalue))) {
    /* addi r,r,offs */
    ADDI(reg, reg, offs);
    offs = 0;
  }

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
  if(!force_reftype) {
#if PIKE_BYTEORDER == 1234
    /* rlwinm r0,r0,0,16,31 */
    RLWINM(0, 0, 0, 16, 31);
#else
    /* rlwinm r0,r0,16,16,31 */
    RLWINM(0, 0, 16, 16, 31);
#endif
  }
  /* stw r11,refs(pike_sp) */
  STW(11, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.refs));
  if(!force_reftype) {
    /* cmplwi r0,MAX_REF_TYPE */
    CMPLI(0, 0, MAX_REF_TYPE);
  }
  INCR_SP_REG(sizeof(struct svalue));
  if(!force_reftype) {
    /* bgt bork */
    BC(12, 1, 4);
  }
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
  struct svalue *sval = &Pike_compiler->new_program->constants[arg].sval;

  if(sval->type > MAX_REF_TYPE) {
    int e;
    INT32 last=0;

    LOAD_SP_REG();

    for(e=0;e<(int)sizeof(struct svalue);e+=4)
    {
      if(e==0 || *(INT32 *)(((char *)sval)+e) != last)
	SET_REG(0, last = *(INT32 *)(((char *)sval)+e));
      /* stw r0,e(pike_sp) */
      STW(0, PPC_REG_PIKE_SP, e);
    }

    INCR_SP_REG(sizeof(struct svalue));  

    return;
  }

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

  ppc32_push_svalue(PPC_REG_ARG1, offs, 1);
}

void ppc32_push_local(INT32 arg)
{
  INT32 offs;
  offs = ppc32_get_local_addr(PPC_REG_ARG1, arg);
  ppc32_push_svalue(PPC_REG_ARG1, offs, 0);
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
  /* li r0,T_LVALUE */
  SET_REG(0, MAKE_TYPE_WORD(T_LVALUE, 0));
  /* stw r0,0(pike_sp) */
  STW(0, PPC_REG_PIKE_SP, 0);
  /* li r0,T_VOID */
  SET_REG(0, MAKE_TYPE_WORD(T_VOID, 0));
  /* stw r0,sizeof(struct svalue)(pike_sp) */
  STW(0, PPC_REG_PIKE_SP, sizeof(struct svalue));
  /* stw r3,u.lval(pike_sp) */
  STW(PPC_REG_ARG1, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.lval));
  INCR_SP_REG(sizeof(struct svalue)*2);  
}

void ppc32_push_global(INT32 arg)
{
  LOAD_FP_REG();
  /* lha r5,context.identifier_level(pike_fp) */
  LHA(PPC_REG_ARG3, PPC_REG_PIKE_FP,
      OFFSETOF(pike_frame, context.identifier_level));
  /* lwz r4,current_object(pike_fp) */
  LWZ(PPC_REG_ARG2, PPC_REG_PIKE_FP, 
      OFFSETOF(pike_frame, current_object));
  if(arg > 32767) {
    /* addis r5,r5,%hi(arg) */
    ADDIS(PPC_REG_ARG3, PPC_REG_ARG3, (arg+32768)>>16);
    if((arg &= 0xffff) > 32767)
      arg -= 65536;
  }
  if(ppc32_codegen_state & PPC_CODEGEN_SP_ISSET) {
    /* mr r3,pike_sp */
    ORI(PPC_REG_ARG1, PPC_REG_PIKE_SP, 0);
  } else {
    /* lwz r3,stack_pointer(pike_interpreter) */ 
    LWZ(PPC_REG_ARG1, PPC_REG_PIKE_INTERP,
	OFFSETOF(Pike_interpreter, stack_pointer));
  }
  if(arg) {
    /* addi r5,r5,arg */
    ADDI(PPC_REG_ARG3, PPC_REG_ARG3, arg);
  }
  FLUSH_CODE_GENERATOR_STATE();
  ADD_CALL(low_object_index_no_free);
  LOAD_SP_REG();
  INCR_SP_REG(sizeof(struct svalue));
}

void ppc32_push_int(INT32 x, int s)
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
  if(x != MAKE_TYPE_WORD(PIKE_T_INT, s))
    SET_REG(0, MAKE_TYPE_WORD(PIKE_T_INT, s));
  STW(0, PPC_REG_PIKE_SP, 0);
  INCR_SP_REG(sizeof(struct svalue));  
}

void ppc32_push_string(INT32 arg, int st)
{
  INT32 offs;

  LOAD_FP_REG();
  LOAD_SP_REG();
  
  /* lwz r11,context.prog(pike_fp) */
  LWZ(11, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, context.prog));
  /* lwz r11,strings(r11) */
  LWZ(11, 11, OFFSETOF(program, strings));

  offs = arg*sizeof(struct pike_string *);

  if(offs > 32767) {
    /* addis r11,r11,%hi(offs) */
    ADDIS(11, 11, (offs+32768)>>16);
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  /* lwz r11,offs(r11) */
  LWZ(11, 11, offs);
  
  if(sizeof(struct svalue) > 8)
  {
    int e;
    SET_REG(0, 0);
    for(e=4;e<(int)sizeof(struct svalue);e+=4)
    {
      if( e == OFFSETOF(svalue,u.string)) continue;
      /* stw r0,e(pike_sp) */
      STW(0, PPC_REG_PIKE_SP, e);
    }
  }
  STW(11, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.string));
  if(st)
    SET_REG(0, MAKE_TYPE_WORD(PIKE_T_STRING, 1));
  else
    SET_REG(0, MAKE_TYPE_WORD(PIKE_T_STRING, 0));
  STW(0, PPC_REG_PIKE_SP, 0);

  /* lwz r0,0(r11) */
  LWZ(0, 11, 0);
  /* addic r0,r0,1 */
  ADDIC(0, 0, 1);
  /* stw r0,0(r11) */
  STW(0, 11, 0);

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

static void ppc32_escape_catch(void)
{
  extern void *do_escape_catch_label;
  void *toc_;
  INT32 delta_;

  LOAD_FP_REG();
  __asm__("\tmr %0,"PPC_REGNAME(2) : "=r" (toc_));
  delta_ = ((char *)&do_escape_catch_label) - ((char *)toc_);
  if(delta_ < -32768 || delta_ > 32767) {
    /* addis r11,r2,%hi(delta) */
    ADDIS(11, 2, (delta_+32768)>>16);
    if ((delta_ &= 0xffff) > 32767)
      delta_ -= 65536;
    /* lwz r0,%lo(delta)(r11) */
    LWZ(0, 11, delta_);
  } else {
    /* lwz r0,delta(r2)	*/
    LWZ(0, 2, delta_);
  }
  FLUSH_CODE_GENERATOR_STATE();
  /* bl .+4 */
  add_to_program(0x48000005);
  /* mflr pike_pc */
  MFSPR(PPC_REG_PIKE_PC, 8);
  /* mtlr r0 */
  add_to_program(0x7c0803a6);
  /* addi pike_pc,pike_pc,20 */
  ADDI(PPC_REG_PIKE_PC, PPC_REG_PIKE_PC, 5*sizeof(PIKE_OPCODE_T));
  /* stw pike_pc,pc(pike_fp) */
  STW(PPC_REG_PIKE_PC, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, pc));
  /* blrl */
  add_to_program(0x4e800021);
}

static void maybe_update_pc(void)
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
  maybe_update_pc();
  addr = instrs[b].address;

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  /* This is not very pretty */
  switch(b)
  {
   case F_MARK2 - F_OFFSET:
     ppc32_mark();
   case F_MARK - F_OFFSET:
     ppc32_mark();
     return;

   case F_CONST0 - F_OFFSET:
     ppc32_push_int(0, 0);
     return;
     
   case F_CONST1 - F_OFFSET:
     ppc32_push_int(1, 0);
     return;
     
   case F_CONST_1 - F_OFFSET:
     ppc32_push_int(-1, 0);
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

  case F_EXIT_CATCH - F_OFFSET:
    ppc32_push_int(0, 1);
  case F_ESCAPE_CATCH - F_OFFSET:
    ppc32_escape_catch();
    return;
  }

  FLUSH_CODE_GENERATOR_STATE();
  ADD_CALL(addr);
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  maybe_update_pc();

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
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

   case F_MARK_AND_GLOBAL:
     ppc32_mark();

   case F_GLOBAL:
     ppc32_push_global(b);
     return;

   case F_MARK_AND_STRING:
     ppc32_mark();

   case F_STRING:
     ppc32_push_string(b, 0);
     return;

   case F_ARROW_STRING:
     ppc32_push_string(b, 1);
     return;

   case F_NUMBER:
     ppc32_push_int(b, 0);
     return;

   case F_NEG_NUMBER:
     ppc32_push_int(-b, 0);
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
  SET_REG(PPC_REG_ARG1, b);
  ins_f_byte(a);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 b,
			    unsigned INT32 c)
{
  maybe_update_pc();

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  switch(a)
  {
   case F_2_LOCALS:
     ppc32_push_local(b);
     ppc32_push_local(c);
     return;
  }
  SET_REG(PPC_REG_ARG1, b);
  SET_REG(PPC_REG_ARG2, c);
  ins_f_byte(a);
  return;
}

INT32 ppc32_ins_f_jump(unsigned int b)
{
  INT32 ret;
  if(b != F_BRANCH) return -1;
  FLUSH_CODE_GENERATOR_STATE();
  ret=DO_NOT_WARN( (INT32) PIKE_PC );
  add_to_program(0x48000000);
  return ret;
}

void ppc32_update_f_jump(INT32 offset, INT32 to_offset)
{
  PIKE_OPCODE_T *op = &Pike_compiler->new_program->program[offset];

  *op = (*op & 0xfc000003) | (((to_offset - offset)<<2)&0x03fffffc);
}

INT32 ppc32_read_f_jump(INT32 offset)
{
  INT32 delta = ((read_pointer(offset)&0x03fffffc)>>2);
  if(delta >= 0x800000)
    delta -= 0x1000000;
  return offset + delta;
}

void ppc32_flush_instruction_cache(void *addr, size_t len)
{
  INT32 a;

#ifdef _POWER
#ifdef _AIX
  __asm__(".machine \"pwr\"");
#endif
  /* Method as described in
   *
   * http://publibn.boulder.ibm.com/doc_link/en_US/a_doc_lib/aixassem/alangref/clf.htm
   */
  len >>= 2;
  for(a = (INT32)addr; len>0; --len) {
    __asm__("clf 0,%0" : : "r" (a));
    a += 4;
  }
  __asm__("dcs");
  __asm__("ics");
#else /* !_POWER */
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
#endif /* _POWER */
}

#if 0
#define addstr(s, l) low_my_binary_strcat((s), (l), buf)
#define adddata2(s,l) addstr((char *)(s),(l) * sizeof((s)[0]));

void ppc32_encode_program(struct program *p, struct dynamic_buffer_s *buf)
{
  size_t prev = 0, rel;
  /* De-relocate the program... */
  for (rel = 0; rel < p->num_relocations; rel++)
  {
    size_t off = p->relocations[rel];
    INT32 opcode;
#ifdef PIKE_DEBUG
    if (off < prev) {
      Pike_fatal("Relocations in bad order!\n");
    }
#endif /* PIKE_DEBUG */
    adddata2(p->program + prev, off - prev);

#ifdef PIKE_DEBUG
    if ((p->program[off] & 0xfc000002) != 0x48000000)
      Pike_fatal("Bad relocation: %d, off:%d, opcode: 0x%08x\n",
	    rel, off, p->program[off]);
#endif /* PIKE_DEBUG */
    /* Relocate to 0 */
    opcode = p->program[off] & 0x03ffffff;
    opcode += (INT32)(void *)(p->program+off);
    adddata2(&opcode, 1);
    prev = off+4;
  }
  adddata2(p->program + prev, p->num_program - prev);
}

void ppc32_decode_program(struct program *p)
{
  /* Relocate the program... */
  PIKE_OPCODE_T *prog = p->program;
  size_t rel = p->num_relocations;
  while (rel--) {
    PIKE_OPCODE_T *o = prog+p->relocations[rel];
    INT32 disp = *o - (INT32)(void*)o;
    if(disp < -33554432 || disp > 33554431)
      Pike_fatal("Relocation %d out of range!\n", disp);
    *o = 0x48000000 | (disp & 0x03ffffff);
  }
}
#endif

