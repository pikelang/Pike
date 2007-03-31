/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ppc64.c,v 1.1 2007/03/31 22:59:54 marcus Exp $
*/

/*
 * Machine code generator for 32 bit PowerPC
 *
 * Marcus Comstedt 20010726
 */

#include "operators.h"
#include "constants.h"
#include "object.h"
#include "builtin_functions.h"


/* Get MakeDataExecutable from CarbonLib on Mac OS X */
#ifdef HAVE_CORESERVICES_CORESERVICES_H
#include <CoreServices/CoreServices.h>
#endif


#if PIKE_BYTEORDER == 1234
#define MAKE_TYPE_WORD(t,st) ((t)|((st)<<16))
#else
#define MAKE_TYPE_WORD(t,st) ((st)|((t)<<16))
#endif

#define ADD_CALL(X) do {			\
    INT64 func_=(INT64)(void*)(X);		\
						\
    SET_REG64(11, (func_+32768)&~0xffff);	\
    if ((func_ &= 0xffff) > 32767)		\
      func_ -= 65536;				\
    /* ld r0,%lo(func)(r11) */			\
    LD(0, 11, func_);				\
    /* ld r2,%lo(func+8)(r11) */		\
    LD(2, 11, func_+8);				\
    /* mtlr r0 */				\
    MTSPR(0, PPC_SPREG_LR);			\
    /* blrl */					\
    BCLRL(20, 0);				\
  } while(0)

int ppc64_codegen_state = 0, ppc64_codegen_last_pc = 0;
static int last_prog_id=-1;

void ppc64_flush_code_generator_state()
{
  if(ppc64_codegen_state & PPC_CODEGEN_SP_NEEDSSTORE) {
    /* stw pike_sp,stack_pointer(pike_interpreter) */
    STD(PPC_REG_PIKE_SP, PPC_REG_PIKE_INTERP,
	OFFSETOF(Pike_interpreter, stack_pointer));
  }
  if(ppc64_codegen_state & PPC_CODEGEN_MARK_SP_NEEDSSTORE) {
    /* stw pike_mark_sp,mark_stack_pointer(pike_interpreter) */
    STD(PPC_REG_PIKE_MARK_SP, PPC_REG_PIKE_INTERP,
	OFFSETOF(Pike_interpreter, mark_stack_pointer));
  }
  ppc64_codegen_state = 0;
  last_prog_id=-1;
}

INT32 ppc64_get_local_addr(int reg, INT32 arg)
{
  INT32 offs = arg * sizeof(struct svalue);

  LOAD_FP_REG();
  /* ld reg,locals(pike_fp) */
  LD(reg, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, locals));

  if(offs > 32767) {
    /* addis reg,reg,%hi(offs) */
    ADDIS(reg, reg, (offs+32768)>>16);
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  return offs;
}

void ppc64_push_svalue(int reg, INT32 offs, int force_reftype)
{
  int e;

  LOAD_SP_REG();

  if(offs > (INT32)(32768-sizeof(struct svalue))) {
    /* addi r,r,offs */
    ADDI(reg, reg, offs);
    offs = 0;
  }

  if(sizeof(struct svalue) > 16)
  {
    for(e=8;e<(int)sizeof(struct svalue);e+=8)
    {
      if( e ==  OFFSETOF(svalue,u.refs)) continue;

      /* ld r0,e+offs(r) */
      LD(0, reg, e+offs);
      /* std r0,e(pike_sp) */
      STD(0, PPC_REG_PIKE_SP, e);
    }
  }
 
  /* ld r0,offs(r) */
  LD(0, reg, offs);
  /* ld r11,refs+offs(r) */
  LD(11, reg, offs+OFFSETOF(svalue,u.refs));
  /* std r0,0(pike_sp) */
  STD(0, PPC_REG_PIKE_SP, 0);
  if(!force_reftype) {
#if PIKE_BYTEORDER == 1234
    /* rldicl r0,r0,0,48 */
    RLDICL(0, 0, 0, 48);
#else
    /* rldicl r0,r0,16,48 */
    RLDICL(0, 0, 16, 48);
#endif
  }
  /* std r11,refs(pike_sp) */
  STD(11, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.refs));
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

void ppc64_push_constant(INT32 arg)
{
  INT32 offs;
  struct svalue *sval = &Pike_compiler->new_program->constants[arg].sval;

  /*
   * Note: The constants table may contain UNDEFINED in case of being
   *       called through decode_value() in PORTABLE_BYTECODE mode.
   */
  if((sval->type > MAX_REF_TYPE) && !sval->subtype) {
    int e;
    INT64 last=0;

    LOAD_SP_REG();

    for(e=0;e<(int)sizeof(struct svalue);e+=8)
    {
      if(e==0 || *(INT64 *)(((char *)sval)+e) != last)
	SET_REG64(0, last = *(INT64 *)(((char *)sval)+e));
      /* std r0,e(pike_sp) */
      STD(0, PPC_REG_PIKE_SP, e);
    }

    INCR_SP_REG(sizeof(struct svalue));  

    return;
  }

  LOAD_FP_REG();
  /* ld r3,context.prog(pike_fp) */
  LD(PPC_REG_ARG1, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, context.prog));
  /* ld r3,constants(r3) */
  LD(PPC_REG_ARG1, PPC_REG_ARG1, OFFSETOF(program, constants));

  offs = arg*sizeof(struct program_constant)+OFFSETOF(program_constant, sval);

  if(offs > 32767) {
    /* addis r3,r3,%hi(offs) */
    ADDIS(PPC_REG_ARG1, PPC_REG_ARG1, (offs+32768)>>16);
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  ppc64_push_svalue(PPC_REG_ARG1, offs, (sval->type <= MAX_REF_TYPE));
}

