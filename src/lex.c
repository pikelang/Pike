/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: lex.c,v 1.21.2.4 1997/07/09 07:45:01 hubbe Exp $");
#include "language.h"
#include "array.h"
#include "lex.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "constants.h"
#include "hashtable.h"
#include "stuff.h"
#include "memory.h"
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
#include <sys/param.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include "time_stuff.h"

#define LEXDEBUG 0

void exit_lex(void)
{
#ifdef DEBUG
  if(p_flag)
  {
    int e;
    fprintf(stderr,"Opcode usage: (opcode, runned, compiled)\n");
    for(e=0;e<F_MAX_OPCODE-F_OFFSET;e++)
    {
      fprintf(stderr,":: %-20s %8ld %8ld\n",
	      low_get_f_name(e+F_OFFSET,0),
	      (long)instrs[e].runs,
	      (long)instrs[e].compiles);
    }
  }
#endif
}

struct keyword instr_names[]=
{
{ "!",			F_NOT },	
{ "!=",			F_NE },	
{ "%",			F_MOD },	
{ "%=",			F_MOD_EQ },	
{ "& global",           F_GLOBAL_LVALUE, I_HASARG },
{ "& local",            F_LOCAL_LVALUE, I_HASARG },
{ "&",			F_AND },
{ "&=",			F_AND_EQ },	
{ "*",			F_MULTIPLY },	
{ "*=",			F_MULT_EQ },	
{ "+",			F_ADD },	
{ "++x",		F_INC },	
{ "+=",			F_ADD_EQ },	
{ "-",			F_SUBTRACT },	
{ "--x",		F_DEC },	
{ "-=",			F_SUB_EQ },	
{ "/",			F_DIVIDE },	
{ "/=",			F_DIV_EQ },	
{ "<",			F_LT },	
{ "<<",			F_LSH },	
{ "<<=",		F_LSH_EQ },	
{ "<=",			F_LE },	
{ "==",			F_EQ },	
{ ">",			F_GT },	
{ ">=",			F_GE },	
{ ">>",			F_RSH },	
{ ">>=",		F_RSH_EQ },	
{ "@",			F_PUSH_ARRAY },
{ "^",			F_XOR },
{ "^=",			F_XOR_EQ },	
{ "arg+=1024",		F_PREFIX_1024 },
{ "arg+=256",		F_PREFIX_256 },
{ "arg+=256*X",		F_PREFIX_CHARX256 },
{ "arg+=256*XX",	F_PREFIX_WORDX256 },
{ "arg+=256*XXX",	F_PREFIX_24BITX256 },
{ "arg+=512",		F_PREFIX_512 },
{ "arg+=768",		F_PREFIX_768 },
{ "assign and pop",	F_ASSIGN_AND_POP },
{ "assign global",	F_ASSIGN_GLOBAL, I_HASARG },
{ "assign global and pop",	F_ASSIGN_GLOBAL_AND_POP, I_HASARG },
{ "assign local",       F_ASSIGN_LOCAL, I_HASARG },
{ "assign local and pop",	F_ASSIGN_LOCAL_AND_POP, I_HASARG },
{ "assign",		F_ASSIGN },
{ "break",		F_BREAK },	
{ "case",		F_CASE },	
{ "cast",		F_CAST },	
{ "const-1",		F_CONST_1 },	
{ "constant",           F_CONSTANT, I_HASARG },
{ "continue",		F_CONTINUE },	
{ "copy_value",         F_COPY_VALUE },
{ "default",		F_DEFAULT },	
{ "do-while",		F_DO },	
{ "dumb return",	F_DUMB_RETURN },	
{ "float number",	F_FLOAT },
{ "for",		F_FOR },
{ "global",		F_GLOBAL, I_HASARG },
{ "index",              F_INDEX },
{ "->",                 F_ARROW, I_HASARG },
{ "clear string subtype", F_CLEAR_STRING_SUBTYPE },
{ "arrow string",       F_ARROW_STRING, I_HASARG },
{ "indirect",		F_INDIRECT },

{ "branch",             F_BRANCH, I_ISJUMP },
{ "branch non zero",	F_BRANCH_WHEN_NON_ZERO, I_ISJUMP },	
{ "branch when zero",	F_BRANCH_WHEN_ZERO, I_ISJUMP },	
{ "branch if <",	F_BRANCH_WHEN_LT, I_ISJUMP },
{ "branch if >",	F_BRANCH_WHEN_GT, I_ISJUMP },
{ "branch if <=",	F_BRANCH_WHEN_LE, I_ISJUMP },
{ "branch if >=",	F_BRANCH_WHEN_GE, I_ISJUMP },
{ "branch if ==",	F_BRANCH_WHEN_EQ, I_ISJUMP },
{ "branch if !=",	F_BRANCH_WHEN_NE, I_ISJUMP },
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
{ "data",		F_POINTER, I_ISPOINTER },

{ "local function call",F_CALL_LFUN, I_HASARG },
{ "local function call and pop",F_CALL_LFUN_AND_POP, I_HASARG },
{ "local function",	F_LFUN, I_HASARG },	
{ "local",		F_LOCAL, I_HASARG },	
{ "external",		F_EXTERNAL, I_HASARG },
{ "& external",		F_EXTERNAL_LVALUE, I_HASARG },
{ "LDA",			F_LDA, I_HASARG },
{ "mark & local",	F_MARK_AND_LOCAL, I_HASARG },	
{ "ltosval2",		F_LTOSVAL2 },
{ "lvalue to svalue",	F_LTOSVAL },	
{ "lvalue_list",	F_LVALUE_LIST },	
{ "mark",               F_MARK },
{ "mark mark",          F_MARK2 },
{ "negative number",	F_NEG_NUMBER, I_HASARG },
{ "number",             F_NUMBER, I_HASARG },
{ "pop",		F_POP_VALUE },	
{ "pop_n_elems",        F_POP_N_ELEMS, I_HASARG },
{ "push 0",             F_CONST0 },
{ "push 1",             F_CONST1 },
{ "push 0x7fffffff",    F_BIGNUM },
{ "range",              F_RANGE },
{ "return",		F_RETURN },
{ "return 0",		F_RETURN_0 },
{ "return 1",		F_RETURN_1 },
{ "sscanf",		F_SSCANF, I_HASARG },	
{ "string",             F_STRING, I_HASARG },
{ "switch",		F_SWITCH, I_HASARG },
{ "unary minus",	F_NEGATE },
{ "while",		F_WHILE },	
{ "x++ and pop",	F_INC_AND_POP },	
{ "x++",		F_POST_INC },	
{ "x-- and pop",	F_DEC_AND_POP },	
{ "x--",		F_POST_DEC },	
{ "|",			F_OR },
{ "|=",			F_OR_EQ },	
{ "~",			F_COMPL },
{ "label",		F_LABEL,1 },
{ "align",		F_ALIGN, I_HASARG },
{ "call",		F_APPLY, I_HASARG },
{ "clear local",	F_CLEAR_LOCAL, I_HASARG },
{ "clear 2 local",	F_CLEAR_2_LOCAL, I_HASARG },
{ "++local",		F_INC_LOCAL, I_HASARG },
{ "++local and pop",	F_INC_LOCAL_AND_POP, I_HASARG },
{ "local++",		F_POST_INC_LOCAL, I_HASARG },
{ "--local",		F_DEC_LOCAL, I_HASARG },
{ "--local and pop",	F_DEC_LOCAL_AND_POP, I_HASARG },
{ "local--",		F_POST_DEC_LOCAL, I_HASARG },
{ "sizeof",		F_SIZEOF },
{ "sizeof local",	F_SIZEOF_LOCAL, I_HASARG },
{ "throw(0)",		F_THROW_ZERO },
{ "string index",       F_STRING_INDEX, I_HASARG },
{ "local index",        F_LOCAL_INDEX, I_HASARG },
{ "int index",          F_POS_INT_INDEX, I_HASARG },
{ "-int index",         F_NEG_INT_INDEX, I_HASARG },
{ "apply and pop",      F_APPLY_AND_POP, I_HASARG },
{ "2 locals",           F_2_LOCALS, I_HASARG },
{ "byte",               F_BYTE, I_HASARG },
{ "nop",                F_NOP },
{ "add integer",        F_ADD_INT, I_HASARG },
{ "add -integer",       F_ADD_NEG_INT, I_HASARG },
{ "mark & call",        F_MARK_APPLY, I_HASARG },
{ "mark, call & pop",   F_MARK_APPLY_POP, I_HASARG },
{ "apply and return",   F_APPLY_AND_RETURN, I_HASARG },
{ "call lfun & return", F_CALL_LFUN_AND_RETURN, I_HASARG },
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
       p->constants[n-F_MAX_OPCODE].type==T_FUNCTION &&
       (p->constants[n-F_MAX_OPCODE].subtype == FUNCTION_BUILTIN) &&
       p->constants[n-F_MAX_OPCODE].u.efun)
    {
      return p->constants[n-F_MAX_OPCODE].u.efun->name->str;
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
       fp->context.prog->constants[n-F_MAX_OPCODE].type==T_FUNCTION &&
       fp->context.prog->constants[n-F_MAX_OPCODE].subtype == FUNCTION_BUILTIN &&
       fp->context.prog->constants[n-F_MAX_OPCODE].u.efun)
    {
      return fp->context.prog->constants[n-F_MAX_OPCODE].u.efun->name->str;
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

#define LOOK() (*(lex.pos))
#define GETC() (*(lex.pos++))
#define GOBBLE(c) (LOOK()==c?(lex.pos++,1):0)
#define SKIPSPACE() do { while(ISSPACE(LOOK()) && LOOK()!='\n') lex.pos++; }while(0)
#define SKIPWHITE() do { while(ISSPACE(LOOK())) lex.pos++; }while(0)
#define SKIPUPTO(X) do { while(LOOK()!=(X) && LOOK()) lex.pos++; }while(0)

#define READBUF(X) {				\
  register int C;				\
  buf=lex.pos;					\
  while((C=LOOK()) && (X)) lex.pos++;		\
  len=lex.pos - buf;				\
}

