/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#include "global.h"
RCSID("$Id: lex.c,v 1.67 1999/11/30 07:50:17 hubbe Exp $");
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

struct keyword instr_names[]=
{
{ "!",			F_NOT,0 },	
{ "!=",			F_NE,0 },	
{ "%",			F_MOD,0 },	
{ "%=",			F_MOD_EQ,0 },	
{ "& global",           F_GLOBAL_LVALUE, I_HASARG },
{ "& lexical local",	F_LEXICAL_LOCAL_LVALUE, I_HASARG },	
{ "& local",            F_LOCAL_LVALUE, I_HASARG },
{ "&",			F_AND,0 },
{ "&=",			F_AND_EQ,0 },	
{ "*",			F_MULTIPLY,0 },	
{ "*=",			F_MULT_EQ,0 },	
{ "+",			F_ADD,0 },	
{ "++x",		F_INC,0 },	
{ "+=",			F_ADD_EQ,0 },	
{ "-",			F_SUBTRACT,0 },	
{ "--x",		F_DEC,0 },	
{ "-=",			F_SUB_EQ,0 },	
{ "/",			F_DIVIDE,0 },	
{ "/=",			F_DIV_EQ,0 },	
{ "<",			F_LT,0 },	
{ "<<",			F_LSH,0 },	
{ "<<=",		F_LSH_EQ,0 },	
{ "<=",			F_LE,0 },	
{ "==",			F_EQ,0 },	
{ ">",			F_GT,0 },	
{ ">=",			F_GE,0 },	
{ ">>",			F_RSH,0 },	
{ ">>=",		F_RSH_EQ,0 },	
{ "@",			F_PUSH_ARRAY,0 },
{ "^",			F_XOR,0 },
{ "^=",			F_XOR_EQ,0 },	
{ "arg+=1024",		F_PREFIX_1024,0 },
{ "arg+=256",		F_PREFIX_256,0 },
{ "arg+=256*X",		F_PREFIX_CHARX256,0 },
{ "arg+=256*XX",	F_PREFIX_WORDX256,0 },
{ "arg+=256*XXX",	F_PREFIX_24BITX256,0 },
{ "arg+=512",		F_PREFIX_512,0 },
{ "arg+=768",		F_PREFIX_768,0 },
{ "assign and pop",	F_ASSIGN_AND_POP,0 },
{ "assign global",	F_ASSIGN_GLOBAL, I_HASARG },
{ "assign global and pop",	F_ASSIGN_GLOBAL_AND_POP, I_HASARG },
{ "assign local",       F_ASSIGN_LOCAL, I_HASARG },
{ "assign local and pop",	F_ASSIGN_LOCAL_AND_POP, I_HASARG },
{ "assign",		F_ASSIGN,0 },
{ "break",		F_BREAK,0 },	
{ "case",		F_CASE,0 },	
{ "cast",		F_CAST,0 },	
{ "const-1",		F_CONST_1,0 },	
{ "constant",           F_CONSTANT, I_HASARG },
{ "continue",		F_CONTINUE,0 },	
{ "copy_value",         F_COPY_VALUE,0 },
{ "default",		F_DEFAULT,0 },	
{ "do-while",		F_DO,0 },	
{ "dumb return",	F_DUMB_RETURN,0 },	
{ "float number",	F_FLOAT,0 },
{ "for",		F_FOR,0 },
{ "global",		F_GLOBAL, I_HASARG },
{ "index",              F_INDEX,0 },
{ "->x",                F_ARROW, I_HASARG },
{ "clear string subtype", F_CLEAR_STRING_SUBTYPE },
{ "arrow string",       F_ARROW_STRING, I_HASARG },
{ "indirect",		F_INDIRECT,0 },

{ "branch",             F_BRANCH, I_ISJUMP },
{ "branch non zero",	F_BRANCH_WHEN_NON_ZERO, I_ISJUMP },	
{ "branch if local",	F_BRANCH_IF_LOCAL, I_HASARG },	
{ "branch if !local",	F_BRANCH_IF_NOT_LOCAL, I_HASARG },	
{ "branch if ! local->x",	F_BRANCH_IF_NOT_LOCAL_ARROW, I_HASARG },	
{ "branch when zero",	F_BRANCH_WHEN_ZERO, I_ISJUMP },	
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

{ "local function call",F_CALL_LFUN, I_HASARG },
{ "local function call and pop",F_CALL_LFUN_AND_POP, I_HASARG },
{ "local function",	F_LFUN, I_HASARG },	
{ "trampoline",         F_TRAMPOLINE, I_HASARG },	
{ "local",		F_LOCAL, I_HASARG },	
{ "lexical local",	F_LEXICAL_LOCAL, I_HASARG },	
{ "external",		F_EXTERNAL, I_HASARG },
{ "& external",		F_EXTERNAL_LVALUE, I_HASARG },
{ "LDA",			F_LDA, I_HASARG },
{ "mark & local",	F_MARK_AND_LOCAL, I_HASARG },	
{ "ltosval2",		F_LTOSVAL2,0 },
{ "lvalue to svalue",	F_LTOSVAL,0 },	
{ "lvalue_list",	F_LVALUE_LIST },	
{ "[ lvalues ]",	F_ARRAY_LVALUE, I_HASARG },	
{ "mark sp-X",          F_MARK_X, I_HASARG },
{ "mark",               F_MARK,0 },
{ "mark mark",          F_MARK2,0 },
{ "pop mark",           F_POP_MARK,0 },
{ "negative number",	F_NEG_NUMBER, I_HASARG },
{ "number",             F_NUMBER, I_HASARG },
{ "pop",		F_POP_VALUE,0 },	
{ "pop_n_elems",        F_POP_N_ELEMS, I_HASARG },
{ "push UNDEFINED",     F_UNDEFINED,0 },
{ "push 0",             F_CONST0,0 },
{ "push 1",             F_CONST1,0 },
{ "push 0x7fffffff",    F_BIGNUM,0 },
{ "range",              F_RANGE,0 },
{ "return",		F_RETURN,0 },
{ "return 0",		F_RETURN_0,0 },
{ "return 1",		F_RETURN_1,0 },
{ "return local",	F_RETURN_LOCAL, I_HASARG },
{ "return if true",	F_RETURN_IF_TRUE, 0 },
{ "sscanf",		F_SSCANF, I_HASARG },	
{ "string",             F_STRING, I_HASARG },
{ "switch",		F_SWITCH, I_HASARG },
{ "unary minus",	F_NEGATE,0 },
{ "while",		F_WHILE,0 },	
{ "x++ and pop",	F_INC_AND_POP,0 },	
{ "x++",		F_POST_INC,0 },	
{ "x-- and pop",	F_DEC_AND_POP,0 },	
{ "x--",		F_POST_DEC,0 },	
{ "|",			F_OR,0 },
{ "|=",			F_OR_EQ,0 },	
{ "~",			F_COMPL,0 },
{ "label",		F_LABEL,I_HASARG },
{ "align",		F_ALIGN, I_HASARG },
{ "call",		F_APPLY, I_HASARG },
{ "clear local",	F_CLEAR_LOCAL, I_HASARG },
{ "clear 2 local",	F_CLEAR_2_LOCAL, I_HASARG },
{ "clear 4 local",	F_CLEAR_4_LOCAL, I_HASARG },
{ "++local",		F_INC_LOCAL, I_HASARG },
{ "++local and pop",	F_INC_LOCAL_AND_POP, I_HASARG },
{ "local++",		F_POST_INC_LOCAL, I_HASARG },
{ "--local",		F_DEC_LOCAL, I_HASARG },
{ "--local and pop",	F_DEC_LOCAL_AND_POP, I_HASARG },
{ "local--",		F_POST_DEC_LOCAL, I_HASARG },
{ "sizeof",		F_SIZEOF,0 },
{ "sizeof local",	F_SIZEOF_LOCAL, I_HASARG },
{ "throw(0)",		F_THROW_ZERO,0 },
{ "string index",       F_STRING_INDEX, I_HASARG },
{ "local index",        F_LOCAL_INDEX, I_HASARG },
{ "local local index",  F_LOCAL_LOCAL_INDEX, I_HASARG },
{ "int index",          F_POS_INT_INDEX, I_HASARG },
{ "-int index",         F_NEG_INT_INDEX, I_HASARG },
{ "apply and pop",      F_APPLY_AND_POP, I_HASARG },
{ "2 locals",           F_2_LOCALS, I_HASARG },
{ "byte",               F_BYTE, I_HASARG },
{ "nop",                F_NOP,0 },
{ "add integer",        F_ADD_INT, I_HASARG },
{ "add -integer",       F_ADD_NEG_INT, I_HASARG },
{ "mark & string",      F_MARK_AND_STRING, I_HASARG },
{ "mark & call",        F_MARK_APPLY, I_HASARG },
{ "mark, call & pop",   F_MARK_APPLY_POP, I_HASARG },
{ "apply and return",   F_APPLY_AND_RETURN, I_HASARG },
{ "apply, assign local and pop",   F_APPLY_ASSIGN_LOCAL_AND_POP, I_HASARG },
{ "apply & assign local",   F_APPLY_ASSIGN_LOCAL, I_HASARG },
{ "call lfun & return", F_CALL_LFUN_AND_RETURN, I_HASARG },
{ "call function",      F_CALL_FUNCTION, 0 },
{ "call function & return", F_CALL_FUNCTION_AND_RETURN, 0 },
{ "+= and pop",         F_ADD_TO_AND_POP, 0 },
{ "local=local;",       F_LOCAL_2_LOCAL, I_HASARG },
{ "local=global;",      F_GLOBAL_2_LOCAL, I_HASARG },
{ "global=local;",      F_LOCAL_2_GLOBAL, I_HASARG },
{ "local->x",           F_LOCAL_ARROW, I_HASARG },
{ "global[local]",      F_GLOBAL_LOCAL_INDEX, I_HASARG },
{ "::`[]",              F_MAGIC_INDEX, I_HASARG },
{ "::`[]=",             F_MAGIC_SET_INDEX, I_HASARG },
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
  for(i=0; i<NELEM(instr_names);i++)
  {
    if(instr_names[i].token >= F_MAX_INSTR)
      fatal("Error in instr_names[%u]\n\n",i);

    if(instrs[instr_names[i].token - F_OFFSET].name)
      fatal("Duplicate name for %s\n",instr_names[i].word);

    instrs[instr_names[i].token - F_OFFSET].name = instr_names[i].word;
    instrs[instr_names[i].token - F_OFFSET].flags=instr_names[i].flags;
  }
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



