/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: ia32.c,v 1.30 2004/05/24 18:40:46 grubba Exp $
*/

/*
 * Machine code generator for IA32.
 */

#include "operators.h"
#include "constants.h"

#define PUSH_INT(X) ins_int((INT32)(X), (void (*)(char))add_to_program)
#define PUSH_ADDR(X) PUSH_INT((X))

#define NOP() add_to_program(0x90); /* for alignment */

/* This is ugly, but since the code may be moved we cannot use
 * relative addressing :(
 */
#define CALL_ABSOLUTE(X) do{			\
  add_to_program(0xb8);	/* mov $xxx,%eax */	\
  PUSH_INT(X);					\
  add_to_program(0xff);	/* jsr *(%eax) */	\
  add_to_program(0xd0);				\
}while(0)

#define ADDB_EAX(X) do {			\
  add_to_program(0x83);				\
  add_to_program(0xc0);				\
 add_to_program(X);				\
}while(0)

#define MOVEAX2(ADDR) do {				\
    add_to_program(0xa3 /* mov %eax, $xxxxx */);	\
    PUSH_INT( (INT32)&(ADDR) );				\
}while(0)

#define INC_MEM(ADDR, HOWMUCH) do {			\
  int h_=(HOWMUCH);					\
  if(h_ == 1)						\
  {							\
    add_to_program(0xff); /* incl  0xXXXXXXXX */	\
    add_to_program(0x05);				\
    PUSH_INT( (INT32) (ADDR) );				\
  }else{						\
    add_to_program(0x83); /* addl $0xXX, 0xXXXXXXXX */	\
    add_to_program(0x05);				\
    PUSH_INT( (INT32) (ADDR) );				\
    add_to_program(HOWMUCH);				\
  }							\
}while(0)


#define CALL_RELATIVE(X) do{						\
  struct program *p_=Pike_compiler->new_program;			\
  add_to_program(0xe8);							\
  add_to_program(0);                                                    \
  add_to_program(0);                                                    \
  add_to_program(0);                                                    \
  add_to_program(0);                                                    \
  add_to_relocations(p_->num_program-4);				\
  *(INT32 *)(p_->program + p_->num_program - 4)=                        \
    ((INT32)(X)) - (INT32)(p_->program + p_->num_program);              \
}while(0)

static void update_arg1(INT32 value)
{
  add_to_program(0xc7);  /* movl $xxxx, (%esp) */
  add_to_program(0x04); 
  add_to_program(0x24); 
  PUSH_INT(value);
}

static void update_arg2(INT32 value)
{
  add_to_program(0xc7);  /* movl $xxxx, 4(%esp) */
  add_to_program(0x44);
  add_to_program(0x24);
  add_to_program(0x04);
  PUSH_INT(value);
}

int ia32_reg_eax=REG_IS_UNKNOWN;
int ia32_reg_ecx=REG_IS_UNKNOWN;
int ia32_reg_edx=REG_IS_UNKNOWN;
ptrdiff_t ia32_prev_stored_pc = -1; /* PROG_PC at the last point
				     * Pike_fp->pc was updated. */

void ia32_flush_code_generator(void)
{
  ia32_reg_eax=REG_IS_UNKNOWN;
  ia32_reg_ecx=REG_IS_UNKNOWN;
  ia32_reg_edx=REG_IS_UNKNOWN;
  ia32_prev_stored_pc = -1;
}

void ia32_update_pc(void)
{
  INT32 tmp = PIKE_PC, disp;

  if (ia32_prev_stored_pc < 0) {
    if(ia32_reg_eax != REG_IS_FP) {
      MOV2EAX(Pike_interpreter.frame_pointer);
      ia32_reg_eax=REG_IS_FP;
    }

#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc absolute\n", tmp);
#endif

    /* Store the negated pointer to make the relocation displacements
     * work in the right direction. */
    ia32_reg_edx = REG_IS_UNKNOWN;
    add_to_program(0xba);		/* mov $xxxxxxxx, %edx */
    ins_pointer(0);
    add_to_relocations(PIKE_PC - 4);
    upd_pointer(PIKE_PC - 4, - (INT32) (tmp + Pike_compiler->new_program->program));
    add_to_program(0xf7);		/* neg %edx */
    add_to_program(0xda);
    add_to_program(0x89);		/* mov %edx, yy(%eax) */
    if (OFFSETOF(pike_frame, pc)) {
      add_to_program(0x50);
      add_to_program(OFFSETOF(pike_frame, pc));
    }
    else
      add_to_program(0x10);
  }

  else if ((disp = tmp - ia32_prev_stored_pc)) {
    if(ia32_reg_eax != REG_IS_FP) {
      MOV2EAX(Pike_interpreter.frame_pointer);
      ia32_reg_eax=REG_IS_FP;
    }

#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc relative: %d\n", tmp, disp);
#endif

    if (-128 <= disp && disp <= 127)
      /* Add sign-extended imm8 to r/m32. */
      add_to_program(0x83); /* addl $nn, yy(%eax) */
    else
      /* Add imm32 to r/m32. */
      add_to_program(0x81); /* addl $nn, yy(%eax) */
    if (OFFSETOF(pike_frame, pc)) {
      add_to_program(0x40);
      add_to_program(OFFSETOF(pike_frame, pc));
    }
    else
      add_to_program(0x0);
    if (-128 <= disp && disp <= 127)
      add_to_program(disp);
    else
      PUSH_INT(disp);
  }

  else {
#ifdef PIKE_DEBUG
    if (a_flag >= 60)
      fprintf (stderr, "pc %d  update pc - already up-to-date\n", tmp);
#endif
  }

  ia32_prev_stored_pc = tmp;
}

