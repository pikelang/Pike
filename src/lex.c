/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: lex.c,v 1.80 2000/05/11 14:09:45 grubba Exp $");
#include "language.h"
#include "array.h"
#include "lex.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "constants.h"
#include "hashtable.h"
#include "stuff.h"
#include "pike_memory.h"
#include "interpret.h"
#include "error.h"
#include "object.h"
#include "las.h"
#include "operators.h"
#include "opcodes.h"
#include "builtin_functions.h"
#include "main.h"
#include "mapping.h"

#include "pike_macros.h"

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include "time_stuff.h"

#define LEXDEBUG 0

#ifdef INSTR_PROFILING
int last_instruction=0;
#endif

void exit_lex(void)
{
#ifdef PIKE_DEBUG
  if(p_flag)
  {
    int e;
    fprintf(stderr,"Opcode usage: (opcode, runned, compiled)\n");
    for(e=0;e<F_MAX_OPCODE-F_OFFSET;e++)
    {
      fprintf(stderr,":: %-30s %8ld %8ld\n",
	      low_get_f_name(e+F_OFFSET,0),
	      (long)instrs[e].runs,
	      (long)instrs[e].compiles);
    }

#ifdef INSTR_PROFILING
    for(e=0;e<F_MAX_OPCODE-F_OFFSET;e++)
    {
      int d;
      for(d=0;d<256;d++)
	if(instrs[e].reruns[d])
	  fprintf(stderr,"%010ld::%s - %s\n",instrs[e].reruns[d],low_get_f_name(e+F_OFFSET,0),low_get_f_name(d+F_OFFSET,0));
    }
#endif
  }
#endif
}

#define OPCODE0(OP,DESC) { DESC, OP, 0 },
#define OPCODE1(OP,DESC) { DESC, OP, I_HASARG },
#define OPCODE2(OP,DESC) { DESC, OP, I_TWO_ARGS },

#define OPCODE0_TAIL(OP,DESC) { DESC, OP, 0 },
#define OPCODE1_TAIL(OP,DESC) { DESC, OP, I_HASARG },
#define OPCODE2_TAIL(OP,DESC) { DESC, OP, I_TWO_ARGS },

#define OPCODE0_JUMP(OP,DESC) { DESC, OP, I_ISJUMP },
#define OPCODE1_JUMP(OP,DESC) { DESC, OP, I_HASARG },
#define OPCODE2_JUMP(OP,DESC) { DESC, OP, I_TWO_ARGS },

#define OPCODE0_TAILJUMP(OP,DESC) { DESC, OP, I_ISJUMP },
#define OPCODE1_TAILJUMP(OP,DESC) { DESC, OP, I_HASARG },
#define OPCODE2_TAILJUMP(OP,DESC) { DESC, OP, I_TWO_ARGS },

#define LEXER

