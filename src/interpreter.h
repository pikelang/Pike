
#undef GET_ARG
#undef GET_ARG2
#undef GET_JUMP
#undef SKIPJUMP

#ifdef PIKE_DEBUG

#define GET_ARG() (backlog[backlogp].arg=(\
  instr=prefix,\
  prefix=0,\
  instr+=EXTRACT_UCHAR(pc++),\
  (t_flag>3 ? sprintf(trace_buffer,"-    Arg = %ld\n",(long)instr),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))

#define GET_ARG2() (backlog[backlogp].arg2=(\
  instr=prefix2,\
  prefix2=0,\
  instr+=EXTRACT_UCHAR(pc++),\
  (t_flag>3 ? sprintf(trace_buffer,"-    Arg2 = %ld\n",(long)instr),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))

#define GET_JUMP() (backlog[backlogp].arg=(\
  (t_flag>3 ? sprintf(trace_buffer,"-    Target = %+ld\n",(long)EXTRACT_INT(pc)),write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  EXTRACT_INT(pc)))

#define SKIPJUMP() (GET_JUMP(), pc+=sizeof(INT32))

#else
#define GET_ARG() (instr=prefix,prefix=0,instr+EXTRACT_UCHAR(pc++))
#define GET_ARG2() (instr=prefix2,prefix2=0,instr+EXTRACT_UCHAR(pc++))
#define GET_JUMP() EXTRACT_INT(pc)
#define SKIPJUMP() pc+=sizeof(INT32)
#endif

#define DOJUMP() \
 do { int tmp; tmp=GET_JUMP(); pc+=tmp; if(tmp < 0) fast_check_threads_etc(6); }while(0)

#ifndef STEP_BREAK_LINE
#define STEP_BREAK_LINE
#endif

