
#undef LOW_GET_ARG
#undef LOW_GET_JUMP
#undef LOW_SKIPJUMP
#undef GET_ARG
#undef GET_ARG2
#undef GET_JUMP
#undef SKIPJUMP
#undef DOJUMP
#undef CASE
#undef BREAK
#undef DONE

#ifdef HAVE_COMPUTED_GOTO

#define CASE(OP)	PIKE_CONCAT(LABEL_,OP):
#ifdef PIKE_DEBUG
#define DONE		continue
#else /* !PIKE_DEBUG */
#define DONE		do {	\
    Pike_fp->pc = pc;		\
    instr = (pc++)[0];		\
    goto *instr;		\
  } while(0)
    
#endif /* PIKE_DEBUG */

#define LOW_GET_ARG()	((INT32)(ptrdiff_t)(*(pc++)))
#define LOW_GET_JUMP()	((INT32)(ptrdiff_t)(*(pc)))
#define LOW_SKIPJUMP()	(pc++)

#define GET_ARG()	LOW_GET_ARG()
#define GET_ARG2()	LOW_GET_ARG()

#else /* !HAVE_COMPUTED_GOTO */

#define CASE(X)		case (X)-F_OFFSET:
#define DONE		break

#define LOW_GET_ARG()	((pc++)[0])
#define LOW_GET_JUMP()	EXTRACT_INT(pc)
#define LOW_SKIPJUMP()	(pc += sizeof(INT32))

#ifdef PIKE_DEBUG

#define GET_ARG() (backlog[backlogp].arg=(\
  instr=prefix,\
  prefix=0,\
  instr += LOW_GET_ARG(),\
  (t_flag>3 ? sprintf(trace_buffer, "-    Arg = %ld\n", \
                     (long)instr), \
              write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))

#define GET_ARG2() (backlog[backlogp].arg2=(\
  instr=prefix2,\
  prefix2=0,\
  instr += LOW_GET_ARG(),\
  (t_flag>3 ? sprintf(trace_buffer, "-    Arg2 = %ld\n", \
                      (long)instr), \
              write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0),\
  instr))

#else /* !PIKE_DEBUG */

#define GET_ARG() (instr=prefix,prefix=0,instr+LOW_GET_ARG())
#define GET_ARG2() (instr=prefix2,prefix2=0,instr+LOW_GET_ARG())

#endif /* PIKE_DEBUG */

#endif /* HAVE_COMPUTED_GOTO */

#ifdef PIKE_DEBUG

#define GET_JUMP() (backlog[backlogp].arg=(\
  (t_flag>3 ? sprintf(trace_buffer, "-    Target = %+ld\n", \
                      (long)LOW_GET_JUMP()), \
              write_to_stderr(trace_buffer,strlen(trace_buffer)) : 0), \
  LOW_GET_JUMP()))

#define SKIPJUMP() (GET_JUMP(), LOW_SKIPJUMP())

#else /* !PIKE_DEBUG */

#define GET_JUMP() LOW_GET_JUMP()
#define SKIPJUMP() LOW_SKIPJUMP()

#endif /* PIKE_DEBUG */

#define DOJUMP() do { \
    INT32 tmp; \
    tmp = GET_JUMP(); \
    pc += tmp; \
    if(tmp < 0) \
      fast_check_threads_etc(6); \
  } while(0)

#ifndef STEP_BREAK_LINE
#define STEP_BREAK_LINE
#endif