struct keyword instr_names[]=
{
#include "interpret_protos.h"
{ "!=",			F_NE,0 },	
{ "%=",			F_MOD_EQ,0 },	
{ "&=",			F_AND_EQ,0 },	
{ "|=",			F_OR_EQ,0 },	
{ "*=",			F_MULT_EQ,0 },	
{ "+=",			F_ADD_EQ,0 },	
{ "-=",			F_SUB_EQ,0 },	
{ "/=",			F_DIV_EQ,0 },	
{ "<",			F_LT,0 },	
{ "<<=",		F_LSH_EQ,0 },	
{ "<=",			F_LE,0 },	
{ "==",			F_EQ,0 },	
{ ">",			F_GT,0 },	
{ ">=",			F_GE,0 },	
{ ">>=",		F_RSH_EQ,0 },	
{ "^=",			F_XOR_EQ,0 },	
{ "arg+=1024",		F_PREFIX_1024,0 },
{ "arg+=256",		F_PREFIX_256,0 },
{ "arg+=256*X",		F_PREFIX_CHARX256,0 },
{ "arg+=256*XX",	F_PREFIX_WORDX256,0 },
{ "arg+=256*XXX",	F_PREFIX_24BITX256,0 },
{ "arg+=512",		F_PREFIX_512,0 },
{ "arg+=768",		F_PREFIX_768,0 },

{ "arg+=1024",		F_PREFIX2_1024,0 },
{ "arg+=256",		F_PREFIX2_256,0 },
{ "arg+=256*X",		F_PREFIX2_CHARX256,0 },
{ "arg+=256*XX",	F_PREFIX2_WORDX256,0 },
{ "arg+=256*XXX",	F_PREFIX2_24BITX256,0 },
{ "arg+=512",		F_PREFIX2_512,0 },
{ "arg+=768",		F_PREFIX2_768,0 },

{ "break",		F_BREAK,0 },	
{ "case",		F_CASE,0 },	
{ "continue",		F_CONTINUE,0 },	
{ "default",		F_DEFAULT,0 },	
{ "do-while",		F_DO,0 },	
{ "dumb return",	F_DUMB_RETURN,0 },	
{ "for",		F_FOR,0 },
{ "index",              F_INDEX,0 },

{ "branch if !local",	F_BRANCH_IF_NOT_LOCAL, I_HASARG },	
{ "branch if <",	F_BRANCH_WHEN_LT, I_ISJUMP },
{ "branch if >",	F_BRANCH_WHEN_GT, I_ISJUMP },
{ "branch if <=",	F_BRANCH_WHEN_LE, I_ISJUMP },
{ "branch if >=",	F_BRANCH_WHEN_GE, I_ISJUMP },
{ "branch if ==",	F_BRANCH_WHEN_EQ, I_ISJUMP },
{ "branch if !=",	F_BRANCH_WHEN_NE, I_ISJUMP },
{ "branch & pop if zero",	F_BRANCH_AND_POP_WHEN_ZERO, I_ISJUMP },
{ "branch & pop if !zero",	F_BRANCH_AND_POP_WHEN_NON_ZERO, I_ISJUMP },
{ "++Loop",		F_INC_LOOP, I_ISJUMP },	
{ "++Loop!=",		F_INC_NEQ_LOOP, I_ISJUMP },
{ "--Loop",		F_DEC_LOOP, I_ISJUMP },	
{ "--Loop!=",		F_DEC_NEQ_LOOP, I_ISJUMP },
{ "&&",			F_LAND, I_ISJUMP },	
{ "||",			F_LOR, I_ISJUMP },	
{ "==||",               F_EQ_OR, I_ISJUMP },
{ "==&&",               F_EQ_AND, I_ISJUMP },
{ "catch",		F_CATCH, I_ISJUMP },
{ "foreach",		F_FOREACH, I_ISJUMP },
{ "pointer",		F_POINTER, I_ISPOINTER },
{ "data",		F_DATA, I_DATA },
{ "byte",		F_BYTE, I_DATA },

{ "lvalue_list",	F_LVALUE_LIST,0 },	
{ "return",		F_RETURN,0 },
{ "return 0",		F_RETURN_0,0 },
{ "return 1",		F_RETURN_1,0 },
{ "return local",	F_RETURN_LOCAL, I_HASARG },
{ "return if true",	F_RETURN_IF_TRUE, 0 },
{ "label",		F_LABEL,I_HASARG },
{ "align",		F_ALIGN, I_HASARG },
{ "call",		F_APPLY, I_HASARG },
{ "int index",          F_POS_INT_INDEX, I_HASARG },
{ "-int index",         F_NEG_INT_INDEX, I_HASARG },
{ "apply and pop",      F_APPLY_AND_POP, I_HASARG },
{ "nop",                F_NOP,0 },
{ "function start",     F_START_FUNCTION,0 },
{ "apply and return",   F_APPLY_AND_RETURN, I_HASARG },
{ "call function",      F_CALL_FUNCTION, 0 },
{ "call function & return", F_CALL_FUNCTION_AND_RETURN, 0 },
};

struct instr instrs[F_MAX_INSTR - F_OFFSET];

struct reserved
{
  struct hash_entry link;
  int token;
};