void ppc64_push_local(INT32 arg)
{
  INT32 offs;
  offs = ppc64_get_local_addr(PPC_REG_ARG1, arg);
  ppc64_push_svalue(PPC_REG_ARG1, offs, 0);
}

void ppc64_local_lvalue(INT32 arg)
{
  INT32 offs;
  int e;

  offs = ppc64_get_local_addr(PPC_REG_ARG1, arg);
  LOAD_SP_REG();
  SET_REG32(0, 0);
  STW(0, PPC_REG_PIKE_SP, 4);
  STW(0, PPC_REG_PIKE_SP, sizeof(struct svalue)+4);
  for(e=8;e<(int)(2*sizeof(struct svalue));e+=8)
  {
    if( e == OFFSETOF(svalue,u.lval) || e == sizeof(struct svalue) ) continue;
    /* std r0,e(pike_sp) */
    STD(0, PPC_REG_PIKE_SP, e);
  }
  if(offs) {
    /* addi r3,r3,offs */
    ADDI(PPC_REG_ARG1, PPC_REG_ARG1, offs);
  }
  /* li r0,T_SVALUE_PTR */
  SET_REG32(0, MAKE_TYPE_WORD(T_SVALUE_PTR, 0));
  /* stw r0,0(pike_sp) */
  STW(0, PPC_REG_PIKE_SP, 0);
  /* li r0,T_VOID */
  SET_REG32(0, MAKE_TYPE_WORD(T_VOID, 0));
  /* stw r0,sizeof(struct svalue)(pike_sp) */
  STW(0, PPC_REG_PIKE_SP, sizeof(struct svalue));
  /* stw r3,u.lval(pike_sp) */
  STD(PPC_REG_ARG1, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.lval));
  INCR_SP_REG(sizeof(struct svalue)*2);  
}