static int eval_instruction(PIKE_OPCODE_T *pc)
{
  unsigned INT32 prefix2=0,prefix=0;
#ifdef HAVE_COMPUTED_GOTO
  static void *strap = &&init_strap;
  void *instr = NULL;
#else /* !HAVE_COMPUTED_GOTO */
  unsigned INT32 instr;
#endif /* HAVE_COMPUTED_GOTO */

#ifdef HAVE_COMPUTED_GOTO
  goto *strap;
 normal_strap:
#endif /* HAVE_COMPUTED_GOTO */

  debug_malloc_touch(Pike_fp);
  while(1)
  {
    Pike_fp->pc = pc;
    instr = (pc++)[0];

    STEP_BREAK_LINE
    
#ifdef PIKE_DEBUG

    if(t_flag > 2)
    {
      char *file, *f;
      INT32 linep;

      file = get_line(pc-1,Pike_fp->context.prog,&linep);
      while((f=STRCHR(file,'/'))) file=f+1;
      fprintf(stderr,"- %s:%4ld:(%lx): %-25s %4ld %4ld\n",
	      file,(long)linep,
	      DO_NOT_WARN((long)(pc-Pike_fp->context.prog->program-1)),
#ifdef HAVE_COMPUTED_GOTO
	      get_opcode_name(instr),
#else /* !HAVE_COMPUTED_GOTO */
	      get_f_name(instr + F_OFFSET),
#endif /* HAVE_COMPUTED_GOTO */
	      DO_NOT_WARN((long)(Pike_sp-Pike_interpreter.evaluator_stack)),
	      DO_NOT_WARN((long)(Pike_mark_sp-Pike_interpreter.mark_stack)));
    }

#ifdef HAVE_COMPUTED_GOTO
    if (instr) 
      ADD_RUNNED(instr);
#else /* !HAVE_COMPUTED_GOTO */
    if(instr + F_OFFSET < F_MAX_OPCODE) 
      ADD_RUNNED(instr + F_OFFSET);
#endif /* HAVE_COMPUTED_GOTO */

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
      if(OBJ2THREAD(Pike_interpreter.thread_id)->state.thread_id !=
	 Pike_interpreter.thread_id)
	fatal("Arglebargle glop glyf, thread swap problem!\n");

      if(d_flag>1 && thread_for_id(th_self()) != Pike_interpreter.thread_id)
        fatal("thread_for_id() (or Pike_interpreter.thread_id) failed in interpreter.h! %p != %p\n",thread_for_id(th_self()),Pike_interpreter.thread_id);
#endif

      Pike_sp[0].type=99; /* an invalid type */
      Pike_sp[1].type=99;
      Pike_sp[2].type=99;
      Pike_sp[3].type=99;
      
      if(Pike_sp<Pike_interpreter.evaluator_stack ||
	 Pike_mark_sp < Pike_interpreter.mark_stack || Pike_fp->locals>Pike_sp)
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

      if(Pike_interpreter.recoveries &&
	 (Pike_sp-Pike_interpreter.evaluator_stack <
	  Pike_interpreter.recoveries->stack_pointer))
	fatal("Stack error (underflow).\n");

      if(Pike_mark_sp > Pike_interpreter.mark_stack &&
	 Pike_mark_sp[-1] > Pike_sp)
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

#ifdef HAVE_COMPUTED_GOTO
    goto *instr;
#else /* !HAVE_COMPUTED_GOTO */
    switch(instr)
    {
      /* NOTE: The prefix handling is not needed in computed-goto mode. */
      /* Support to allow large arguments */
      CASE(F_PREFIX_256); prefix+=256; DONE;
      CASE(F_PREFIX_512); prefix+=512; DONE;
      CASE(F_PREFIX_768); prefix+=768; DONE;
      CASE(F_PREFIX_1024); prefix+=1024; DONE;
      CASE(F_PREFIX_24BITX256);
        prefix += (pc++)[0]<<24;
      CASE(F_PREFIX_WORDX256);
        prefix += (pc++)[0]<<16;
      CASE(F_PREFIX_CHARX256);
        prefix += (pc++)[0]<<8;
      DONE;

      /* Support to allow large arguments */
      CASE(F_PREFIX2_256); prefix2+=256; DONE;
      CASE(F_PREFIX2_512); prefix2+=512; DONE;
      CASE(F_PREFIX2_768); prefix2+=768; DONE;
      CASE(F_PREFIX2_1024); prefix2+=1024; DONE;
      CASE(F_PREFIX2_24BITX256);
        prefix2 += (pc++)[0]<<24;
      CASE(F_PREFIX2_WORDX256);
        prefix2 += (pc++)[0]<<16;
      CASE(F_PREFIX2_CHARX256);
        prefix2 += (pc++)[0]<<8;
      DONE;
#endif /* HAVE_COMPUTED_GOTO */


#define INTERPRETER

#define OPCODE0(OP, DESC, CODE) CASE(OP); CODE; DONE
#define OPCODE1(OP, DESC, CODE) CASE(OP); { \
    INT32 arg1=GET_ARG(); \
    CODE; \
  } DONE

#define OPCODE2(OP, DESC, CODE) CASE(OP); { \
    INT32 arg1=GET_ARG(); \
    INT32 arg2=GET_ARG2(); \
    CODE; \
  } DONE

#define OPCODE0_TAIL(OP, DESC, CODE) CASE(OP); CODE
#define OPCODE1_TAIL(OP, DESC, CODE) CASE(OP); CODE
#define OPCODE2_TAIL(OP, DESC, CODE) CASE(OP); CODE

#define OPCODE0_JUMP(OP, DESC, CODE) CASE(OP); CODE; DONE
#define OPCODE0_TAILJUMP(OP, DESC, CODE) CASE(OP); CODE

/* These are something of a special case as they
 * requires a POINTER stored explicitly after
 * the instruction itself.
 */
#define OPCODE1_JUMP(OP, DESC, CODE) CASE(OP); { \
    INT32 arg1=GET_ARG(); \
    CODE; \
  } DONE

#define OPCODE2_JUMP(OP, DESC, CODE) CASE(OP); { \
    INT32 arg1=GET_ARG(); \
    INT32 arg2=GET_ARG2(); \
    CODE; \
  } DONE

#define OPCODE1_TAILJUMP(OP, DESC, CODE) CASE(OP); CODE
#define OPCODE2_TAILJUMP(OP, DESC, CODE) CASE(OP); CODE

#include "interpret_functions.h"

#ifndef HAVE_COMPUTED_GOTO      
    default:
      fatal("Strange instruction %ld\n",(long)instr);
    }