static int eval_instruction(unsigned char *pc)
{
  unsigned INT32 prefix2=0,instr, prefix=0;
  debug_malloc_touch(Pike_fp);
  while(1)
  {
    Pike_fp->pc = pc;
    instr=EXTRACT_UCHAR(pc++);

    STEP_BREAK_LINE
    
#ifdef PIKE_DEBUG
    if(t_flag > 2)
    {
      char *file, *f;
      INT32 linep;

      file=get_line(pc-1,Pike_fp->context.prog,&linep);
      while((f=STRCHR(file,'/'))) file=f+1;
      fprintf(stderr,"- %s:%4ld:(%lx): %-25s %4ld %4ld\n",
	      file,(long)linep,
	      DO_NOT_WARN((long)(pc-Pike_fp->context.prog->program-1)),
	      get_f_name(instr + F_OFFSET),
	      DO_NOT_WARN((long)(Pike_sp-Pike_interpreter.evaluator_stack)),
	      DO_NOT_WARN((long)(Pike_mark_sp-Pike_interpreter.mark_stack)));
    }

    if(instr + F_OFFSET < F_MAX_OPCODE) 
      ADD_RUNNED(instr + F_OFFSET);

    if(d_flag)
    {
      backlogp++;
      if(backlogp >= BACKLOG) backlogp=0;

      if(backlog[backlogp].program)
	free_program(backlog[backlogp].program);

      backlog[backlogp].program=Pike_fp->context.prog;
      add_ref(Pike_fp->context.prog);
      backlog[backlogp].instruction=instr;
      backlog[backlogp].pc=pc;
      backlog[backlogp].stack = Pike_sp - Pike_interpreter.evaluator_stack;
      backlog[backlogp].mark_stack = Pike_mark_sp - Pike_interpreter.mark_stack;
#ifdef _REENTRANT
      backlog[backlogp].thread_id=Pike_interpreter.thread_id;
#endif

#ifdef _REENTRANT
      CHECK_INTERPRETER_LOCK();
      if(OBJ2THREAD(Pike_interpreter.thread_id)->state.thread_id != Pike_interpreter.thread_id)
	fatal("Arglebargle glop glyf, thread swap problem!\n");

      if(d_flag>1 && thread_for_id(th_self()) != Pike_interpreter.thread_id)
        fatal("thread_for_id() (or Pike_interpreter.thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id);
#endif

      Pike_sp[0].type=99; /* an invalid type */
      Pike_sp[1].type=99;
      Pike_sp[2].type=99;
      Pike_sp[3].type=99;
      
      if(Pike_sp<Pike_interpreter.evaluator_stack || Pike_mark_sp < Pike_interpreter.mark_stack || Pike_fp->locals>Pike_sp)
	fatal("Stack error (generic) sp=%p/%p mark_sp=%p/%p locals=%p.\n",
	      Pike_sp,
	      Pike_interpreter.evaluator_stack,
	      Pike_mark_sp,
	      Pike_interpreter.mark_stack,
	      Pike_fp->locals);
      
      if(Pike_mark_sp > Pike_interpreter.mark_stack+Pike_stack_size)
	fatal("Mark Stack error (overflow).\n");


      if(Pike_mark_sp < Pike_interpreter.mark_stack)
	fatal("Mark Stack error (underflow).\n");

      if(Pike_sp > Pike_interpreter.evaluator_stack+Pike_stack_size)
	fatal("stack error (overflow).\n");
      
      if(/* Pike_fp->fun>=0 && */ Pike_fp->current_object->prog &&
	 Pike_fp->locals+Pike_fp->num_locals > Pike_sp)
	fatal("Stack error (stupid!).\n");

      if(Pike_interpreter.recoveries && Pike_sp-Pike_interpreter.evaluator_stack < Pike_interpreter.recoveries->stack_pointer)
	fatal("Stack error (underflow).\n");

      if(Pike_mark_sp > Pike_interpreter.mark_stack && Pike_mark_sp[-1] > Pike_sp)
	fatal("Stack error (underflow?)\n");
      
      if(d_flag > 9) do_debug();

      debug_malloc_touch(Pike_fp->current_object);
      switch(d_flag)
      {
	default:
	case 3:
	  check_object(Pike_fp->current_object);
/*	  break; */

	case 2:
	  check_object_context(Pike_fp->current_object,
			       Pike_fp->context.prog,
			       CURRENT_STORAGE);
	case 1:
	case 0:
	  break;
      }
    }
#endif

    switch(instr)
    {
      /* Support to allow large arguments */
      CASE(F_PREFIX_256); prefix+=256; break;
      CASE(F_PREFIX_512); prefix+=512; break;
      CASE(F_PREFIX_768); prefix+=768; break;
      CASE(F_PREFIX_1024); prefix+=1024; break;
      CASE(F_PREFIX_24BITX256);
      prefix+=EXTRACT_UCHAR(pc++)<<24;
      CASE(F_PREFIX_WORDX256);
      prefix+=EXTRACT_UCHAR(pc++)<<16;
      CASE(F_PREFIX_CHARX256);
      prefix+=EXTRACT_UCHAR(pc++)<<8;
      break;

      /* Support to allow large arguments */
      CASE(F_PREFIX2_256); prefix2+=256; break;
      CASE(F_PREFIX2_512); prefix2+=512; break;
      CASE(F_PREFIX2_768); prefix2+=768; break;
      CASE(F_PREFIX2_1024); prefix2+=1024; break;
      CASE(F_PREFIX2_24BITX256);
      prefix2+=EXTRACT_UCHAR(pc++)<<24;
      CASE(F_PREFIX2_WORDX256);
      prefix2+=EXTRACT_UCHAR(pc++)<<16;
      CASE(F_PREFIX2_CHARX256);
      prefix2+=EXTRACT_UCHAR(pc++)<<8;
      break;


#define INTERPRETER

#define OPCODE0(OP,DESC) CASE(OP); {
#define OPCODE1(OP,DESC) CASE(OP); { \
  INT32 arg1=GET_ARG();

#define OPCODE2(OP,DESC) CASE(OP); { \
  INT32 arg1=GET_ARG(); \
  INT32 arg2=GET_ARG2();


#define OPCODE0_TAIL(OP,DESC) CASE(OP);
#define OPCODE1_TAIL(OP,DESC) CASE(OP);
#define OPCODE2_TAIL(OP,DESC) CASE(OP);

#define OPCODE0_JUMP(OP,DESC) CASE(OP); {
#define OPCODE0_TAILJUMP(OP,DESC) } CASE(OP) {;

/* These are something of a special case as they
 * requires a POINTER stored explicitly after
 * the instruction itself.
 */
#define OPCODE1_JUMP(OP,DESC) CASE(OP); { \
  INT32 arg1=GET_ARG(); \

#define OPCODE2_JUMP(OP,DESC) CASE(OP); { \
  INT32 arg1=GET_ARG(); \
  INT32 arg2=GET_ARG2();

#define OPCODE1_TAILJUMP(OP,DESC) } CASE(OP) {; 
#define OPCODE2_TAILJUMP(OP,DESC) } CASE(OP) {; 


#define BREAK break; }
#define DONE break

#include "interpret_functions.h"
      
    default:
      fatal("Strange instruction %ld\n",(long)instr);
    }

  }
}

