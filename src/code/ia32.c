/*
 * $Id: ia32.c,v 1.5 2001/07/21 09:31:23 hubbe Exp $
 *
 * Machine code generator for IA32.
 *
 */

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
  
  CALL_RELATIVE(instrs[b].address);
/*  CALL_ABSOLUTE(instrs[b].address); */
  return;
}

void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
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
    opcode += (long)p->program;
    adddata2(&opcode, 1);
    prev = off+1;
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
    *(INT32*)(prog + p->relocations[rel]) += delta;
  }
}
