/*
 * $Id: ia32.c,v 1.10 2001/07/24 09:22:13 hubbe Exp $
 *
 * Machine code generator for IA32.
 *
 */

#include "operators.h"

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

void ia32_flush_code_generator(void)
{
  ia32_reg_eax=REG_IS_UNKNOWN;
}

void ia32_push_constant(struct svalue *tmp)
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
//  ia32_reg_eax = -1;
}

INT32 ins_f_jump(unsigned int b)
{
  INT32 ret;
  if(b != F_BRANCH) return -1;
  add_to_program(0xe9);
  ret=DO_NOT_WARN( (INT32) PC );
  PUSH_INT(0);
  return ret;
}

void update_f_jump(INT32 offset, INT32 to_offset)
{
  upd_pointer(offset, to_offset - offset - 4);
}

void ia32_push_int(INT32 x)
{
  struct svalue tmp;
  tmp.type=PIKE_T_INT;
  tmp.subtype=0;
  tmp.u.integer=x;
  ia32_push_constant(&tmp);
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

  do{
    static int last_prog_id=-1;
    static size_t last_num_linenumbers=-1;
    if(last_prog_id != Pike_compiler->new_program->id ||
       last_num_linenumbers != Pike_compiler->new_program->num_linenumbers)
    {
      last_prog_id=Pike_compiler->new_program->id;
      last_num_linenumbers = Pike_compiler->new_program->num_linenumbers;
      UPDATE_PC();
    }
  }while(0);

  addr=instrs[b].address;

#ifndef PIKE_DEBUG
  /* This is not very pretty */
  switch(b)
  {
    case F_CONST0 - F_OFFSET:
      ia32_push_int(0);
      return;

    case F_CONST1 - F_OFFSET:
      ia32_push_int(1);
      return;

    case F_CONST_1 - F_OFFSET:
      ia32_push_int(-1);
      return;

    case F_ADD - F_OFFSET:
      update_arg1(2);
      addr=(void *)f_add;
      break;

    case F_AND - F_OFFSET: addr=(void *)o_and; break;
    case F_OR - F_OFFSET: addr=(void *)o_or; break;
    case F_XOR - F_OFFSET: addr=(void *)o_xor; break;
    case F_MULTIPLY - F_OFFSET: addr=(void *)o_multiply; break;
    case F_DIVIDE - F_OFFSET: addr=(void *)o_divide; break;
    case F_MOD - F_OFFSET: addr=(void *)o_mod; break;
    case F_COMPL - F_OFFSET: addr=(void *)o_compl; break;
    case F_SSCANF - F_OFFSET: addr=(void *)o_sscanf; break;
  }
#endif
  CALL_RELATIVE(addr);
  ia32_reg_eax=REG_IS_UNKNOWN;
/*  CALL_ABSOLUTE(instrs[b].address); */

  return;
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
#ifndef PIKE_DEBUG
  switch(a)
  {
    case F_NUMBER:
      ia32_push_int(b);
      return;

    case F_CONSTANT:
      /* 
       * This would work nicely for all pike types, but we would
       * have to augment dumping
       */
      if(Pike_compiler->new_program->constants[b].sval.type > MAX_REF_TYPE)
      {
	ia32_push_constant(& Pike_compiler->new_program->constants[b].sval);
	return;
      }
  }
#endif
  update_arg1(b);
  ins_f_byte(a);
}

void ins_f_byte_with_2_args(unsigned int a,
			    unsigned INT32 c,
			    unsigned INT32 b)
{
  update_arg1(c);
  update_arg2(b);
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
      fatal("Relocations in bad order!\n");
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