void init_lex()
{
  unsigned int i;
#ifdef PIKE_DEBUG
  int fatal_later=0;
#endif

  for(i=0; i<NELEM(instr_names);i++)
  {
#ifdef PIKE_DEBUG
    if(instr_names[i].token >= F_MAX_INSTR)
    {
      fprintf(stderr,"Error in instr_names[%u]\n\n",i);
      fatal_later++;
    }

    if(instrs[instr_names[i].token - F_OFFSET].name)
    {
      fprintf(stderr,"Duplicate name for %s\n",instr_names[i].word);
      fatal_later++;
    }
#endif

    instrs[instr_names[i].token - F_OFFSET].name = instr_names[i].word;
    instrs[instr_names[i].token - F_OFFSET].flags=instr_names[i].flags;
  }

#ifdef PIKE_DEBUG
  for(i=1; i<F_MAX_OPCODE-F_OFFSET;i++)
  {
    if(!instrs[i].name)
    {
      fprintf(stderr,"Opcode %d does not have a name.\n",i);
      fatal_later++;
    }
  }
  if(fatal_later)
    fatal("Found %d errors in instrs.\n",fatal_later);

#endif

}

char *low_get_f_name(int n,struct program *p)
{
  static char buf[30];
  
  if (n<F_MAX_OPCODE && instrs[n-F_OFFSET].name)
  {
    return instrs[n-F_OFFSET].name;
  }else if(n >= F_MAX_OPCODE) {
    if(p &&
       (int)p->num_constants > (int)(n-F_MAX_OPCODE) &&
       p->constants[n-F_MAX_OPCODE].sval.type==T_FUNCTION &&
       (p->constants[n-F_MAX_OPCODE].sval.subtype == FUNCTION_BUILTIN) &&
       p->constants[n-F_MAX_OPCODE].sval.u.efun)
    {
      return p->constants[n-F_MAX_OPCODE].sval.u.efun->name->str;
    }else{
      sprintf(buf, "Call efun %d", n - F_MAX_OPCODE);
      return buf;
    }
  }else{
    sprintf(buf, "<OTHER %d>", n);
    return buf;
  }
}

char *get_f_name(int n)
{
  static char buf[30];
  if (n<F_MAX_OPCODE && instrs[n-F_OFFSET].name)
  {
    return instrs[n-F_OFFSET].name;
  }else if(n >= F_MAX_OPCODE) {
    if(fp && fp->context.prog &&
       (int)fp->context.prog->num_constants > (int)(n-F_MAX_OPCODE) &&
       fp->context.prog->constants[n-F_MAX_OPCODE].sval.type==T_FUNCTION &&
       fp->context.prog->constants[n-F_MAX_OPCODE].sval.subtype == FUNCTION_BUILTIN &&
       fp->context.prog->constants[n-F_MAX_OPCODE].sval.u.efun)
    {
      return fp->context.prog->constants[n-F_MAX_OPCODE].sval.u.efun->name->str;
    }else{
      sprintf(buf, "Call efun %d", n - F_MAX_OPCODE);
      return buf;
    }
  }else{
    sprintf(buf, "<OTHER %d>", n);
    return buf;
  }
}

char *get_token_name(int n)
{
  static char buf[30];
  if (n<F_MAX_INSTR && instrs[n-F_OFFSET].name)
  {
    return instrs[n-F_OFFSET].name;
  }else{
    sprintf(buf, "<OTHER %d>", n);
    return buf;
  }
}

struct lex lex;

/* Make lexers for shifts 0, 1 and 2. */

#define SHIFT	0
#include "lexer.h"
#undef SHIFT
#define SHIFT	1
#include "lexer.h"
#undef SHIFT
#define SHIFT	2
#include "lexer.h"
#undef SHIFT

int yylex(YYSTYPE *yylval)
{
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX: Calling lexer at 0x%08lx\n",
	  (long)lex.current_lexer);
#endif /* LEXDEBUG > 8 */
  return(lex.current_lexer(yylval));
}