static void ia32_push_constant(struct svalue *tmp)
{
  int e;
  if(tmp->type <= MAX_REF_TYPE)
    INC_MEM(tmp->u.refs, 1);

  if(ia32_reg_eax != REG_IS_SP)
    MOV2EAX(Pike_interpreter.stack_pointer);

  for(e=0;e<(int)sizeof(*tmp)/4;e++)
    SET_MEM_REL_EAX(e*4, ((INT32 *)tmp)[e]);
  ADDB_EAX(sizeof(*tmp));
  MOVEAX2(Pike_interpreter.stack_pointer);

  ia32_reg_eax = REG_IS_SP;
}

/* This expects %edx to point to the svalue we wish to push */
static void ia32_push_svalue(void)
{
  int e;
  if(ia32_reg_eax != REG_IS_SP)
    MOV2EAX(Pike_interpreter.stack_pointer);

  if(sizeof(struct svalue) > 8)
  {
    for(e=4;e<(int)sizeof(struct svalue);e++)
    {
      if( e ==  OFFSETOF(svalue,u.refs)) continue;

      add_to_program(0x8b); /* mov 0x4(%edx), %ecx */
      add_to_program(0x4a); 
      add_to_program(e);
      
      add_to_program(0x89); /* mov %ecx,4(%eax) */
      add_to_program(0x48); 
      add_to_program(e);
    }
  }


  add_to_program(0x8b); /* mov 0x4(%edx), %ecx */
  add_to_program(0x4a); 
  add_to_program(OFFSETOF(svalue,u.refs));

  add_to_program(0x8b); /* mov (%edx),%edx */
  add_to_program(0x12); 

  add_to_program(0x89); /* mov %edx,(%eax) */
  add_to_program(0x10); 

  add_to_program(0x89); /* mov %ecx,4(%eax) */
  add_to_program(0x48);
  add_to_program(OFFSETOF(svalue,u.refs));

  add_to_program(0x66); /* cmp $0x7,%dx */
  add_to_program(0x83); 
  add_to_program(0xfa); 
  add_to_program(0x07); 

  add_to_program(0x77); /* ja bork */
  add_to_program(0x02);
  
  add_to_program(0xff); /* incl (%ecx) */
  add_to_program(0x01);
  /* bork: */

  ADDB_EAX(sizeof(struct svalue));
  MOVEAX2(Pike_interpreter.stack_pointer);

  ia32_reg_eax = REG_IS_SP;
  ia32_reg_ecx = REG_IS_UNKNOWN;
  ia32_reg_edx = REG_IS_UNKNOWN;
}

static void ia32_get_local_addr(INT32 arg)
{
#if 1
  if(ia32_reg_edx == REG_IS_FP)
  {
    add_to_program(0x8b); /* movl $XX(%edx), %edx */
    add_to_program(0x52);
  }else{
    switch(ia32_reg_eax)
    {
      case REG_IS_SP: /* use %edx */
	MOV2EDX(Pike_interpreter.frame_pointer);

	add_to_program(0x8b); /* movl $XX(%edx), %edx */
	add_to_program(0x52);
	break;

      default: /* use %eax */
	MOV2EAX(Pike_interpreter.frame_pointer);
	ia32_reg_eax=REG_IS_FP;

      case REG_IS_FP:
	add_to_program(0x8b); /* movl $XX(%eax), %edx */
	add_to_program(0x50);
    }
  }
#else
  MOV2EAX(Pike_interpreter.frame_pointer);
  add_to_program(0x8b); /* movl $XX(%eax), %edx */
  add_to_program(0x50);
  ia32_reg_eax = REG_IS_UNKNOWN;
#endif
  add_to_program(OFFSETOF(pike_frame, locals));
  /* EDX is now fp->locals */

  add_to_program(0x81); /* add $xxxxx,%edx */
  add_to_program(0xc2);
  PUSH_INT(arg * sizeof(struct svalue));

  /* EDX is now & ( fp->locals[arg] ) */
  ia32_reg_edx=REG_IS_UNKNOWN;
}

