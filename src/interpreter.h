/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: interpreter.h,v 1.86 2004/05/29 18:19:25 grubba Exp $
*/

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

#undef JUMP_DONE
#define JUMP_DONE DONE

#ifdef HAVE_COMPUTED_GOTO

#define CASE(OP)	PIKE_CONCAT(LABEL_,OP): FETCH
#define FETCH		(instr = PROG_COUNTER[0])
#ifdef PIKE_DEBUG
#define DONE		continue
#else /* !PIKE_DEBUG */
#define DONE		do {	\
    Pike_fp->pc = PROG_COUNTER++;		\
    goto *instr;		\
  } while(0)
    
#endif /* PIKE_DEBUG */

#define LOW_GET_ARG()	((INT32)(ptrdiff_t)(*(PROG_COUNTER++)))
#define LOW_GET_JUMP()	((INT32)(ptrdiff_t)(*(PROG_COUNTER)))
#define LOW_SKIPJUMP()	(instr = (++PROG_COUNTER)[0])

#define GET_ARG()	LOW_GET_ARG()
#define GET_ARG2()	LOW_GET_ARG()

#else /* !HAVE_COMPUTED_GOTO */

#define CASE(X)		case (X)-F_OFFSET:
#define DONE		break
#define FETCH

#define LOW_GET_ARG()	((PROG_COUNTER++)[0])
#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#define LOW_GET_JUMP()	(PROG_COUNTER[0])
#define LOW_SKIPJUMP()	(++PROG_COUNTER)
#else /* PIKE_BYTECODE_METHOD != PIKE_BYTECODE_SPARC */
#define LOW_GET_JUMP()	EXTRACT_INT(PROG_COUNTER)
#define LOW_SKIPJUMP()	(PROG_COUNTER += sizeof(INT32))
#endif /* PIKE_BYTECODE_METHOD */

#ifdef PIKE_DEBUG

#define GET_ARG() (						\
  instr=prefix,							\
  prefix=0,							\
  instr += LOW_GET_ARG(),					\
  DEBUG_LOG_ARG (instr),					\
  instr)

#define GET_ARG2() (						\
  instr=prefix2,						\
  prefix2=0,							\
  instr += LOW_GET_ARG(),					\
  DEBUG_LOG_ARG2 (instr),					\
  instr)

#else /* !PIKE_DEBUG */

#define GET_ARG() (instr=prefix,prefix=0,instr+LOW_GET_ARG())
#define GET_ARG2() (instr=prefix2,prefix2=0,instr+LOW_GET_ARG())

#endif /* PIKE_DEBUG */

#endif /* HAVE_COMPUTED_GOTO */

#ifndef STEP_BREAK_LINE
#define STEP_BREAK_LINE
#endif

static int eval_instruction(PIKE_OPCODE_T *pc)
{
  PIKE_INSTR_T instr;
#ifdef HAVE_COMPUTED_GOTO
  static void *strap = &&init_strap;
  instr = NULL;
#else /* !HAVE_COMPUTED_GOTO */
  unsigned INT32 prefix2=0,prefix=0;
#endif /* HAVE_COMPUTED_GOTO */

#ifdef HAVE_COMPUTED_GOTO
  goto *strap;
 normal_strap:
#endif /* HAVE_COMPUTED_GOTO */

  debug_malloc_touch(Pike_fp);
  while(1)
  {
    INT32 arg1, arg2;
    instr = pc[0];
    Pike_fp->pc = pc++;

    STEP_BREAK_LINE

#ifdef PIKE_DEBUG
    if (d_flag || Pike_interpreter.trace_level > 2)
      low_debug_instr_prologue (instr);
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

#define OPCODE0(OP, DESC, FLAGS, CODE) CASE(OP); CODE; DONE
#define OPCODE1(OP, DESC, FLAGS, CODE) CASE(OP); { \
    arg1=GET_ARG(); \
    FETCH; \
    CODE; \
  } DONE

#define OPCODE2(OP, DESC, FLAGS, CODE) CASE(OP); { \
    arg1=GET_ARG(); \
    arg2=GET_ARG2(); \
    FETCH; \
    CODE; \
  } DONE

#define OPCODE0_ALIAS(OP, DESC, FLAGS, FUN) OPCODE0(OP, DESC, FLAGS, {FUN();})
#define OPCODE1_ALIAS(OP, DESC, FLAGS, FUN) OPCODE1(OP, DESC, FLAGS, {FUN(arg1);})
#define OPCODE2_ALIAS(OP, DESC, FLAGS, FUN) OPCODE2(OP, DESC, FLAGS, {FUN(arg1, arg2);})

#define OPCODE0_TAIL(OP, DESC, FLAGS, CODE) CASE(OP); CODE
#define OPCODE1_TAIL(OP, DESC, FLAGS, CODE) CASE(OP); CODE
#define OPCODE2_TAIL(OP, DESC, FLAGS, CODE) CASE(OP); CODE

#define OPCODE0_JUMP		OPCODE0
#define OPCODE1_JUMP		OPCODE1
#define OPCODE2_JUMP		OPCODE2
#define OPCODE0_TAILJUMP	OPCODE0_TAIL
#define OPCODE1_TAILJUMP	OPCODE1_TAIL
#define OPCODE2_TAILJUMP	OPCODE2_TAIL

