/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: lex.c,v 1.97 2001/07/15 23:14:37 hubbe Exp $");
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
#include "pike_error.h"
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

/*
 * If you have a 64 bit machine and 15+ Gb memory, this
 * routine should handle -p4 nicely. -Hubbe
 * (-p3 only requires ~38Mb on a 32bit machine)
 */

struct instr_counter
{
  long runned;
  struct instr_counter* next[256];
};

int last_instruction[256];
struct instr_counter *instr_counter_storage;

struct instr_counter *init_instr_storage_pointers(int depth)
{
  int e;
  struct instr_counter *d;
  if(!depth) return 0;
  d=ALLOC_STRUCT(instr_counter);
  if(!d)
  {
    fprintf(stderr,"-p%d: out of memory.\n",p_flag);
    exit(2);
  }
  dmalloc_accept_leak(d);
  d->runned=0;
  for(e=0;e<F_MAX_OPCODE-F_OFFSET;e++)
    d->next[e]=init_instr_storage_pointers(depth-1);
  return d;
}

void add_runned(PIKE_OPCODE_T instr)
{
  int e;
  struct instr_counter **tmp=&instr_counter_storage;

  for(e=0;e<p_flag;e++)
  {
    tmp[0]->runned++;
    tmp=tmp[0]->next + last_instruction[e];
    last_instruction[e]=last_instruction[e+1];
  }
#ifndef HAVE_COMPUTED_GOTO
  ((char **)(tmp))[0]++;
#endif /* !HAVE_COMPUTED_GOTO */
  last_instruction[e]=instr;
}

void present_runned(struct instr_counter *d, int depth, int maxdepth)
{
  int e;
  if(depth == maxdepth)
  {
    long runned = depth < p_flag ? d->runned : (long)d;
    if(!runned) return;
    fprintf(stderr,"%010ld @%d@: ",runned,maxdepth);
    for(e=0;e<depth;e++)
    {
      if(e) fprintf(stderr," :: ");
      fprintf(stderr,"%s",
	      low_get_f_name(last_instruction[e] + F_OFFSET,0));
    }
    fprintf(stderr,"\n");
  }else{
    for(e=0;e<F_MAX_OPCODE-F_OFFSET;e++)
    {
      last_instruction[depth]=e;
      present_runned(d->next[e],depth+1, maxdepth);
    }
  }
}

#endif

void exit_lex(void)
{
#ifdef PIKE_DEBUG
  if(p_flag)
  {
    extern void present_constant_profiling(void);
    int e;
    present_constant_profiling();

    fprintf(stderr,"Opcode compiles: (opcode, compiled)\n");
    for(e=0;e<F_MAX_OPCODE-F_OFFSET;e++)
    {
      fprintf(stderr,"%08ld;;%-30s\n",
	      (long)instrs[e].compiles,
	      low_get_f_name(e+F_OFFSET,0));
    }

#ifdef INSTR_PROFILING
    for(e=0;e<=p_flag;e++)
    {
      fprintf(stderr,"Opcode x %d usage:\n",e);
      present_runned(instr_counter_storage, 0, e);
    }
  }
#endif
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
#ifndef PIKE_PRECOMPILER
#include "interpret_protos.h"
#endif /* !PIKE_PRECOMPILER */
{ "%=",			F_MOD_EQ,0 },	
{ "&=",			F_AND_EQ,0 },	
{ "|=",			F_OR_EQ,0 },	
{ "*=",			F_MULT_EQ,0 },	
{ "+=",			F_ADD_EQ,0 },	
{ "-=",			F_SUB_EQ,0 },	
{ "/=",			F_DIV_EQ,0 },	
{ "<<=",		F_LSH_EQ,0 },	
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
{ "for",		F_FOR,0 },

{ "pointer",		F_POINTER, I_ISPOINTER },
{ "data",		F_DATA, I_DATA },
{ "byte",		F_BYTE, I_DATA },
{ "lvalue_list",	F_LVALUE_LIST,0 },	
{ "label",		F_LABEL,I_HASARG },
{ "align",		F_ALIGN, I_HASARG },
{ "nop",                F_NOP,0 },
{ "function start",     F_START_FUNCTION,0 },
{ "notreached!",        F_NOTREACHED, 0 },
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
#ifdef INSTR_PROFILING
  instr_counter_storage=init_instr_storage_pointers(p_flag);
#endif
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
    if(Pike_fp && Pike_fp->context.prog &&
       (int)Pike_fp->context.prog->num_constants > (int)(n-F_MAX_OPCODE) &&
       Pike_fp->context.prog->constants[n-F_MAX_OPCODE].sval.type==T_FUNCTION &&
       Pike_fp->context.prog->constants[n-F_MAX_OPCODE].sval.subtype == FUNCTION_BUILTIN &&
       Pike_fp->context.prog->constants[n-F_MAX_OPCODE].sval.u.efun)
    {
      return Pike_fp->context.prog->constants[n-F_MAX_OPCODE].sval.u.efun->name->str;
    }else{
      sprintf(buf, "Call efun %d", n - F_MAX_OPCODE);
      return buf;
    }
  }else{
    sprintf(buf, "<OTHER %d>", n);
    return buf;
  }
}

#ifdef HAVE_COMPUTED_GOTO
char *get_opcode_name(PIKE_OPCODE_T n)
{
  int fcode;
  int low = 0;
  int high = F_MAX_OPCODE - F_OFFSET;
  static char buf[64];

  if (!n) {
    return "<NULL opcode!>";
  }

  while (low < high) {
    int mid = (low+high)/2;
    if (opcode_to_fcode[mid].opcode < n) {
      low = mid + 1;
    } else if (opcode_to_fcode[mid].opcode > n) {
      high = mid;
    } else {
      return get_f_name(opcode_to_fcode[mid].fcode);
    }
  }

  sprintf(buf, "<Unknown opcode 0x%p>", n);
  return buf;
}
#endif /* HAVE_COMPUTED_GOTO */

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
#include "lexer0.h"
#undef SHIFT
#define SHIFT	1
#include "lexer1.h"
#undef SHIFT
#define SHIFT	2
#include "lexer2.h"
#undef SHIFT

int yylex(YYSTYPE *yylval)
{
#if LEXDEBUG>8
  fprintf(stderr, "YYLEX: Calling lexer at 0x%08lx\n",
	  (long)lex.current_lexer);
#endif /* LEXDEBUG > 8 */
  return(lex.current_lexer(yylval));
}