static void ia32_push_local(INT32 arg)
{
  ia32_get_local_addr(arg);
  ia32_push_svalue();
}

static void ia32_local_lvalue(INT32 arg)
{
  int e;
  struct svalue tmp[2];
  ia32_get_local_addr(arg);

  MEMSET(tmp, 0, sizeof(tmp));
  tmp[0].type=T_LVALUE;
  tmp[0].u.lval=(struct svalue *)4711;
  tmp[1].type=T_VOID;

  if(ia32_reg_eax != REG_IS_SP)
  {
    MOV2EAX(Pike_interpreter.stack_pointer);
    ia32_reg_eax=REG_IS_SP;
  }
  for(e=0;e<(int)sizeof(tmp)/4;e++)
  {
    INT32 t2=((INT32 *)&tmp)[e];
    if(t2 == 4711)
    {
      add_to_program(0x89); /* movl %edx, xxx(%eax) */
      add_to_program(0x50);
      add_to_program(e*4);
    }else{
      SET_MEM_REL_EAX(e*4,t2);
    }
  }

  ADDB_EAX(sizeof(struct svalue)*2);
  MOVEAX2(Pike_interpreter.stack_pointer);
}

static void ia32_mark(void)
{
  if(ia32_reg_eax != REG_IS_SP)
  {
    MOV2EAX(Pike_interpreter.stack_pointer);
    ia32_reg_eax=REG_IS_SP;
  }

#if 0
  /* who keeps changing edx?? -Hubbe */
  if(ia32_reg_edx == REG_IS_MARK_SP)
#endif
  {
    MOV2EDX(Pike_interpreter.mark_stack_pointer);
    ia32_reg_edx=REG_IS_MARK_SP;
  }

  add_to_program(0x89); /* movl %eax,(%edx) */
  add_to_program(0x02);

  add_to_program(0x83); /* addl $4, %edx */
  add_to_program(0xc2);
  add_to_program(sizeof(struct svalue *)); /*4*/
  
  add_to_program(0x89); /* movl %edx, mark_sp */
  add_to_program(0x15);
  PUSH_ADDR( & Pike_interpreter.mark_stack_pointer );
}

INT32 ins_f_jump(unsigned int b, int backward_jump)
{
  INT32 ret;
  if(b != F_BRANCH) return -1;
  add_to_program(0xe9);
  ret=DO_NOT_WARN( (INT32) PIKE_PC );
  PUSH_INT(0);
  return ret;
}

void update_f_jump(INT32 offset, INT32 to_offset)
{
  upd_pointer(offset, to_offset - offset - 4);
}

INT32 read_f_jump(INT32 offset)
{
  return read_pointer(offset) + offset + 4;
}

static void ia32_push_int(INT32 x)
{
  struct svalue tmp;
  tmp.type=PIKE_T_INT;
  tmp.subtype=0;
  tmp.u.integer=x;
  ia32_push_constant(&tmp);
}

static void ia32_call_c_function(void *addr)
{
/*  CALL_ABSOLUTE(instrs[b].address); */
  CALL_RELATIVE(addr);
  ia32_reg_eax=REG_IS_UNKNOWN;
  ia32_reg_ecx=REG_IS_UNKNOWN;
  ia32_reg_edx=REG_IS_UNKNOWN;
}

static void maybe_update_pc(void)
{
  static int last_prog_id=-1;
  static size_t last_num_linenumbers=-1;
  if(
#ifdef PIKE_DEBUG
    /* Update the pc more often for the sake of the opcode level trace. */
    d_flag ||
#endif
    last_prog_id != Pike_compiler->new_program->id ||
    last_num_linenumbers != Pike_compiler->new_program->num_linenumbers
  ) {
    last_prog_id=Pike_compiler->new_program->id;
    last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
    UPDATE_PC();
  }
}

#ifdef PIKE_DEBUG
static void ins_debug_instr_prologue (PIKE_INSTR_T instr, INT32 arg1, INT32 arg2)
{
  int flags = instrs[instr].flags;

  if (flags & I_HASARG2) {
    add_to_program (0xc7);	/* movl $xxxx, 0xc(%esp) */
    add_to_program (0x44);
    add_to_program (0x24);
    add_to_program (0x0c);
    PUSH_INT (arg2);
  }
  if (flags & I_HASARG) {
    add_to_program (0xc7);	/* movl $xxxx, 0x8(%esp) */
    add_to_program (0x44);
    add_to_program (0x24);
    add_to_program (0x08);
    PUSH_INT (arg1);
  }
  add_to_program (0xc7);	/* movl $xxxx, 0x4(%esp) */
  add_to_program (0x44);
  add_to_program (0x24);
  add_to_program (0x04);
  PUSH_INT (instr);

  if (flags & I_HASARG2)
    ia32_call_c_function (simple_debug_instr_prologue_2);
  else if (flags & I_HASARG)
    ia32_call_c_function (simple_debug_instr_prologue_1);
  else
    ia32_call_c_function (simple_debug_instr_prologue_0);
}
#else  /* !PIKE_DEBUG */
#define ins_debug_instr_prologue(instr, arg1, arg2)
#endif