#define TWO_CHAR(X,Y) ((X)<<8)+(Y)
#define ISWORD(X) (len==(long)strlen(X) && !MEMCMP(buf,X,strlen(X)))

/*** Lexical analyzing ***/

static int char_const(void)
{
  int c;
  switch(c=GETC())
  {
  case 0:
    lex.pos--;
    yyerror("Unexpected end of file\n");
    return 0;
    
  case '0': case '1': case '2': case '3':
  case '4': case '5': case '6': case '7':
    c-='0';
      if(LOOK()<'0' || LOOK()>'8') return c;
      c=c*8+(GETC()-'0');
      if(LOOK()<'0' || LOOK()>'8') return c;
      c=c*8+(GETC()-'0');
      return c;
      
  case 'r': return '\r';
  case 'n': return '\n';
  case 't': return '\t';
  case 'b': return '\b';

  case '\n':
    lex.current_line++;
    return '\n';
    
  }
  return c;
}

static struct pike_string *readstring(void)
{
  int c;
  dynamic_buffer tmp;
  initialize_buf(&tmp);
  
  while(1)
  {
    switch(c=GETC())
    {
    case 0:
      lex.pos--;
      yyerror("End of file in string.");
      break;
      
    case '\n':
      lex.current_line++;
      yyerror("Newline in string.");
      break;
      
    case '\\':
      low_my_putchar(char_const(),&tmp);
      continue;
      
    case '"':
      break;
      
    default:
      low_my_putchar(c,&tmp);
      continue;
    }
    break;
  }
  return low_free_buf(&tmp);
}