void ppc64_push_global(INT32 arg)
{
  LOAD_FP_REG();
  /* lha r5,context.identifier_level(pike_fp) */
  LHA(PPC_REG_ARG3, PPC_REG_PIKE_FP,
      OFFSETOF(pike_frame, context.identifier_level));
  /* ld r4,current_object(pike_fp) */
  LD(PPC_REG_ARG2, PPC_REG_PIKE_FP, 
     OFFSETOF(pike_frame, current_object));
  if(arg > 32767) {
    /* addis r5,r5,%hi(arg) */
    ADDIS(PPC_REG_ARG3, PPC_REG_ARG3, (arg+32768)>>16);
    if((arg &= 0xffff) > 32767)
      arg -= 65536;
  }
  if(ppc64_codegen_state & PPC_CODEGEN_SP_ISSET) {
    /* mr r3,pike_sp */
    ORI(PPC_REG_ARG1, PPC_REG_PIKE_SP, 0);
  } else {
    /* ld r3,stack_pointer(pike_interpreter) */ 
    LD(PPC_REG_ARG1, PPC_REG_PIKE_INTERP,
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

void ppc64_push_int(INT32 x, int s)
{
  LOAD_SP_REG();

  SET_REG32(0, 0);
  if(sizeof(struct svalue) > 16)
  {
    int e;
    for(e=8;e<(int)sizeof(struct svalue);e+=8)
    {
      if( e == OFFSETOF(svalue,u.integer)) continue;
      /* std r0,e(pike_sp) */
      STD(0, PPC_REG_PIKE_SP, e);
    }
  }
  STW(0, PPC_REG_PIKE_SP, 4);
  if(x != 0)
    SET_REG32(0, x);
  STD(0, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.integer));
  if(x != MAKE_TYPE_WORD(PIKE_T_INT, s))
    SET_REG32(0, MAKE_TYPE_WORD(PIKE_T_INT, s));
  STW(0, PPC_REG_PIKE_SP, 0);
  INCR_SP_REG(sizeof(struct svalue));  
}

void ppc64_push_string(INT32 arg, int st)
{
  INT32 offs;

  LOAD_FP_REG();
  LOAD_SP_REG();
  
  /* ld r11,context.prog(pike_fp) */
  LD(11, PPC_REG_PIKE_FP, OFFSETOF(pike_frame, context.prog));
  /* ld r11,strings(r11) */
  LD(11, 11, OFFSETOF(program, strings));

  offs = arg*sizeof(struct pike_string *);

  if(offs > 32767) {
    /* addis r11,r11,%hi(offs) */
    ADDIS(11, 11, (offs+32768)>>16);
    if((offs &= 0xffff) > 32767)
      offs -= 65536;
  }

  /* ld r11,offs(r11) */
  LD(11, 11, offs);
  
  SET_REG32(0, 0);
  if(sizeof(struct svalue) > 16)
  {
    int e;
    for(e=8;e<(int)sizeof(struct svalue);e+=8)
    {
      if( e == OFFSETOF(svalue,u.string)) continue;
      /* std r0,e(pike_sp) */
      STD(0, PPC_REG_PIKE_SP, e);
    }
  }
  STW(0, PPC_REG_PIKE_SP, 4);
  STD(11, PPC_REG_PIKE_SP, OFFSETOF(svalue,u.string));
  if(st)
    SET_REG32(0, MAKE_TYPE_WORD(PIKE_T_STRING, 1));
  else
    SET_REG32(0, MAKE_TYPE_WORD(PIKE_T_STRING, 0));
  STW(0, PPC_REG_PIKE_SP, 0);

  /* lwz r0,0(r11) */
  LWZ(0, 11, 0);
  /* addic r0,r0,1 */
  ADDIC(0, 0, 1);
  /* stw r0,0(r11) */
  STW(0, 11, 0);

  INCR_SP_REG(sizeof(struct svalue));  
}

void ppc64_mark(void)
{
  LOAD_SP_REG();
  LOAD_MARK_SP_REG();
  /* std pike_sp, 0(pike_mark_sp) */
  STD(PPC_REG_PIKE_SP, PPC_REG_PIKE_MARK_SP, 0);
  INCR_MARK_SP_REG(sizeof(struct svalue *));
}

static void ppc64_escape_catch(void)
{
  LOAD_FP_REG();
  /* ld r3,catch_ctx(pike_interpreter) */
  LD(PPC_REG_ARG1, PPC_REG_PIKE_INTERP,
     OFFSETOF(Pike_interpreter, catch_ctx));
  /* ld r4,recovery.previous(r3) */
  LD(PPC_REG_ARG2, PPC_REG_ARG1,
     OFFSETOF(catch_context, recovery.previous));
  /* std r4,recoveries(pike_interpreter) */
  STD(PPC_REG_ARG2, PPC_REG_PIKE_INTERP,
      OFFSETOF(Pike_interpreter, recoveries));
  /* ld r4,save_expendible(r3) */
  LD(PPC_REG_ARG2, PPC_REG_ARG1,
     OFFSETOF(catch_context, save_expendible));
  /* std r4,expendible(pike_fp) */
  STD(PPC_REG_ARG2, PPC_REG_PIKE_FP,
      OFFSETOF(pike_frame, expendible));
  /* ld r4,prev(r3) */
  LD(PPC_REG_ARG2, PPC_REG_ARG1,
     OFFSETOF(catch_context, prev));
  /* std r4,catch_ctx(pike_interpreter) */
  STD(PPC_REG_ARG2, PPC_REG_PIKE_INTERP,
      OFFSETOF(Pike_interpreter, catch_ctx));

  FLUSH_CODE_GENERATOR_STATE();
  ADD_CALL(really_free_catch_context);
}

static void maybe_update_pc(void)
{
  static unsigned int last_num_linenumbers=~0;
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
     ppc64_mark();
   case F_MARK - F_OFFSET:
     ppc64_mark();
     return;

   case F_CONST0 - F_OFFSET:
     ppc64_push_int(0, 0);
     return;
     
   case F_CONST1 - F_OFFSET:
     ppc64_push_int(1, 0);
     return;
     
   case F_CONST_1 - F_OFFSET:
     ppc64_push_int(-1, 0);
     return;
     
  case F_MAKE_ITERATOR - F_OFFSET:
    {
      SET_REG32(PPC_REG_ARG1, 1);
      addr = (void *)f_get_iterator;
    }
    break;
  case F_ADD - F_OFFSET:
    SET_REG32(PPC_REG_ARG1, 2);
    addr = (void *)f_add;
    break;

  case F_EXIT_CATCH - F_OFFSET:
    ppc64_push_int(0, 1);
  case F_ESCAPE_CATCH - F_OFFSET:
    ppc64_escape_catch();
    return;
  }

  FLUSH_CODE_GENERATOR_STATE();
  ADD_CALL(addr);
#ifdef OPCODE_RETURN_JUMPADDR
  if (instrs[b].flags & I_JUMP) {
    /* This is the code that JUMP_EPILOGUE_SIZE compensates for. */
    /* mtlr r3 */
    MTSPR(PPC_REG_RET, PPC_SPREG_LR);
    /* blr */
    BCLR(20, 0);
  }
#endif
}

void ins_f_byte_with_arg(unsigned int a, INT32 b)
{
  maybe_update_pc();

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  switch(a)
  {
   case F_MARK_AND_LOCAL:
     ppc64_mark();

   case F_LOCAL:
     ppc64_push_local(b);
     return;

   case F_LOCAL_LVALUE:
     ppc64_local_lvalue(b);
     return;

   case F_MARK_AND_GLOBAL:
     ppc64_mark();

   case F_GLOBAL:
     ppc64_push_global(b);
     return;

   case F_MARK_AND_STRING:
     ppc64_mark();

   case F_STRING:
     ppc64_push_string(b, 0);
     return;

   case F_ARROW_STRING:
     ppc64_push_string(b, 1);
     return;

   case F_NUMBER:
     ppc64_push_int(b, 0);
     return;

   case F_NEG_NUMBER:
     ppc64_push_int(-b, 0);
     return;

   case F_CONSTANT:
     ppc64_push_constant(b);
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
       SET_REG32(PPC_REG_ARG1, 1);
     else
       SET_REG32(PPC_REG_ARG1, 0);
     FLUSH_CODE_GENERATOR_STATE();
     ADD_CALL(Pike_compiler->new_program->constants[b].sval.u.efun->function);
     return;
  }
  SET_REG32(PPC_REG_ARG1, b);
  ins_f_byte(a);
  return;
}

void ins_f_byte_with_2_args(unsigned int a,
			    INT32 b,
			    INT32 c)
{
  maybe_update_pc();

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  switch(a)
  {
   case F_2_LOCALS:
     ppc64_push_local(b);
     ppc64_push_local(c);
     return;
  }
  SET_REG32(PPC_REG_ARG1, b);
  SET_REG32(PPC_REG_ARG2, c);
  ins_f_byte(a);
  return;
}

INT32 ppc64_ins_f_jump(unsigned int a, int backward_jump)
{
  INT32 ret, pos_=0;
  int (*test_func)(void);
  if(a == F_BRANCH)
    test_func = NULL;
  else if(a == F_CATCH || a == F_RECUR ||
	  a == F_RECUR_AND_POP || a == F_TAIL_RECUR)
    return -1;
  else
    if(a<F_OFFSET || !(instrs[a-F_OFFSET].flags & I_BRANCH) ||
       !(test_func = instrs[a-F_OFFSET].address))
      Pike_fatal("ppc64_ins_f_jump: invalid branch op %d\n", a);
  FLUSH_CODE_GENERATOR_STATE();
  if(test_func) {
    ADD_CALL(test_func);
    /* cmpldi r3,0 */
    CMPLI(1, PPC_REG_RET, 0);
    /* beq .+8 */
    BC(12, 2, 2);
    pos_ = PIKE_PC;
  }
  if(backward_jump) {
    ADD_CALL(branch_check_threads_etc);
    if(pos_)
      Pike_compiler->new_program->program[pos_-1] += 4*(PIKE_PC-pos_);
  }
  ret=DO_NOT_WARN( (INT32) PIKE_PC );
  /* b . */
  B(0);
  return ret;
}

INT32 ppc64_ins_f_jump_with_arg(unsigned int a, unsigned INT32 b, int backward_jump)
{
  if(a == F_COND_RECUR) return -1;
  SET_REG32(PPC_REG_ARG1, b);
  return ppc64_ins_f_jump(a, backward_jump);
}

INT32 ppc64_ins_f_jump_with_2_args(unsigned int a, unsigned INT32 b,
				   unsigned INT32 c, int backward_jump)
{
  SET_REG32(PPC_REG_ARG1, b);
  SET_REG32(PPC_REG_ARG2, c);
  return ppc64_ins_f_jump(a, backward_jump);
}

void ppc64_update_f_jump(INT32 offset, INT32 to_offset)
{
  PIKE_OPCODE_T *op = &Pike_compiler->new_program->program[offset];

  *op = (*op & 0xfc000003) | (((to_offset - offset)<<2)&0x03fffffc);
}

INT32 ppc64_read_f_jump(INT32 offset)
{
  INT32 delta = ((read_pointer(offset)&0x03fffffc)>>2);
  if(delta >= 0x800000)
    delta -= 0x1000000;
  return offset + delta;
}

void ppc64_flush_instruction_cache(void *addr, size_t len)
{
  INT64 a;

#ifdef HAVE_MAKEDATAEXECUTABLE
  /* Call optimized cache flushing in Mac OS X */
  MakeDataExecutable(addr, len);
  return;
#endif

#ifdef _POWER
#ifdef _AIX
  __asm__(".machine \"pwr\"");
#endif
  /* Method as described in
   *
   * http://publibn.boulder.ibm.com/doc_link/en_US/a_doc_lib/aixassem/alangref/clf.htm
   */
  len >>= 2;
  for(a = (INT64)addr; len>0; --len) {
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
  for(a = (INT64)addr; len>0; --len) {
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

void ppc64_encode_program(struct program *p, struct dynamic_buffer_s *buf)
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

void ppc64_decode_program(struct program *p)
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

#ifdef PIKE_DEBUG

void ppc64_disassemble_code(void *addr, size_t bytes)
{
  /*
    Short forms of multicharacter opcode forms:
    
    F = XFX
    G = XFL
    L = XL
    O = XO
    Z = XS
    S = SC
    N = MD
    W = MDS
    E = DS
  */
  static const char *opname[] = {
    NULL, NULL, "Dtdi", "Dtwi", NULL, NULL, NULL, "Dmulli", 
    "Dsubfic", NULL, "Dcmpli", "Dcmpi", "Daddic", "Daddic.", "Daddi", "Daddis",
    NULL, "Ssc", NULL, NULL, "Mrlwimi", "Mrlwinm", NULL, "Mrlwnm",
    "Dori", "Doris", "Dxori", "Dxoris", "Dandi.", "Dandis.", NULL, NULL,
    "Dlwz", "Dlwzu", "Dlbz", "Dlbzu", "Dstw", "Dstwu", "Dstb", "Dstbu",
    "Dlhz", "Dlhzu", "Dlha", "Dlhau", "Dsth", "Dsthu", "Dlmw", "Dstmw",
    "Dlfs", "Dlfsu", "Dlfd", "Dlfdu", "Dstfs", "Dstfsu", "Dstfd", "Dstfdu",
  };
  static const char *opname_bcx[] = {
    "Bbc", "Bbcl", "Bbca", "Bbcla",
  };
  static const char *opname_bx[] = {
    "Ib", "Ibl", "Iba", "Ibla",
  };
  static const char *opname_19[] = {
    "Lcreqv", "Lcror", NULL, NULL, NULL, "Lmcrf", "Lrfid", "Lbcctr",
    "Lbclr", NULL, "Lcrnor", "Lcrxor", "Lrfi", "Lcrand", "Lcrorc", NULL,
    "Lisync", "Lcrnand", "Lcrandc",
  };
  static short opxo_19[] = {
    289, 449, 0, 0, 0, 0, 18, 528,
    16, 0, 33, 193, 50, 257, 417, 0,
    150, 225, 129,
  };
  static const char *opname_31_010[] = {
    "Osubfc", "Omulhdu", "Oaddc", "Omulhwu", "Oneg", NULL, "Oadd", NULL,
    NULL, "Omulhd", NULL, "Omulhw", "Osubfe", NULL, "Oadde", NULL,
    "Osubfme", "Omulld", "Oaddme", "Omullw", "Osubfze", "Odivd", "Oaddze", "Odivw",
    "Osubf", "Odivdu", NULL, "Odivwu",
  };
  static short opxo_31_010[] = {
    8, 9, 10, 11, 104, 0, 266, 0,
    0, 73, 0, 75, 136, 0, 138, 0,
    232, 233, 234, 235, 200, 489, 202, 491,
    40, 457, 0, 459,
  };
  static const char *opname_31_100[] = {
    "Xmtmsrd", NULL, "Xslbmte", NULL, "Xtlbiel", NULL, "Xmtmsr", NULL,
    "Fmtcrf", "Xmfcr", "Xslbia", "Xslbmfev", "Xtlbia", "Fmftb", "Xmtsrin", "Xmfsr",
    NULL, "Fmtspr", NULL, "Fmfspr", "Xmtsr", "Xslbmfee", "Xslbie", "Xmfmsr",
    "Xtlbie", "Xmfsrin",
  };
  static short opxo_31_100[] = {
    178, 0, 402, 0, 274, 0, 146, 0,
    144, 19, 498, 851, 370, 371, 242, 595,
    0, 467, 0, 339, 210, 915, 434, 83,
    306, 659,
  };
  static const char *opname_31_101[] = {
    "Xdcba", NULL, "Xlfsx", "Xlwbrx", "Xlswx", "Xsthux", "Xecowx", NULL,
    NULL, "Xdcbi", NULL, "Xlhaux", NULL, "Xlwaux", "Xstwx", "Xstwcx.",
    "Xstdx", "Xlwzux", "Xdcbst", "Xldux", "Xlbzx", "Xdcbf", NULL, "Xldarx",
    NULL, "Xstfiwx", "Xicbi", NULL, NULL, NULL, NULL, "Xstfsx",
    "Xstwbrx", "Xstswx", "Xlfsux", "Xtlbsync", NULL, "Xlfdx", "Xsync", "Xlswi",
    NULL, NULL, NULL, "Xlhzx", "Xdcbt", NULL, "Xstwux", NULL,
    "Xstdux", "Xstbx", "Xstdcx.", NULL, "Xlbzux", NULL, NULL, NULL,
    NULL, NULL, "Xdcbz", NULL, NULL, "Xlhbrx", NULL, "Xstfsux",
    NULL, NULL, "Xstfdx", NULL, "Xstswi", "Xlfdux", NULL, NULL,
    "Xsthx", NULL, NULL, "Xlhzux", "Xeciwx", NULL, "Xlhax", NULL,
    "Xlwax", "Xstbux", "Xdcbtst", NULL, "Xlwzx", NULL, "Xldx", "Xlwarx",
    NULL, NULL, "Xsthbrx", NULL, NULL, NULL, NULL, NULL,
    "Xeieio", NULL, "Xstfdux",
  };
  static short opxo_31_101[] = {
    758, 0, 535, 534, 533, 439, 438, 0,
    0, 470, 0, 375, 0, 373, 151, 150,
    149, 55, 54, 53, 87, 86, 0, 84,
    0, 983, 982, 0, 0, 0, 0, 663,
    662, 661, 567, 566, 0, 599, 598, 597,
    0, 0, 0, 279, 278, 0, 183, 0,
    181, 215, 214, 0, 119, 0, 0, 0,
    0, 0, 1014, 0, 0, 790, 0, 695,
    0, 0, 727, 0, 725, 631, 0, 0,
    407, 0, 0, 311, 310, 0, 343, 0,
    341, 247, 246, 0, 23, 0, 21, 20,
    0, 0, 918, 0, 0, 0, 0, 0,
    854, 0, 759,
  };
  static const char *opname_31_110[] = {
    NULL, NULL, "Xextsh", NULL, "Xextsb", NULL, "Xsld", "Xcntlzw",
    "Xsrd", "Xslw", "Xsrad", "Xsrw", "Xsraw", "Xextsw", "Xsrawi",
  };
  static short opxo_31_110[] = {
    0, 0, 922, 0, 954, 0, 27, 26,
    539, 24, 794, 536, 792, 986, 824,
  };
  static const char *opname_31_111[] = {
    "Xand", "Xandc", NULL, "Xnor", NULL, NULL, NULL, NULL,
    "Xeqv", "Xxor", NULL, NULL, "Xorc", "Xor", "Xnand", NULL,
  };
  static const char *opname_59[] = {
    NULL, NULL, "Afdivs", NULL, "Afsubs", "Afadds", "Afsqrts", NULL,
    "Afres", "Afmuls", NULL, NULL, "Afmsubs", "Afmadds", "Afnmsubs", "Afnmadds"
  };
  static const char *opname_63_0[] = {
    NULL, "Xfnabs", "Xfabs", NULL, NULL, "Xfcmpu", NULL, NULL,
    "Xfcmpo", NULL, "Xmtfsb1", "Xmcrfs", NULL, "Xmtfsb0", "Xfctiwz", "Xfctiw",
    "Xfneg", "Xfrsp", NULL, "Xfmr", NULL, NULL, "Gmtfsf", NULL,
    NULL, NULL, NULL, NULL, "Xfctidz", "Xfctid", "Xmtfsfi", NULL,
    "Xfcfid", NULL, "Xmffs",
  };
  static short opxo_63_0[] = {
    0, 136, 264, 0, 0, 0, 0, 0,
    32, 0, 38, 64, 0, 70, 15, 14,
    40, 12, 0, 72, 0, 0, 711, 0,
    0, 0, 0, 0, 815, 814, 134, 0,
    846, 0, 583,
  };
  static const char *opname_63_1[] = {
    NULL, NULL, "Afdiv", NULL, "Afsub", "Afadd", "Afsqrt", "Afsel",
    NULL, "Afmul", "Afrsqrte", NULL, "Afmsub", "Afmadd", "Afnmsub", "Afnmadd"
  };
  unsigned INT32 *code = addr;
  size_t len = (bytes+3)>>2;

  while(len--) {
    int opcd, xo=0, form;
    const char *instr_name;
    unsigned INT32 instr = *code;

    fprintf(stderr, "%p  %08x      ", code++, instr);

    switch(opcd = ((instr>>26)&63)) {
    case 16: /* bcx, B-Form */
      instr_name = opname_bcx[instr&3];
      break;
    case 18: /* bx, I-Form */
      instr_name = opname_bx[instr&3];
      break;
    case 19:
      {
	int h;
	xo = (instr>>1)&1023;
	h = (xo^119)%19;
	instr_name = (xo == opxo_19[h]? opname_19[h]:NULL);	
      }
      break;
    case 30: /* 64 bit rotate, MD or MDS form */
      xo = (instr>>2)&7;
      switch(xo) {
      case 0: instr_name = "Nrldicl"; break;
      case 1: instr_name = "Nrldicr"; break;
      case 2: instr_name = "Nrldic"; break;
      case 3: instr_name = "Nrldimi"; break;
      case 4: instr_name = ((instr&2)? "Wrldcr":"Wrldcl"); break;
      default: instr_name = NULL; break;
      }
      break;
    case 31:
      xo = (instr>>1)&1023;
      switch((xo>>2)&7) {
	int h;
      case 0:
	switch(xo) {
	case 0: instr_name = "Xcmp"; break;
	case 32: instr_name = "Xcmpl"; break;
	case 512: instr_name = "Xmcrxr"; break;
	default: instr_name = NULL; break;
	}
	break;
      case 1:
	switch(xo) {
	case 4: instr_name = "Xtw"; break;
	case 58: instr_name = "Xcntlzd"; break;
	case 68: instr_name = "Xtd"; break;
	case 711: instr_name = "Zsradi"; break;
	default: instr_name = NULL; break;
	}
	break;
      case 2:
	h = ((xo&0x1ff)^160)%28;
	instr_name = ((xo&0x1ff) == opxo_31_010[h]? opname_31_010[h]:NULL);
	break;
      case 4:
	h = (xo^98)%26;
	instr_name = (xo == opxo_31_100[h]? opname_31_100[h]:NULL);
	break;
      case 5:
	h = (xo^67)%99;
	instr_name = (xo == opxo_31_101[h]? opname_31_101[h]:NULL);
	break;
      case 6:
	h = (xo^195)%15;
	instr_name = (xo == opxo_31_110[h]? opname_31_110[h]:NULL);
	break;
      case 7:
	instr_name = ((xo & 0x21f)==0x1c? opname_31_111[(xo>>5)&15]:NULL);
	break;
      default:
	instr_name = NULL;
	break;
      }
      break;
    case 58: /* 64-bit load, DS-Form */
      xo = instr&3;
      switch(xo) {
      case 0: instr_name = "Eld"; break;
      case 1: instr_name = "Eldu"; break;
      case 2: instr_name = "Elwa"; break;
      default: instr_name = NULL; break;
      }
      break;
    case 59: /* single precision math, A-Form */
      xo = (instr>>1)&0x1f;
      instr_name = ((xo&0x10)? opname_59[xo&15] : NULL);
      break;
    case 62: /* 64-bit store, DS-Form */
      xo = instr&3;
      switch(xo) {
      case 0: instr_name = "Estd"; break;
      case 1: instr_name = "Estdu"; break;
      default: instr_name = NULL; break;
      }
      break;
    case 63: /* double precision math */
      if(instr&0x20) {
	xo = (instr>>1)&0x1f;
	instr_name = opname_63_1[xo&15];
      } else {
	int h;
	xo = (instr>>1)&0x3ff;
	h = (xo^355)%35;
	instr_name = (xo == opxo_63_0[h]? opname_63_0[h]:NULL);
      }
      break;
    default:
      instr_name = (opcd < 56? opname[opcd] : NULL);
      break;
    }
    form = (instr_name? *instr_name++ : '?');
    switch(form) {
    case 'I':
      fprintf(stderr, "%s 0x%lx\n", instr_name,
	      (instr&0x03fffffc)+((instr&2)? 0:(INT64)(code-1)));
      break;
    case 'B':
      /* Maybe pretty-print BO/BI here? */
      fprintf(stderr, "%s %d,%d,0x%lx\n", instr_name,
	      (instr>>21)&31, (instr>>16)&31,
	      (instr&0x0000fffc)+((instr&2)? 0:(INT64)(code-1)));
      break;
    case 'S': /* really 'SC' */
      fprintf(stderr, "%s\n", instr_name);
      break;
    case 'D':
    case 'E': /* really 'DS' */
      {
	INT32 imm = instr & 0xffff;
	char immtext[8];
	if(form == 'E')
	  imm &= ~3;
	if((imm & 0x8000) && opcd != 10 && (opcd < 24 || opcd > 29))
	  imm -= 0x10000;
	if(imm<0)
	  sprintf(immtext, "-0x%x", -imm);
	else
	  sprintf(immtext, "0x%x", imm);
	if(opcd == 2 || opcd == 3)
	  fprintf(stderr, "%s %d,r%d,%s\n", instr_name,
		  (instr>>21)&31, (instr>>16)&31, immtext);
	else if(opcd == 10 || opcd == 11)
	  fprintf(stderr, "%s cr%d,%d,r%d,%s\n", instr_name,
		  (instr>>23)&7, (instr>>21)&1, (instr>>16)&31, immtext);
	else if(opcd >= 32)
	  fprintf(stderr, "%s r%d,%s(r%d)\n", instr_name,
		  (instr>>21)&31, immtext, (instr>>16)&31);
	else if(opcd >= 24 && opcd <= 29)
	  fprintf(stderr, "%s r%d,r%d,%s\n", instr_name,
		  (instr>>16)&31, (instr>>21)&31, immtext);
	else
	  fprintf(stderr, "%s r%d,r%d,%s\n", instr_name,
		  (instr>>21)&31, (instr>>16)&31, immtext);
      }
      break;
    case 'X':
      if(opcd == 63) {
	if(xo & 8)
	  fprintf(stderr, "%s%s fr%d,fr%d\n", instr_name, ((instr&1)? ".":""),
		  (instr>>21)&31, (instr>>11)&31);
	else if(xo & 512)
	  fprintf(stderr, "%s%s fr%d\n", instr_name, ((instr&1)? ".":""),
		  (instr>>21)&31);
	else if(xo & 128)
	  fprintf(stderr, "%s%s crf%d,%d\n", instr_name, ((instr&1)? ".":""),
		  (instr>>23)&7, (instr>>12)&15);
	else if(xo & 2)
	  fprintf(stderr, "%s%s crb%d\n", instr_name, ((instr&1)? ".":""),
		  (instr>>21)&31);
	else if(xo & 64)
	  fprintf(stderr, "%s crf%d,crf%d\n", instr_name,
		  (instr>>23)&7, (instr>>18)&7);
	else
	  fprintf(stderr, "%s crf%d,fr%d,fr%d\n", instr_name,
		  (instr>>23)&7, (instr>>16)&31, (instr>>11)&31);
      } else if((xo & 135)==135) {
	fprintf(stderr, "%s fr%d,r%d,r%d\n", instr_name,
		(instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
      } else {
	if (xo == 824)
	  fprintf(stderr, "%s%s r%d,r%d,%d\n", instr_name, ((instr&1)? ".":""),
		  (instr>>16)&31, (instr>>21)&31, (instr>>11)&31);
	else if(!(xo & 991))
	  fprintf(stderr, "%s crf%d,%d,r%d,r%d\n", instr_name, (instr>>23)&7,
		  (instr>>21)&1, (instr>>16)&31, (instr>>11)&31);
	else if((xo & 27)==24)
	  fprintf(stderr, "%s%s r%d,r%d,r%d\n", instr_name,
		  ((instr&1)? ".":""), (instr>>16)&31, (instr>>21)&31,
		  (instr>>11)&31);
	else if(!(xo & 24)) {
	  if(xo&4)
	    fprintf(stderr, "%s %d,r%d,r%d\n", instr_name,
		    (instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
	  else
	    fprintf(stderr, "%s crf%d\n", instr_name, (instr>>23)&7);
	} else if(xo & 8)
	  fprintf(stderr, "%s%s r%d,r%d\n", instr_name, ((instr&1)? ".":""),
		  (instr>>16)&31, (instr>>21)&31);
	else if(!(xo & 772)) {
	  if(xo & 32)
	    fprintf(stderr, "%s r%d,r%d\n", instr_name,
		    (instr>>21)&31, (instr>>11)&31);
	  else if((xo & 192)==192)
	    fprintf(stderr, "%s r%d\n", instr_name, (instr>>21)&31);
	  else
	    fprintf(stderr, "%s %d,r%d\n", instr_name,
		    (instr>>16)&15, (instr>>21)&31);
	} else if((xo & 127)==85)
	  fprintf(stderr, "%s r%d,r%d,%d\n", instr_name,
		  (instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
	else if(xo == 370 || xo == 566 || xo == 598 || xo == 854)
	  fprintf(stderr, "%s\n", instr_name);
	else if(!(xo & 8)) {
	  if(xo & 128)
	    fprintf(stderr, "%s r%d,r%d\n", instr_name,
		    (instr>>21)%31, (instr>>11)&31);
	  else if(xo & 512)
	    fprintf(stderr, "%s r%d,%d\n", instr_name,
		    (instr>>21)%31, (instr>>16)&15);
	  else
	    fprintf(stderr, "%s r%d\n", instr_name, (instr>>11)&31);
	} else if(instr_name[0]=='d' || instr_name[0]=='i')
	  fprintf(stderr, "%s r%d,r%d\n", instr_name,
		  (instr>>16)&31, (instr>>11)&31);
	else
	  fprintf(stderr, "%s r%d,r%d,r%d\n", instr_name,
		  (instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
      }
      break;
    case 'L': /* really 'XL' */
      if(!xo)
	fprintf(stderr, "%s cr%d,cr%d\n", instr_name,
		(instr>>23)&7, (instr>>18)&7);
      else if((xo & 511) == 16)
	/* Maybe pretty-print BO/BI here? */
	fprintf(stderr, "%s%s %d,%d\n", instr_name, ((instr&1)? "l":""),
		(instr>>21)&31, (instr>>16)&31);
      else if(xo&1) 
	fprintf(stderr, "%s crb%d,crb%d,crb%d\n", instr_name,
		(instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
      else
	fprintf(stderr, "%s\n", instr_name);
      break;
    case 'F': /* really 'XFX' */
      {
	int arg = (xo == 144? (instr>>12)&255 :
		   ((instr>>6)&0x3e0)|((instr>>16)&31));
	if(xo&128)
	  fprintf(stderr, "%s %d,r%d\n", instr_name, arg, (instr>>21)&31);
	else
	  fprintf(stderr, "%s r%d,%d\n", instr_name, (instr>>21)&31, arg);
      }
      break;
    case 'G': /* really 'XFL' */
      fprintf(stderr, "%s%s 0x%x,fr%d\n", instr_name, ((instr&1)? ".":""),
	      (instr>>17)&255, (instr>>11)&31);
      break;
    case 'O': /* really 'XO' */
      if((xo&0x41)==0x40)
	fprintf(stderr, "%s%s%s r%d,r%d\n", instr_name,
		((instr&(1<<10))? "o":""), ((instr&1)? ".":""),
		(instr>>21)&31, (instr>>16)&31);
      else
	fprintf(stderr, "%s%s%s r%d,r%d,r%d\n", instr_name,
		((instr&(1<<10))? "o":""), ((instr&1)? ".":""),
		(instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
      break;
    case 'Z': /* really 'XS' */
      fprintf(stderr, "%s%s r%d,r%d,%d\n",
	      instr_name, ((instr&1)? ".":""), (instr>>16)&31, (instr>>21)&31,
	      ((instr>>11)&31)|((instr&2)<<4));
      break;
    case 'A':
      if(xo<22)
	fprintf(stderr, "%s%s fr%d,fr%d,fr%d\n", instr_name,
		((instr&1)? ".":""),
		(instr>>21)&31, (instr>>16)&31, (instr>>11)&31);
      else if(xo >= 28 || xo == 23)
	fprintf(stderr, "%s%s fr%d,fr%d,fr%d,fr%d\n", instr_name,
		((instr&1)? ".":""),
		(instr>>21)&31, (instr>>16)&31, (instr>>6)&31, (instr>>11)&31);
      else if(xo & 1)
	fprintf(stderr, "%s%s fr%d,fr%d,fr%d\n", instr_name,
		((instr&1)? ".":""),
		(instr>>21)&31, (instr>>16)&31, (instr>>6)&31);
      else
	fprintf(stderr, "%s%s fr%d,fr%d\n", instr_name, ((instr&1)? ".":""),
		(instr>>21)&31, (instr>>11)&31);
      break;
    case 'M':
      fprintf(stderr, (opcd==23? "%s%s r%d,r%d,r%d,%d,%d\n" :
		       "%s%s r%d,r%d,%d,%d,%d\n"),
	      instr_name, ((instr&1)? ".":""), (instr>>16)&31, (instr>>21)&31,
	      (instr>>11)&31, (instr>>6)&31, (instr>>1)&31);
      break;
    case 'N':
      fprintf(stderr, "%s%s r%d,r%d,%d,%d\n",
	      instr_name, ((instr&1)? ".":""), (instr>>16)&31, (instr>>21)&31,
	      ((instr>>11)&31)|((instr&2)<<4), ((instr>>6)&31)|(instr&32));
      break;
    case 'W':
      fprintf(stderr, "%s%s r%d,r%d,r%d,%d\n",
	      instr_name, ((instr&1)? ".":""), (instr>>16)&31, (instr>>21)&31,
	      (instr>>11)&31, ((instr>>6)&31)|(instr&32));
      break;
    default:
      fprintf(stderr, ".long 0x%x\n", instr);
      break;
    }
  }
}

#endif
