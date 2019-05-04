/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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


#define CASE(X)		case (X):
#define DONE		break
#define FETCH

#define LOW_GET_ARG()	((PROG_COUNTER++)[0])
#if PIKE_BYTECODE_METHOD == PIKE_BYTECODE_SPARC
#define LOW_GET_JUMP()	(PROG_COUNTER[0])
#define LOW_SKIPJUMP()	(++PROG_COUNTER)
#else /* PIKE_BYTECODE_METHOD != PIKE_BYTECODE_SPARC */
#define LOW_GET_JUMP()	(INT32)get_unaligned32(PROG_COUNTER)
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


#ifndef STEP_BREAK_LINE
#define STEP_BREAK_LINE
#endif

static int eval_instruction(PIKE_OPCODE_T *pc)
{
  unsigned INT32 prefix2=0,prefix=0;

  debug_malloc_touch(Pike_fp);
  while(1)
  {
    INT32 arg1, arg2;
    PIKE_INSTR_T instr = pc[0];
    Pike_fp->pc = pc++;

    STEP_BREAK_LINE

#ifdef PIKE_DEBUG
    if (d_flag || Pike_interpreter.trace_level > 2)
      low_debug_instr_prologue (instr);
#endif

    switch(instr + F_OFFSET)
    {
      /* NOTE: The prefix handling is not needed in computed-goto mode. */
      /* Support to allow large arguments */
      case F_PREFIX_256: prefix+=256; break;
      case F_PREFIX_512: prefix+=512; break;
      case F_PREFIX_768: prefix+=768; break;
      case F_PREFIX_1024: prefix+=1024; break;
      case F_PREFIX_24BITX256:
        prefix += (unsigned INT32)(pc++)[0]<<24;
      /* FALLTHRU */
      case F_PREFIX_WORDX256:
        prefix += (unsigned INT32)(pc++)[0]<<16;
      /* FALLTHRU */
      case F_PREFIX_CHARX256:
        prefix += (pc++)[0]<<8;
        break;

      /* Support to allow large arguments */
      case F_PREFIX2_256: prefix2+=256; break;
      case F_PREFIX2_512: prefix2+=512; break;
      case F_PREFIX2_768: prefix2+=768; break;
      case F_PREFIX2_1024: prefix2+=1024; break;
      case F_PREFIX2_24BITX256:
        prefix2 += (unsigned INT32)(pc++)[0]<<24;
      /* FALLTHRU */
      case F_PREFIX2_WORDX256:
        prefix2 += (unsigned INT32)(pc++)[0]<<16;
      /* FALLTHRU */
      case F_PREFIX2_CHARX256:
        prefix2 += (pc++)[0]<<8;
      break;


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

#define OPCODE0_RETURN(OP, DESC, FLAGS, CODE) OPCODE0(OP, DESC, FLAGS | I_RETURN, CODE)
#define OPCODE1_RETURN(OP, DESC, FLAGS, CODE) OPCODE1(OP, DESC, FLAGS | I_RETURN, CODE)
#define OPCODE2_RETURN(OP, DESC, FLAGS, CODE) OPCODE2(OP, DESC, FLAGS | I_RETURN, CODE)
#define OPCODE0_TAILRETURN(OP, DESC, FLAGS, CODE) OPCODE0_TAIL(OP, DESC, FLAGS | I_RETURN, CODE)
#define OPCODE1_TAILRETURN(OP, DESC, FLAGS, CODE) OPCODE1_TAIL(OP, DESC, FLAGS | I_RETURN, CODE)
#define OPCODE2_TAILRETURN(OP, DESC, FLAGS, CODE) OPCODE2_TAIL(OP, DESC, FLAGS | I_RETURN, CODE)

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

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 7
# define ADVERTISE_FALLTHROUGH ATTRIBUTE((fallthrough));
#else
# define ADVERTISE_FALLTHROUGH
#endif

#include "interpret_functions.h"

#undef ADVERTISE_FALLTHROUGH

    default:
      Pike_fatal("Strange instruction %ld\n",(long)instr);
    }
  }

  UNREACHABLE(0);

}