#define OPCODE0_RETURN(OP, DESC, FLAGS, CODE) OPCODE0(OP, DESC, FLAGS, CODE)
#define OPCODE1_RETURN(OP, DESC, FLAGS, CODE) OPCODE1(OP, DESC, FLAGS, CODE)
#define OPCODE2_RETURN(OP, DESC, FLAGS, CODE) OPCODE2(OP, DESC, FLAGS, CODE)
#define OPCODE0_TAILRETURN(OP, DESC, FLAGS, CODE) OPCODE0_TAIL(OP, DESC, FLAGS, CODE)
#define OPCODE1_TAILRETURN(OP, DESC, FLAGS, CODE) OPCODE1_TAIL(OP, DESC, FLAGS, CODE)
#define OPCODE2_TAILRETURN(OP, DESC, FLAGS, CODE) OPCODE2_TAIL(OP, DESC, FLAGS, CODE)

#define OPCODE0_PTRJUMP(OP, DESC, FLAGS, CODE) CASE(OP); CODE; DONE
#define OPCODE0_TAILPTRJUMP(OP, DESC, FLAGS, CODE) CASE(OP); CODE

/* These are something of a special case as they
 * requires a POINTER stored explicitly after
 * the instruction itself.
 */
#define OPCODE1_PTRJUMP(OP, DESC, FLAGS, CODE) CASE(OP); { \
    arg1=GET_ARG(); \
    FETCH; \
    CODE; \
  } DONE

#define OPCODE2_PTRJUMP(OP, DESC, FLAGS, CODE) CASE(OP); { \
    arg1=GET_ARG(); \
    arg2=GET_ARG2(); \
    FETCH; \
    CODE; \
  } DONE

#define OPCODE1_TAILPTRJUMP(OP, DESC, FLAGS, CODE) CASE(OP); CODE
#define OPCODE2_TAILPTRJUMP(OP, DESC, FLAGS, CODE) CASE(OP); CODE

#define OPCODE0_BRANCH		OPCODE0_PTRJUMP
#define OPCODE1_BRANCH		OPCODE1_PTRJUMP
#define OPCODE2_BRANCH		OPCODE2_PTRJUMP
#define OPCODE0_TAILBRANCH	OPCODE0_TAILPTRJUMP
#define OPCODE1_TAILBRANCH	OPCODE1_TAILPTRJUMP
#define OPCODE2_TAILBRANCH	OPCODE2_TAILPTRJUMP

#include "interpret_functions.h"

#ifndef HAVE_COMPUTED_GOTO      
    default:
      Pike_fatal("Strange instruction %ld\n",(long)instr);
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
#undef OPCODE0_PTRJUMP
#undef OPCODE1_PTRJUMP
#undef OPCODE2_PTRJUMP
#undef OPCODE0_TAILPTRJUMP
#undef OPCODE1_TAILPTRJUMP
#undef OPCODE2_TAILPTRJUMP
#undef OPCODE0_RETURN
#undef OPCODE1_RETURN
#undef OPCODE2_RETURN
#undef OPCODE0_TAILRETURN
#undef OPCODE1_TAILRETURN
#undef OPCODE2_TAILRETURN
#undef OPCODE0_BRANCH
#undef OPCODE1_BRANCH
#undef OPCODE2_BRANCH
#undef OPCODE0_TAILBRANCH
#undef OPCODE1_TAILBRANCH
#undef OPCODE2_TAILBRANCH
  /* NOTE: No need to redefine these.
   * #undef OPCODE0_ALIAS
   * #undef OPCODE1_ALIAS
   * #undef OPCODE2_ALIAS
   */
#undef LABEL
#define LABEL(OP)			&&PIKE_CONCAT(LABEL_,OP)
#define NULL_LABEL(OP)			NULL
#define OPCODE0(OP,DESC)		LABEL(OP),
#define OPCODE1(OP,DESC)		LABEL(OP),
#define OPCODE2(OP,DESC)		LABEL(OP),
#define OPCODE0_TAIL(OP,DESC)		LABEL(OP),
#define OPCODE1_TAIL(OP,DESC)		LABEL(OP),
#define OPCODE2_TAIL(OP,DESC)		LABEL(OP),
#define OPCODE0_PTRJUMP(OP,DESC)	LABEL(OP),
#define OPCODE1_PTRJUMP(OP,DESC)	LABEL(OP),
#define OPCODE2_PTRJUMP(OP,DESC)	LABEL(OP),
#define OPCODE0_TAILPTRJUMP(OP,DESC)	LABEL(OP),
#define OPCODE1_TAILPTRJUMP(OP,DESC)	LABEL(OP),
#define OPCODE2_TAILPTRJUMP(OP,DESC)	LABEL(OP),
#define OPCODE0_RETURN(OP,DESC)		LABEL(OP),
#define OPCODE1_RETURN(OP,DESC)		LABEL(OP),
#define OPCODE2_RETURN(OP,DESC)		LABEL(OP),
#define OPCODE0_TAILRETURN(OP,DESC)	LABEL(OP),
#define OPCODE1_TAILRETURN(OP,DESC)	LABEL(OP),
#define OPCODE2_TAILRETURN(OP,DESC)	LABEL(OP),

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

#include "interpret_protos.h"
    };

#ifdef PIKE_DEBUG
    if (sizeof(table) != (F_MAX_OPCODE-F_OFFSET)*sizeof(void *))
      Pike_fatal("opcode_to_label out of sync: 0x%08lx != 0x%08lx\n",
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