#endif /* !HAVE_COMPUTED_GOTO */
  }

  /* NOT_REACHED */

#ifdef HAVE_COMPUTED_GOTO

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE0_TAIL
#undef OPCODE1_TAIL
#undef OPCODE2_TAIL
#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP
#undef LABEL
#define LABEL(OP)			&&PIKE_CONCAT(LABEL_,OP)
#define NULL_LABEL(OP)			NULL
#define OPCODE0(OP,DESC)		LABEL(OP),
#define OPCODE1(OP,DESC)		LABEL(OP),
#define OPCODE2(OP,DESC)		LABEL(OP),
#define OPCODE0_TAIL(OP,DESC)		LABEL(OP),
#define OPCODE1_TAIL(OP,DESC)		LABEL(OP),
#define OPCODE2_TAIL(OP,DESC)		LABEL(OP),
#define OPCODE0_JUMP(OP,DESC)		LABEL(OP),
#define OPCODE1_JUMP(OP,DESC)		LABEL(OP),
#define OPCODE2_JUMP(OP,DESC)		LABEL(OP),
#define OPCODE0_TAILJUMP(OP,DESC)	LABEL(OP),
#define OPCODE1_TAILJUMP(OP,DESC)	LABEL(OP),
#define OPCODE2_TAILJUMP(OP,DESC)	LABEL(OP),

 init_strap:
  strap = &&normal_strap;
  {
    static void *table[] = {
      NULL_LABEL(F_OFFSET),

      NULL_LABEL(F_PREFIX_256),
      NULL_LABEL(F_PREFIX_512),
      NULL_LABEL(F_PREFIX_768),
      NULL_LABEL(F_PREFIX_1024),
      NULL_LABEL(F_PREFIX_CHARX256),
      NULL_LABEL(F_PREFIX_WORDX256),
      NULL_LABEL(F_PREFIX_24BITX256),

      NULL_LABEL(F_PREFIX2_256),
      NULL_LABEL(F_PREFIX2_512),
      NULL_LABEL(F_PREFIX2_768),
      NULL_LABEL(F_PREFIX2_1024),
      NULL_LABEL(F_PREFIX2_CHARX256),
      NULL_LABEL(F_PREFIX2_WORDX256),
      NULL_LABEL(F_PREFIX2_24BITX256),

      LABEL(F_INDEX),
      LABEL(F_POS_INT_INDEX),
      LABEL(F_NEG_INT_INDEX),

      LABEL(F_RETURN),
      LABEL(F_DUMB_RETURN),
      LABEL(F_RETURN_0),
      LABEL(F_RETURN_1),
      LABEL(F_RETURN_LOCAL),
      LABEL(F_RETURN_IF_TRUE),

#include "interpret_protos.h"
    };

    static struct op_2_f lookup[] = {
#undef LABEL
#define LABEL(OP)	{ &&PIKE_CONCAT(LABEL_, OP), OP }
#undef NULL_LABEL
#define NULL_LABEL(OP)	{ NULL, OP }

      NULL_LABEL(F_OFFSET),

      NULL_LABEL(F_PREFIX_256),
      NULL_LABEL(F_PREFIX_512),
      NULL_LABEL(F_PREFIX_768),
      NULL_LABEL(F_PREFIX_1024),
      NULL_LABEL(F_PREFIX_CHARX256),
      NULL_LABEL(F_PREFIX_WORDX256),
      NULL_LABEL(F_PREFIX_24BITX256),

      NULL_LABEL(F_PREFIX2_256),
      NULL_LABEL(F_PREFIX2_512),
      NULL_LABEL(F_PREFIX2_768),
      NULL_LABEL(F_PREFIX2_1024),
      NULL_LABEL(F_PREFIX2_CHARX256),
      NULL_LABEL(F_PREFIX2_WORDX256),
      NULL_LABEL(F_PREFIX2_24BITX256),

      LABEL(F_INDEX),
      LABEL(F_POS_INT_INDEX),
      LABEL(F_NEG_INT_INDEX),

      LABEL(F_RETURN),
      LABEL(F_DUMB_RETURN),
      LABEL(F_RETURN_0),
      LABEL(F_RETURN_1),
      LABEL(F_RETURN_LOCAL),
      LABEL(F_RETURN_IF_TRUE),

#include "interpret_protos.h"
    };

#ifdef PIKE_DEBUG
    if (sizeof(table) != (F_MAX_OPCODE-F_OFFSET)*sizeof(void *))
      fatal("opcode_to_label out of sync: 0x%08lx != 0x%08lx\n",
	    DO_NOT_WARN((long)sizeof(table)),
	    DO_NOT_WARN((long)((F_MAX_OPCODE-F_OFFSET)*sizeof(void *))));
#endif /* PIKE_DEBUG */
    fcode_to_opcode = table;
    opcode_to_fcode = lookup;

    qsort(lookup, F_MAX_OPCODE-F_OFFSET, sizeof(struct op_2_f),
	  lookup_sort_fun);

    return 0;
  }
#endif /* HAVE_COMPUTED_GOTO */
}