int yylex(YYSTYPE *yylval)
#if LEXDEBUG>4
{
  int t;
  int yylex2(YYSTYPE *);
  t=yylex2(yylval);
  if(t<256)
  {
    fprintf(stderr,"yylex() -> '%c' (%d)\n",t,t);
  }else{
    fprintf(stderr,"yylex() -> %s (%d)\n",get_f_name(t),t);
  }
  return t;
}

static int yylex2(YYSTYPE *yylval)
#endif
{
  INT32 c,len;
  char *buf;

#ifdef __CHECKER__
  MEMSET((char *)yylval,0,sizeof(YYSTYPE));
#endif
#ifdef MALLOC_DEBUG
  check_sfltable();
#endif

  while(1)
  {
    switch(c=GETC())
    {
    case 0:
      lex.pos--;
      return 0;

    case '\n':
      lex.current_line++;
      continue;

    case '#':
      SKIPSPACE();
      READBUF(C!=' ' && C!='\t' && C!='\n');

      switch(len>0?buf[0]:0)
      {
	char *p;
	
      case 'l':
	if(!ISWORD("line")) goto badhash;
	READBUF(C!=' ' && C!='\t' && C!='\n');
	
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	lex.current_line=atoi(buf)-1;
	SKIPSPACE();
	if(GOBBLE('"'))
	{
	  struct pike_string *tmp=readstring();
	  free_string(lex.current_file);
	  lex.current_file=tmp;
	}
	break;
	
      case 'e':
	if(ISWORD("error"))
	{
	  SKIPSPACE();
	  READBUF(C!='\n');
	  yyerror(buf);
	  break;
	}
	goto badhash;

      case 'p':
	if(ISWORD("pragma"))
	{
	  SKIPSPACE();
	  READBUF(C!='\n');
	  if (strcmp(buf, "all_inline") == 0)
	  {
	    lex.pragmas |= PRAGMA_ALL_INLINE;
	  }
	  break;
	}
	
      badhash:
	my_yyerror("Unknown directive #%s.",buf);
	SKIPUPTO('\n');
	continue;
      }
      continue;

    case ' ':
    case '\t':
      continue;

    case '\'':
      switch(c=GETC())
      {
      case 0:
	lex.pos--;
	yyerror("Unexpected end of file\n");
	break;

	case '\\':
	  c=char_const();
      }
      if(!GOBBLE('\''))
	yyerror("Unterminated character constant.");
      yylval->number=c;
      return F_NUMBER;
	
    case '"':
      yylval->string=readstring();
      return F_STRING;
  
    case ':':
      if(GOBBLE(':')) return F_COLON_COLON;
      return c;

    case '.':
      if(GOBBLE('.'))
      {
	if(GOBBLE('.')) return F_DOT_DOT_DOT;
	return F_DOT_DOT;
      }
      return c;
  
    case '0':
      if(GOBBLE('x') || GOBBLE('X'))
      {
	yylval->number=STRTOL(lex.pos, &lex.pos, 16);
	return F_NUMBER;
      }
  
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      char *p1, *p2;
      double f;
      long l;
      lex.pos--;
      f=my_strtod(lex.pos, &p1);
      l=STRTOL(lex.pos, &p2, 0);

      if(p1>p2)
      {
	lex.pos=p1;
	yylval->fnum=(float)f;
	return F_FLOAT;
      }else{
	lex.pos=p2;
	yylval->number=l;
	return F_NUMBER;
      }
  
    case '-':
      if(GOBBLE('=')) return F_SUB_EQ;
      if(GOBBLE('>')) return F_ARROW;
      if(GOBBLE('-')) return F_DEC;
      return '-';
  
    case '+':
      if(GOBBLE('=')) return F_ADD_EQ;
      if(GOBBLE('+')) return F_INC;
      return '+';
  
    case '&':
      if(GOBBLE('=')) return F_AND_EQ;
      if(GOBBLE('&')) return F_LAND;
      return '&';
  
    case '|':
      if(GOBBLE('=')) return F_OR_EQ;
      if(GOBBLE('|')) return F_LOR;
      return '|';

    case '^':
      if(GOBBLE('=')) return F_XOR_EQ;
      return '^';
  
    case '*':
      if(GOBBLE('=')) return F_MULT_EQ;
      return '*';

    case '%':
      if(GOBBLE('=')) return F_MOD_EQ;
      return '%';
  
    case '/':
      if(GOBBLE('=')) return F_DIV_EQ;
      return '/';
  
    case '=':
      if(GOBBLE('=')) return F_EQ;
      return '=';
  
    case '<':
      if(GOBBLE('<'))
      {
	if(GOBBLE('=')) return F_LSH_EQ;
	return F_LSH;
      }
      if(GOBBLE('=')) return F_LE;
      return '<';
  
    case '>':
      if(GOBBLE(')')) return F_MULTISET_END;
      if(GOBBLE('=')) return F_GE;
      if(GOBBLE('>'))
      {
	if(GOBBLE('=')) return F_RSH_EQ;
	return F_RSH;
      }
      return '>';

    case '!':
      if(GOBBLE('=')) return F_NE;
      return F_NOT;

    case '(':
      if(GOBBLE('<')) return F_MULTISET_START;
      return '(';

    case '?':
    case ',':
    case '~':
    case '@':
    case ')':
    case '[':
    case ']':
    case '{':
    case ';':
    case '}': return c;

    case '`':
    {
      char *tmp;
      switch(GETC())
      {
      case '+': tmp="`+"; break;
      case '/': tmp="`/"; break;
      case '%': tmp="`%"; break;
      case '*': tmp="`*"; break;
      case '&': tmp="`&"; break;
      case '|': tmp="`|"; break;
      case '^': tmp="`^"; break;
      case '~': tmp="`~"; break;
      case '<':
	if(GOBBLE('<')) { tmp="`<<"; break; }
	if(GOBBLE('=')) { tmp="`<="; break; }
	tmp="`<";
	break;

      case '>':
	if(GOBBLE('>')) { tmp="`>>"; break; }
	if(GOBBLE('=')) { tmp="`>="; break; }
	tmp="`>";
	break;

      case '!':
	if(GOBBLE('=')) { tmp="`!="; break; }
	tmp="`!";
	break;

      case '=':
	if(GOBBLE('=')) { tmp="`=="; break; }
	tmp="`=";
	break;

      case '(':
	if(GOBBLE(')')) 
	{
	  tmp="`()";
	  break;
	}
	yyerror("Illegal ` identifier.");
	tmp="";
	break;
	
      case '-':
	if(GOBBLE('>'))
	{
	  tmp="`->";
	  if(GOBBLE('=')) tmp="`->=";
	}else{
	  tmp="`-";
	}
	break;

      case '[':
	if(GOBBLE(']'))
	{
	  tmp="`[]";
	  if(GOBBLE('=')) tmp="`[]=";
	  break;
	}

      default:
	yyerror("Illegal ` identifier.");
	lex.pos--;
	tmp="`";
	break;
      }

      yylval->string=make_shared_string(tmp);
      return F_IDENTIFIER;
    }

  
    default:
      if(isidchar(c))
      {
	struct pike_string *s;
	lex.pos--;
	READBUF(isidchar(C));

	yylval->number=lex.current_line;

	if(len>1 && len<10)
	{
	  switch(TWO_CHAR(buf[0],buf[1]))
	  {
	  case TWO_CHAR('a','r'):
	    if(ISWORD("array")) return F_ARRAY_ID;
	  break;
	  case TWO_CHAR('b','r'):
	    if(ISWORD("break")) return F_BREAK;
	  break;
	  case TWO_CHAR('c','a'):
	    if(ISWORD("case")) return F_CASE;
	    if(ISWORD("catch")) return F_CATCH;
	  break;
	  case TWO_CHAR('c','l'):
	    if(ISWORD("class")) return F_CLASS;
	  break;
	  case TWO_CHAR('c','o'):
	    if(ISWORD("constant")) return F_CONSTANT;
	    if(ISWORD("continue")) return F_CONTINUE;
	  break;
	  case TWO_CHAR('d','e'):
	    if(ISWORD("default")) return F_DEFAULT;
	  break;
	  case TWO_CHAR('d','o'):
	    if(ISWORD("do")) return F_DO;
	  break;
	  case TWO_CHAR('e','l'):
	    if(ISWORD("else")) return F_ELSE;
	  break;
	  case TWO_CHAR('f','l'):
	    if(ISWORD("float")) return F_FLOAT_ID;
	  break;
	  case TWO_CHAR('f','o'):
	    if(ISWORD("for")) return F_FOR;
	    if(ISWORD("foreach")) return F_FOREACH;
	  break;
	  case TWO_CHAR('f','u'):
	    if(ISWORD("function")) return F_FUNCTION_ID;
	  break;
	  case TWO_CHAR('g','a'):
	    if(ISWORD("gauge")) return F_GAUGE;
	  break;
	  case TWO_CHAR('i','f'):
	    if(ISWORD("if")) return F_IF;
	  break;
	  case TWO_CHAR('i','m'):
	    if(ISWORD("import")) return F_IMPORT;
	  break;
	  case TWO_CHAR('i','n'):
	    if(ISWORD("int")) return F_INT_ID;
	    if(ISWORD("inherit")) return F_INHERIT;
	    if(ISWORD("inline")) return F_INLINE;
	  break;
	  case TWO_CHAR('l','a'):
	    if(ISWORD("lambda")) return F_LAMBDA;
	  break;
	  case TWO_CHAR('m','a'):
	    if(ISWORD("mapping")) return F_MAPPING_ID;
	  break;
	  case TWO_CHAR('m','i'):
	    if(ISWORD("mixed")) return F_MIXED_ID;
	  break;
	  case TWO_CHAR('m','u'):
	    if(ISWORD("multiset")) return F_MULTISET_ID;
	  break;
	  case TWO_CHAR('n','o'):
	    if(ISWORD("nomask")) return F_NO_MASK;
	  break;
	  case TWO_CHAR('o','b'):
	    if(ISWORD("object")) return F_OBJECT_ID;
	  break;
	  case TWO_CHAR('p','r'):
	    if(ISWORD("predef")) return F_PREDEF;
	    if(ISWORD("program")) return F_PROGRAM_ID;
	    if(ISWORD("private")) return F_PRIVATE;
	    if(ISWORD("protected")) return F_PROTECTED;
	    break;
	  break;
	  case TWO_CHAR('p','u'):
	    if(ISWORD("public")) return F_PUBLIC;
	  break;
	  case TWO_CHAR('r','e'):
	    if(ISWORD("return")) return F_RETURN;
	  break;
	  case TWO_CHAR('s','s'):
	    if(ISWORD("sscanf")) return F_SSCANF;
	  break;
	  case TWO_CHAR('s','t'):
	    if(ISWORD("static")) return F_STATIC;
	    if(ISWORD("string")) return F_STRING_ID;
	  break;
	  case TWO_CHAR('s','w'):
	    if(ISWORD("switch")) return F_SWITCH;
	  break;
	  case TWO_CHAR('t','y'):
	    if(ISWORD("typeof")) return F_TYPEOF;
	  break;
	  case TWO_CHAR('v','a'):
	    if(ISWORD("varargs")) return F_VARARGS;
	  break;
	  case TWO_CHAR('v','o'):
	    if(ISWORD("void")) return F_VOID_ID;
	  break;
	  case TWO_CHAR('w','h'):
	    if(ISWORD("while")) return F_WHILE;
	  break;
	  }
	}
	yylval->string=make_shared_binary_string(buf,len);
	return F_IDENTIFIER;
      }else{
	char buff[100];
	sprintf(buff, "Illegal character (hex %02x) '%c'", c, c);
	yyerror(buff);
	return ' ';
      }
    }
    }
  }
}