void ins_f_byte(unsigned int b)
{
  void *addr;

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
  maybe_update_pc();
  addr=instrs[b].address;

#ifdef PIKE_DEBUG
  if (d_flag < 3)
#endif
  /* This is not very pretty */
  switch(b)
  {
    case F_MARK2 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      ia32_mark();
      return;

    case F_MARK - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      return;

    case F_MARK_AND_CONST0 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      ia32_push_int(0);
      return;

    case F_CONST0 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_push_int(0);
      return;

    case F_MARK_AND_CONST1 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_mark();
      ia32_push_int(1);
      return;

    case F_CONST1 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_push_int(1);
      return;

    case F_CONST_1 - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      ia32_push_int(-1);
      return;

    case F_ADD - F_OFFSET:
      ins_debug_instr_prologue (b, 0, 0);
      update_arg1(2);
      addr=(void *)f_add;
      break;

    case F_MAKE_ITERATOR - F_OFFSET:
      {
	extern void f_Iterator(INT32);
	update_arg1(1);
	addr = (void *)f_Iterator;
      }
      break;
  }
  ia32_call_c_function(addr);
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
      ia32_mark();

    case F_LOCAL:
      ia32_push_local(b);
      return;

    case F_LOCAL_LVALUE:
      ia32_local_lvalue(b);
      return;

    case F_NUMBER:
      ia32_push_int(b);
      return;

    case F_NEG_NUMBER:
      ia32_push_int(-b);
      return;

    case F_CONSTANT:
      /* 
       * This would work nicely for all pike types, but we would
       * have to augment dumping
       *
       * Note: The constants table may contain UNDEFINED in case of being
       *       called through decode_value() in PORTABLE_BYTECODE mode.
       */
      if((Pike_compiler->new_program->constants[b].sval.type > MAX_REF_TYPE) &&
	 !Pike_compiler->new_program->constants[b].sval.subtype)
      {
	ins_debug_instr_prologue (a - F_OFFSET, b, 0);
	ia32_push_constant(& Pike_compiler->new_program->constants[b].sval);
	return;
      }
      break;

      /*
       * And this would work nicely for all non-dynamically loaded
       * functions. Unfortunately there is no way to know if it is
       * dynamically loaded or not at this point...
       */
    case F_MARK_CALL_BUILTIN:
      if(Pike_compiler->new_program->constants[b].sval.u.efun->internal_flags & CALLABLE_DYNAMIC)
	break;
      ins_debug_instr_prologue (a - F_OFFSET, b, 0);
      update_arg1(0);
      ia32_call_c_function(Pike_compiler->new_program->constants[b].sval.u.efun->function);
      return;

    case F_CALL_BUILTIN1:
      if(Pike_compiler->new_program->constants[b].sval.u.efun->internal_flags & CALLABLE_DYNAMIC)
	break;
      update_arg1(1);
      ia32_call_c_function(Pike_compiler->new_program->constants[b].sval.u.efun->function);
      return;
  }
  update_arg1(b);
  ins_f_byte(a);
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
      /* We could optimize this by delaying the sp+=8  -hubbe  */
      ia32_push_local(b);
      ia32_push_local(c);
      return;
  }
  update_arg1(b);
  update_arg2(c);
  ins_f_byte(a);
}

#define addstr(s, l) low_my_binary_strcat((s), (l), buf)
#define adddata2(s,l) addstr((char *)(s),(l) * sizeof((s)[0]));

void ia32_encode_program(struct program *p, struct dynamic_buffer_s *buf)
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

    /* Relocate to 0 */
    opcode = *(INT32 *) (p->program + off);
    opcode += (INT32)p->program;
    adddata2(&opcode, 1);
    prev = off+4;
  }
  adddata2(p->program + prev, p->num_program - prev);
}

void ia32_decode_program(struct program *p)
{
  /* Relocate the program... */
  PIKE_OPCODE_T *prog = p->program;
  INT32 delta = (INT32)prog;
  size_t rel = p->num_relocations;
  while (rel--) {
    *(INT32*)(prog + p->relocations[rel]) -= delta;
  }
}
