/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
#include "global.h"
RCSID("$Id: lex.c,v 1.21 1997/04/16 03:09:12 hubbe Exp $");
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
#define EXPANDMAX 500000

struct pike_string *current_file;

INT32 current_line;
INT32 old_line;
INT32 total_lines;
INT32 nexpands;
int pragma_all_inline;          /* inline all possible inlines */

struct pike_predef_s
{
  char *name;
  char *value;
  struct pike_predef_s *next;
};

struct pike_predef_s *pike_predefs=0;

static int calc();
static void calc1();

void exit_lex()
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


  while(local_variables)
  {
    int e;
    struct locals *l;
    for(e=0;e<local_variables->current_number_of_locals;e++)
    {
      free_string(local_variables->variable[e].name);
      free_string(local_variables->variable[e].type);
    }
    if(local_variables->current_type)
      free_string(local_variables->current_type);
    if(local_variables->current_return_type)
      free_string(local_variables->current_return_type);
    l=local_variables->next;
    free((char *)local_variables);
    local_variables=l;
  }

  if(current_file) free_string(current_file);
  free_reswords();
}

struct keyword reserved_words[] =
{
{ "array",	F_ARRAY_ID, },
{ "break",	F_BREAK, },
{ "case",	F_CASE, },
{ "catch",	F_CATCH, },
{ "class",	F_CLASS, },
{ "constant",	F_CONSTANT, },
{ "continue",	F_CONTINUE, },
{ "default",	F_DEFAULT, },
{ "do",		F_DO, },
{ "else",	F_ELSE, },
{ "float",	F_FLOAT_ID, },
{ "for",	F_FOR, },
{ "foreach",	F_FOREACH, },
{ "function",	F_FUNCTION_ID, },
{ "gauge",	F_GAUGE, },
{ "if",		F_IF, },
{ "import",	F_IMPORT, },
{ "inherit",	F_INHERIT, },
{ "inline",	F_INLINE, },
{ "int",	F_INT_ID, },
{ "lambda",	F_LAMBDA, },
{ "mapping",	F_MAPPING_ID, },
{ "mixed",	F_MIXED_ID, },
{ "multiset",	F_MULTISET_ID, },
{ "nomask",	F_NO_MASK, },
{ "object",	F_OBJECT_ID, },
{ "predef",	F_PREDEF, },
{ "private",	F_PRIVATE, },
{ "program",	F_PROGRAM_ID, },
{ "protected",	F_PROTECTED, },
{ "public",	F_PUBLIC, },
{ "return",	F_RETURN, },
{ "sscanf",	F_SSCANF, },
{ "static",	F_STATIC, },
{ "string",	F_STRING_ID, },
{ "switch",	F_SWITCH, },
{ "typeof",	F_TYPEOF, },
{ "varargs",	F_VARARGS, },
{ "void",	F_VOID_ID, },
{ "while",	F_WHILE, },
};

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
};

struct instr instrs[F_MAX_INSTR - F_OFFSET];

struct reserved
{
  struct hash_entry link;
  int token;
};

struct hash_table *reswords;

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

  reswords=create_hash_table();

  for(i=0; i<NELEM(reserved_words); i++)
  {
    struct reserved *r=ALLOC_STRUCT(reserved);
    r->token = reserved_words[i].token;
    r->link.s = make_shared_string(reserved_words[i].word);
    reswords=hash_insert(reswords, &r->link);
  }

  /* Enlarge this hashtable heruetically */
  reswords = hash_rehash(reswords, 2<<my_log2(NELEM(reserved_words)));
}

void free_reswords()
{
  free_hashtable(reswords,0);
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

/* foo must be a shared string */
static int lookup_resword(struct pike_string *s)
{
  struct hash_entry *h;
  h=hash_lookup(reswords, s);
  if(!h) return -1;
  return BASEOF(h, reserved, link)->token;
}



/*** input routines ***/
struct inputstate
{
  struct inputstate *next;
  int fd;
  unsigned char *data;
  INT32 buflen;
  INT32 pos;
  int dont_free_data;

  int (*my_getc)();
  int (*gobble)(int);
  int (*look)();
  void (*my_ungetc)(int);
  void (*ungetstr)(char *,INT32);
};

#define MY_EOF 4711

struct inputstate *istate=NULL;

static void link_inputstate(struct inputstate *i)
{
  i->next=istate;
  istate=i;
}

static void free_inputstate(struct inputstate *i)
{
  if(!i) return;
  if(i->fd>=0)
  {
  retry:
    if(close(i->fd)< 0)
      if(errno == EINTR)
	goto retry;
  }
  if(i->data && !i->dont_free_data) free(i->data);
  free_inputstate(i->next);
  free((char *)i);
}

static struct inputstate *new_inputstate();
static struct inputstate *memory_inputstate(INT32 size);

static int default_gobble(int c)
{
  if(istate->look()==c)
  {
    istate->my_getc();
    return 1;
  }else{
    return 0;
  }
}

static void default_ungetstr(char *s,INT32 len)
{
  link_inputstate(memory_inputstate(len+1000));
  istate->ungetstr(s,len);
}

static void default_ungetc(int s)
{
  char c=s;
  istate->ungetstr(&c,1);
}

static int default_look()
{
  int c;
  c=istate->my_getc();
  istate->my_ungetc(c);
  return c;
}

static struct inputstate *new_inputstate()
{
  struct inputstate *i;
  i=(struct inputstate *)xalloc(sizeof(struct inputstate));
  i->fd=-1;
  i->data=NULL;
  i->next=NULL;
  i->dont_free_data=0;
  i->gobble=default_gobble;
  i->ungetstr=default_ungetstr;
  i->my_ungetc=default_ungetc;
  i->look=default_look;
  return i;
}

/*** end of file input ***/
static int end_getc() { return MY_EOF; }
static int end_gobble(int c) { return c==MY_EOF; }
static void end_ungetc(int c)
{
  if(c==MY_EOF) return;
  default_ungetc(c);
}

static struct inputstate *end_inputstate()
{
  struct inputstate *i;
  i=new_inputstate();
  i->gobble=end_gobble;
  i->look=end_getc;
  i->my_getc=end_getc;
  i->my_ungetc=end_ungetc;
  return i;
}

/*** MEMORY input ***/
static void memory_ungetstr(char *s,INT32 len)
{
  INT32 tmp;
  tmp=MINIMUM(len,istate->pos);
  if(tmp)
  {
    istate->pos -= tmp;
    MEMCPY(istate->data + istate->pos , s+len-tmp , tmp);
    len-=tmp;
  }
  if(len) default_ungetstr(s,len);
}

static void memory_ungetc(int s)
{
  if(istate->pos)
  {
    istate->pos --;
    istate->data[istate->pos]=s;
  }else{
    default_ungetc(s);
  }
}

static int memory_getc()
{
  if(istate->pos<istate->buflen)
  {
#if LEXDEBUG>9
    fprintf(stderr,"lex: reading from memory '%c' (%d).\n",istate->data[istate->pos],istate->data[istate->pos]);
#endif
    return istate->data[(istate->pos)++];
  }else{
    struct inputstate *i;
    i=istate;
    istate=i->next;
    if(!i->dont_free_data) free(i->data);
    free((char *)i);
    return istate->my_getc();
  }
}

static int memory_look()
{
  if(istate->pos<istate->buflen)
  {
    return istate->data[istate->pos];
  }else{
    struct inputstate *i;
    i=istate;
    istate=i->next;
    if(!i->dont_free_data) free(i->data);
    free((char *)i);
    return istate->look();
  }
}

/* allocate an empty memory state */
static struct inputstate *memory_inputstate(INT32 size)
{
  struct inputstate *i;
  if(!size) size=10000;
  i=new_inputstate();
  i->data=(unsigned char *)xalloc(size);
  i->buflen=size;
  i->pos=size;
  i->ungetstr=memory_ungetstr;
  i->my_getc=memory_getc;
  i->look=memory_look;
  i->my_ungetc=memory_ungetc;
  return i;
}

static void prot_memory_ungetstr(char *s,INT32 len)
{
  INT32 tmp;
  tmp=MINIMUM(len,istate->pos);
  if(tmp)
  {
    if(!MEMCMP(istate->data + istate->pos - tmp, s+len-tmp , tmp))
    {
      istate->pos-=tmp;
      len-=tmp;
    }
  }
  if(len) default_ungetstr(s,len);
}

static void prot_memory_ungetc(int s)
{
  if(istate->pos && istate->data[istate->pos-1] == s)
  {
    istate->pos--;
  }else{
    default_ungetc(s);
  }
}

/* allocate a memory, read-only, inputstate */
static struct inputstate *prot_memory_inputstate(char *data,INT32 len)
{
  struct inputstate *i;
  i=new_inputstate();
  i->data=(unsigned char *)data;
  i->buflen=len;
  i->dont_free_data=1;
  i->pos=0;
  i->my_getc=memory_getc;
  i->look=memory_look;
  i->ungetstr=prot_memory_ungetstr;
  i->my_ungetc=prot_memory_ungetc;
  return i;
}

/*** FILE input ***/

#define READAHEAD 8192
static int file_getc()
{
  unsigned char buf[READAHEAD];
  int got;
  do {
    got=read(istate->fd, buf, READAHEAD);
    if(got > 0)
    {
      default_ungetstr((char *)buf, got);
      return istate->my_getc();
    }
    else if(got==0 || errno != EINTR)
    {
      struct inputstate *i;
      if(got<0 && errno != EINTR)
	my_yyerror("Lex: Read failed with error %d\n",errno);

      i=istate->next;
      close(istate->fd);
      free((char *)istate);
      istate=i;
      return istate->my_getc();
    }
  }while(1);
}

static struct inputstate *file_inputstate(int fd)
{
  struct inputstate *i;
  i=new_inputstate();
  i->fd=fd;
  i->my_getc=file_getc;
  return i;
}

static int GETC()
{
  int c;
  c=istate->my_getc();
  if(c=='\n') current_line++;
  return c;
}

static void UNGETC(int c)
{
  if(c=='\n') current_line--;
  istate->my_ungetc(c);
}

static void UNGETSTR(char *s,INT32 len)
{
  INT32 e;
  for(e=0;e<len;e++) if(s[e]=='\n') current_line--;
  istate->ungetstr(s,len);
}

static int GOBBLE(char c)
{
  if(istate->gobble(c))
  {
    if(c=='\n') current_line++;
    return 1;
  }else{
    return 0;
  }
}

#define LOOK() (istate->look())
#define SKIPWHITE() { int c; while(ISSPACE(c=GETC())); UNGETC(c); }
#define SKIPTO(X) { int c; while((c=GETC())!=(X) && (c!=MY_EOF)); }
#define SKIPUPTO(X) { int c; while((c=GETC())!=(X) && (c!=MY_EOF)); UNGETC(c); }
#define READBUF(X) { \
  register unsigned INT32 p; \
  register int C; \
  for(p=0;(C=GETC())!=MY_EOF && p<sizeof(buf) && (X);p++) \
  buf[p]=C; \
  if(p==sizeof(buf)) { \
    yyerror("Internal buffer overflow.\n"); p--; \
  } \
  UNGETC(C); \
  buf[p]=0; \
}

static char buf[1024];

/*** Define handling ***/

struct define
{
  struct hash_entry link; /* must be first */
  void (*magic)();
  int args;
  struct array *parts;
};

struct hash_table *defines = 0;

#define find_define(N) (defines?BASEOF(hash_lookup(defines, N), define, link):0)

/* argument must be shared string */
static void undefine(struct pike_string *name)
{
  struct define *d;

  d=find_define(name);

  if(!d) return;

  defines=hash_unlink(defines, & d->link);
  free_string(d->link.s);
  free_array(d->parts);
  free((char *)d);
}

/* name and as are supposed to be SHARED strings */
static void add_define(struct pike_string *name,
		       int args,
		       int parts_on_stack,
		       void (*magic)())
{
  struct define *d;

  f_aggregate(parts_on_stack);
  if(sp[-1].type != T_ARRAY)
  {
    yyerror("Define failed for unknown reason.\n");
    free_string(name);
    pop_stack();
    return;
  }

#if 0
  if(find_define(name))
  {
    my_yyerror("Redefining '%s'",name->str);
    free_string(name);
    pop_stack();
    return;
  }
#else
  undefine(name);
#endif

  d=(struct define *)xalloc(sizeof(struct define));
  d->link.s=name;
  d->args=args;
  d->magic=magic;
  d->parts=sp[-1].u.array;
  sp--;

  defines=hash_insert(defines, & d->link);
}

static void simple_add_define(char *name,char *as,void (*magic)())
{
  if(magic)
  {
    add_define(make_shared_string(name),-1,0,magic);
  }else{
    push_string(make_shared_string(as));
    add_define(make_shared_string(name),-1,1,magic);
  }
}

void free_one_define(struct hash_entry *h)
{
  struct define *d;
  d=BASEOF(h, define, link);
  if(d->parts) free_array(d->parts);
  free((void *)d);
}

static void free_all_defines()
{
  if(defines)
    free_hashtable(defines, free_one_define);
  defines=0;
}

static void do_define()
{
  int c,e,t,argc;
  struct svalue *save_sp=sp;
  struct svalue *args_sp;
  struct pike_string *s, *s2;

  SKIPWHITE();
  READBUF(isidchar(C));

  s=make_shared_string(buf);

  if(GOBBLE('('))
  {
    argc=0;

    SKIPWHITE();
    READBUF(isidchar(C));
    if(buf[0])
    {
      push_string(make_shared_string(buf));
      argc++;
      SKIPWHITE();
      while(GOBBLE(','))
      {
        SKIPWHITE();
        READBUF(isidchar(C));
        push_string(make_shared_string(buf));
        argc++;
      }
    }
    SKIPWHITE();

    if(!GOBBLE(')'))
      yyerror("Missing ')'");
  }else{
    argc=-1;
  }

  args_sp=sp;

  init_buf();
  t=0;
  sp->type=T_STRING;
  sp->u.string=make_shared_string("");
  sp++;

  while(1)
  {
    int tmp;

    c=GETC();
    if(c=='\\') if(GOBBLE('\n')) continue;
    if( (t!=!!isidchar(c) && argc>0) || c=='\n' || c==MY_EOF)
    {
      s2=free_buf();
      tmp=0;
      for(e=0;e<argc;e++)
      {
	if(save_sp[e].u.string==s2)
	{
	  free_string(s2);
	  push_int(e);
	  tmp=1;
	  break;
	}
      }
      if(!tmp)
      {
	push_string(s2);
	if(sp[-2].type==T_STRING) f_add(2);
      }
      if(c=='\n' || c==MY_EOF)
      {
	push_string(make_shared_string(" "));
	if(sp[-2].type==T_STRING) f_add(2);
	break;
      }
      t=!!isidchar(c);
      init_buf();
    }
    my_putchar(c);
  }
  UNGETC(c);
  add_define(s,argc,sp-args_sp,0);
  while(sp>save_sp) pop_stack();
}

/* s is a shared string */
static int expand_define(struct pike_string *s, int save_newline)
{
  struct svalue *save_sp=sp;
  struct define *d;
  int len,e,tmp,args;
  d=find_define(s);
  if(!d) return 0;

  if(nexpands>EXPANDMAX)
  {
    yyerror("Macro limit exceeded.");
    return 0;
  }

  if(d->magic)
  {
    d->magic();
    return 1;
  }

  if(d->args >= 0)
  {
    if(!save_newline)
    {
      SKIPWHITE();
    }else{
      do { e=GETC(); }while(ISSPACE(e) && e!='\n');
      UNGETC(e);
    }

    if(GOBBLE('('))
    {
      int parlvl,quote;
      int c;
      args=0;

      SKIPWHITE();
      init_buf();
      parlvl=1;
      quote=0;
      while(parlvl)
      {
	switch(c=GETC())
	{
	case MY_EOF:
	  yyerror("Unexpected end of file.");
	  while(sp>save_sp) pop_stack();
	  return 0;
	case '"': if(!(quote&2)) quote^=1; break;
	case '\'': if(!(quote&1)) quote^=2; break;
	case '(': if(!quote) parlvl++; break;
	case ')': if(!quote) parlvl--; break;
	case '\\': my_putchar(c); c=GETC(); break;
	case ',':
	  if(!quote && parlvl==1)
	  {
	    push_string(free_buf());
	    init_buf();
	    args++;
	    continue;
	  }
	}
	if(parlvl) my_putchar(c);
      }
      push_string(free_buf());
      if(args==0 && !d->args && !sp[-1].u.string->len)
      {
	pop_stack();
      }else{
	args++;
      }
    }else{
      args=0;
    }
  } else {
    args=-1;
  }
  
  if(args>d->args)
  {
    my_yyerror("Too many arguments to macro '%s'.\n",s->str);
    while(sp>save_sp) pop_stack();
    return 0;
  }

  if(args<d->args)
  {
    my_yyerror("Too few arguments to macro '%s'.\n",s->str);
    while(sp>save_sp) pop_stack();
    return 0;
  }
  len=0;
  for(e=d->parts->size-1;e>=0;e--)
  {
    switch(ITEM(d->parts)[e].type)
    {
    case T_STRING:
      tmp=ITEM(d->parts)[e].u.string->len;
      UNGETSTR(ITEM(d->parts)[e].u.string->str,tmp);
      break;

    case T_INT:
      tmp=save_sp[ITEM(d->parts)[e].u.integer].u.string->len;
      UNGETSTR(save_sp[ITEM(d->parts)[e].u.integer].u.string->str,tmp);
      break;

    default: tmp=0;
    }
    len+=tmp;
  }
  while(sp>save_sp) pop_stack();
  nexpands+=len;
  return 1;
}

/*** Handle include ****/

static void handle_include(char *name, int local_include)
{
  int fd;
  char buf[400];
  struct pike_string *s;

  s=make_shared_string(name);
  push_string(s);
  reference_shared_string(s);
  push_string(current_file);
  reference_shared_string(current_file);
  push_int(local_include);
  
  SAFE_APPLY_MASTER("handle_include",3);
  
  if(sp[-1].type != T_STRING)
  {
    my_yyerror("Couldn't include file '%s'.",s->str);
    return;
  }
  free_string(s);
  
 retry:
  fd=open(sp[-1].u.string->str,O_RDONLY);
  if(fd < 0)
  {
    if(errno == EINTR) goto retry;

#ifdef HAVE_STRERROR    
    my_yyerror("Couldn't open file to include '%s'. (%s)",sp[-1].u.string->str,strerror(errno));
#else
    my_yyerror("Couldn't open file to include '%s'. (ERRNO=%d)",sp[-1].u.string->str,errno);
#endif
    return;
  }

  UNGETSTR("\" 2",3);
  UNGETSTR(current_file->str,current_file->len);
  sprintf(buf,"\n# %ld \"",(long)current_line+1);
  UNGETSTR(buf,strlen(buf));

  total_lines+=current_line-old_line;
  old_line=current_line=1;
  free_string(current_file);
  current_file=sp[-1].u.string;
  sp--;
  link_inputstate(file_inputstate(fd));
  UNGETSTR("\n",1);
}

/*** Lexical analyzing ***/

static int char_const()
{
  int c;
  switch(c=GETC())
  {
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
  }
  return c;
}

#define SKIPTO_ENDIF 1
#define SKIPTO_ELSE 2

static void do_skip(int to)
{
  int lvl;
#if LEXDEBUG>3
  fprintf(stderr,"Skipping from %ld to ",(long)current_line);
#endif
  lvl=1;
  while(lvl)
  {
    switch(GETC())
    {
    case '/':
      if(GOBBLE('*'))
      {
	do{
	  SKIPTO('*');
	  if(LOOK()==MY_EOF)
	  {
	    yyerror("Unexpected end of file while skipping comment.");
	    return;
	  }
	}while(!GOBBLE('/'));
      }
      continue;

    case MY_EOF:
      yyerror("Unexpected end of file while skipping.");
      return;

    case '\\':
      GETC();
      continue;
	
    case '\n':
      if(GOBBLE('#'))
      {
	SKIPWHITE();
	READBUF(C!=' ' && C!='\t' && C!='\n');
    
	switch(buf[0])
	{
	case 'l':
	  if(strcmp("line",buf)) break;
	  READBUF(C!=' ' && C!='\t' && C!='\n');

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  SKIPWHITE();
	  READBUF(C!='\n');
	  if(buf[0]=='"' &&
	     buf[strlen(buf)-1]=='2' &&
	     ISSPACE(buf[strlen(buf)-2]))
	  {
	    if(lvl)
	    {
	      yyerror("Unterminated '#if' conditional.");
	      num_parse_error+=1000;
	    }
	  }
	  continue;

	case 'i':
	  if(!strcmp("include",buf)) continue;
	  if(!strcmp("if",buf) || !strcmp("ifdef",buf) || !strcmp("ifndef",buf))
	  {
	    lvl++;
	    continue;
	  }
	  break;

	case 'e':
	  if(!strcmp("endif",buf))
	  { 
	    lvl--;
	    if(lvl<0)
	    {
	      yyerror("Unbalanced '#endif'\n");
	      lvl=0;
	    }
	    continue;
	  }
	  if(!strcmp("else",buf))
	  {
	    if(lvl==1 && to==SKIPTO_ELSE) lvl=0;
	    continue;
	  }
	  if(!strcmp("elif",buf) || !strcmp("elseif",buf))
	  {
	    if(lvl==1 && to==SKIPTO_ELSE && calc()) lvl--;
	    continue;
	  }
	  if(!strcmp("error",buf)) continue;
	  break;

	case 'd':
	  if(!strcmp("define",buf)) continue;
	  break;

	case 'u':
	  if(!strcmp("undef",buf)) continue;
	  break;

	case 'p':
	  if(!strcmp("pragma",buf)) continue;
	  break;
	}
    
	my_yyerror("Unknown directive #%s.",buf);
	SKIPUPTO('\n');
	continue;
      }
    }
  }
#if LEXDEBUG>3
  fprintf(stderr,"%ld in %s.\n",(long)current_line,current_file->str);
#endif
}

static int do_lex(int literal, YYSTYPE *yylval)
#if LEXDEBUG>4
{
  int t;
  int do_lex2(int literal, YYSTYPE *yylval);
  t=do_lex2(literal, yylval);
  if(t<256)
  {
    fprintf(stderr,"do_lex(%d) -> '%c' (%d)\n",literal,t,t);
  }else{
    fprintf(stderr,"do_lex(%d) -> %s (%d)\n",literal,get_f_name(t),t);
  }
  return t;
}

static int do_lex2(int literal, YYSTYPE *yylval)
#endif
{
  int c;
#ifdef MALLOC_DEBUG
  check_sfltable();
#endif
  while(1)
  {
    switch(c=GETC())
    {
    case '\n':
      if(literal)
      {
	UNGETC('\n');
	return '\n';
      }
      if(GOBBLE('#'))
      {
	if(GOBBLE('!'))
	{
	  SKIPUPTO('\n');
	  continue;
	}
            
	SKIPWHITE();
	READBUF(C!=' ' && C!='\t' && C!='\n');

	switch(buf[0])
	{
	  char *p;

	case 'l':
	  if(strcmp("line",buf)) goto badhash;
	  READBUF(C!=' ' && C!='\t' && C!='\n');

	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	  total_lines+=current_line-old_line;
	  old_line=current_line=atoi(buf)-1;
	  SKIPWHITE();
	  READBUF(C!='\n');

	  p=buf;
	  if(*p=='"' && STRCHR(p+1,'"'))
	  {
	    char *p2;
	    p++;
	    if((p2=STRCHR(p+1,'"')))
	    {
	      *p2=0;
	      free_string(current_file);
	      current_file=make_shared_string(p);
	    }
	  }
	  break;

	case 'i':
	  if(!strcmp("include",buf))
	  {
	    SKIPWHITE();
	    c=do_lex(1, yylval);
	    if(c=='<')
	    {
	      READBUF(C!='>' && C!='\n');
	      if(!GOBBLE('>'))
	      {
		yyerror("Missing `>`");
		SKIPUPTO('\n');
		continue;
	      }
	    }else{
	      if(c!=F_STRING)
	      {
		yyerror("Couldn't find include filename.\n");
		SKIPUPTO('\n');
		continue;
	      }
	    }
	    handle_include(buf, c==F_STRING);
	    break;
	  }

	  if(!strcmp("if",buf))
	  {
	    if(!calc()) do_skip(SKIPTO_ELSE);
	    break;
	  }

	  if(!strcmp("ifdef",buf))
	  {
	    struct pike_string *s;
	    SKIPWHITE();
	    READBUF(isidchar(C));
	    s=findstring(buf);
	    if(!s || !find_define(s)) do_skip(SKIPTO_ELSE);
	    break;
	  }

	  if(!strcmp("ifndef",buf))
	  {
	    struct pike_string *s;
	    SKIPWHITE();
	    READBUF(isidchar(C));
	    s=findstring(buf);
	    if(s && find_define(s)) do_skip(SKIPTO_ELSE);
	    break;
	  }
	  goto badhash;

	case 'e':
	  if(!strcmp("endif",buf)) break;
	  if(!strcmp("else",buf))
	  {
	    SKIPUPTO('\n');
	    do_skip(SKIPTO_ENDIF);
	    break;
	  }
	  if(!strcmp("elif",buf) || !strcmp("elseif",buf))
	  {
	    SKIPUPTO('\n');
	    do_skip(SKIPTO_ENDIF);
	    break;
	  }
	  if(!strcmp("error",buf))
	  {
	    SKIPWHITE();
	    READBUF(C!='\n');
	    yyerror(buf);
	    break;
	  }
	  goto badhash;

	case 'u':
	  if(!strcmp("undef",buf))
	  {
	    struct pike_string *s;
	    SKIPWHITE();
	    READBUF(isidchar(C));
	    if((s=findstring(buf))) undefine(s);
	    break;
	  }
	  goto badhash;

	case 'd':
	  if(!strcmp("define",buf))
	  {
	    do_define();
	    break;
	  }
	  goto badhash;

	case 'p':
	  if(!strcmp("pragma",buf))
	  {
	    SKIPWHITE();
	    READBUF(C!='\n');
	    if (strcmp(buf, "all_inline") == 0)
	    {
	      pragma_all_inline = 1;
	    }
	    break;
	  }

	badhash:
	  my_yyerror("Unknown directive #%s.",buf);
	  SKIPUPTO('\n');
	  continue;
	  
	}
      }
      continue;

    case ' ':
    case '\t':
      continue;

    case MY_EOF:
      return 0;
  
    case '\'':
      c=GETC();
      if(c=='\\') c=char_const();
      if(GETC()!='\'')
	yyerror("Unterminated character constant.");
      yylval->number=c;
      return F_NUMBER;
	
    case '"':
      init_buf();
      while(1)
      {
	c=GETC();

	switch(c)
	{
	case MY_EOF:
	  yyerror("End of file in string.");
	  free(simple_free_buf());
	  return 0;

	case '\n':
	  yyerror("Newline in string.");
	  free(simple_free_buf());
	  return 0;

	case '\\':
	  my_putchar(char_const());
	  continue;
	  
	case '"':
	  break;

	default:
	  my_putchar(c);
	  continue;
	}
	break;
      }
      if(literal)
      {
	strncpy(buf,return_buf(),sizeof(buf));
	buf[sizeof(buf)-1]=0;
	yylval->str=buf;
      }else{
	yylval->string=free_buf();
      }
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
      if(GOBBLE('x'))
      {
	READBUF(isxdigit(C));
	yylval->number=STRTOL(buf,NULL,16);
	return F_NUMBER;
      }
  
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
      char *p;
      UNGETC(c);
      READBUF(isdigit(C) || C=='.');

      p=STRCHR(buf,'.');
      
      if(p)
      {
	if(p[1]=='.')
	{
	  UNGETSTR(p,strlen(p));
	  *p=0;
	  p=NULL;
	}else if((p=STRCHR(p+1,'.')))
	{
	  UNGETSTR(p,strlen(p));
	  *p=0;
	}
	if((p=STRCHR(buf,'.')))
	{
	  yylval->fnum=STRTOD(buf,NULL);
	  return F_FLOAT;
	}
      }
      if(buf[0]=='0')
	yylval->number=STRTOL(buf,NULL,8);
      else
	yylval->number=STRTOL(buf,NULL,10);
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
      if(GOBBLE('*'))
      {
	do{
	  SKIPTO('*');
	  if(LOOK()==MY_EOF)
	  {
	    yyerror("Unexpected end of file while skipping comment.");
	    return 0;
	  }
	} while(!GOBBLE('/'));
	continue;
      }else if(GOBBLE('/'))
      {
	SKIPUPTO('\n');
	continue;
      }
       /* Fallthrough */
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
	tmp="";
	break;

      }

      if(literal)
      {
	yylval->str=buf;
      }else{
	yylval->string=make_shared_string(tmp);
      }
      return F_IDENTIFIER;
    }

  
    default:
      if(isidchar(c))
      {
	struct pike_string *s;
	UNGETC(c);
	READBUF(isidchar(C));

	if(!literal)
	{
	  /* identify identifier, if it is not a shared string,
	   * then it is neither a define, reserved word, local variable
	   * or efun, at least not yet.
	   */

	  s=findstring(buf);
	  if(s)
	  {
	    int i;
	    if(expand_define(s,0)) continue;

	    i=lookup_resword(s);
	    if(i >= 0)
	    {
	      yylval->number=current_line;
	      return i;
	    }

	    reference_shared_string(s);
	  }else{
	    s=make_shared_string(buf);
	  }
	  yylval->string=s;
	  return F_IDENTIFIER;
	}
  
	yylval->str=buf;
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

int yylex(YYSTYPE *yylval)
{
#ifdef __CHECKER__
  MEMSET((char *)yylval,0,sizeof(YYSTYPE));
#endif
  return do_lex(0, yylval);
}

static YYSTYPE my_yylval;
static int lookahead;

static void low_lex()
{
  while(1)
  {
    struct pike_string *s;

    lookahead=do_lex(1, &my_yylval);
    if(lookahead == F_IDENTIFIER)
    {
      if(!strcmp("defined",my_yylval.str))
      {
	SKIPWHITE();
	if(!GOBBLE('('))
	{
	  yyerror("Missing '(' in defined.\n");
	  return;
	}
	READBUF(isidchar(C));
	if(!GOBBLE(')'))
	{
	  yyerror("Missing ')' in defined.\n");
	  return;
	}
	s=findstring(buf);

	if(s && find_define(s))
	  UNGETSTR(" 1 ",3);
	else
	  UNGETSTR(" 0 ",3);

	continue;
      }

      if(!strcmp("efun",my_yylval.str) || !strcmp("constant",my_yylval.str))
      {
	SKIPWHITE();
	if(!GOBBLE('('))
	{
	  yyerror("Missing '(' in #if constant().\n");
	  return;
	}
	READBUF(isidchar(C));
	if(!GOBBLE(')'))
	{
	  yyerror("Missing ')' in #if constant().\n");
	  return;
	}
	s=findstring(buf);

	if(s && low_mapping_string_lookup(get_builtin_constants(), s))
	  UNGETSTR(" 1 ",3);
	else
	  UNGETSTR(" 0 ",3);

	continue;
      }

      s=findstring(my_yylval.str);
      if(!s || !expand_define(s,1))
	UNGETSTR(" 0 ",3);
      continue;
    }

    break;
  }
}

static void calcC()
{
  switch(lookahead)
  {
    case '(':
      low_lex();
      calc1();
      if(lookahead != ')')
	error("Missing ')'\n");
      break;

    case F_FLOAT:
      push_float(my_yylval.fnum);
      break;

    case F_STRING:
      push_string(make_shared_string(my_yylval.str));
      break;

    case F_NUMBER:
      push_int(my_yylval.number);
      break;

    default:
      yyerror("Syntax error in #if.");
      return;
  }

  low_lex();

  while(lookahead=='[')
  {
    low_lex();
    calc1();
    f_index(2);
    if(lookahead!=']')
      error("Missing ']'\n");
    else
      low_lex();
  }
}

static void calcB()
{
  switch(lookahead)
  {
    case '-': low_lex(); calcB(); o_negate(); break;
    case F_NOT: low_lex(); calcB(); o_not(); break;
    case '~': low_lex(); calcB(); o_compl(); break;
    default: calcC();
  }
}

static void calcA()
{
  calcB();
  while(1)
  {
    switch(lookahead)
    {
      case '/': low_lex(); calcB(); o_divide(); continue;
      case '*': low_lex(); calcB(); o_multiply(); continue;
      case '%': low_lex(); calcB(); o_mod(); continue;
    }
    break;
  }
}

static void calc9()
{
  calcA();

  while(1)
  {
    switch(lookahead)
    {
      case '+': low_lex(); calcA(); f_add(2); continue;
      case '-': low_lex(); calcA(); o_subtract(); continue;
    }
    break;
  }
}

static void calc8()
{
  calc9();

  while(1)
  {
    switch(lookahead)
    {
      case F_LSH: low_lex(); calc9(); o_lsh(); continue;
      case F_RSH: low_lex(); calc9(); o_rsh(); continue;
    }
    break;
  }
}

static void calc7b()
{
  calc8();

  while(1)
  {
    switch(lookahead)
    {
      case '<': low_lex(); calc8(); f_lt(2); continue;
      case '>': low_lex(); calc8(); f_gt(2); continue;
      case F_GE: low_lex(); calc8(); f_ge(2); continue;
      case F_LE: low_lex(); calc8(); f_le(2); continue;
    }
    break;
  }
}

static void calc7()
{
  calc7b();

  while(1)
  {
    switch(lookahead)
    {
      case F_EQ: low_lex(); calc7b(); f_eq(2); continue;
      case F_NE: low_lex(); calc7b(); f_ne(2); continue;
    }
    break;
  }
}

static void calc6()
{
  calc7();

  while(lookahead=='&')
  {
    low_lex();
    calc7();
    o_and();
  }
}

static void calc5()
{
  calc6();

  while(lookahead=='^')
  {
    low_lex();
    calc6();
    o_xor();
  }
}

static void calc4()
{
  calc5();

  while(lookahead=='|')
  {
    low_lex();
    calc5();
    o_or();
  }
}

static void calc3()
{
  calc4();

  while(lookahead==F_LAND)
  {
    low_lex();
    check_destructed(sp-1);
    if(IS_ZERO(sp-1))
    {
      calc4();
      pop_stack();
    }else{
      pop_stack();
      calc4();
    }
  }
}

static void calc2()
{
  calc3();

  while(lookahead==F_LOR)
  {
    low_lex();
    check_destructed(sp-1);
    if(!IS_ZERO(sp-1))
    {
      calc3();
      pop_stack();
    }else{
      pop_stack();
      calc3();
    }
  }
}

static void calc1()
{
  calc2();

  if(lookahead=='?')
  {
    low_lex();
    calc1();
    if(lookahead!=':')
      error("Colon expected.\n");
    low_lex();
    calc1();

    check_destructed(sp-3);
    assign_svalue(sp-3,IS_ZERO(sp-3)?sp-1:sp-2);
    pop_n_elems(2);
  }

}

static int calc()
{
  JMP_BUF recovery;
  int ret;

  ret=0;
  if (SETJMP(recovery))
  {
    if(throw_value.type == T_ARRAY && throw_value.u.array->size)
    {
      union anything *a;
      a=low_array_get_item_ptr(throw_value.u.array, 0, T_STRING);
      if(a)
      {
	yyerror(a->string->str);
      }else{
	yyerror("Nonstandard error format.\n");
      }
    }else{
      yyerror("Nonstandard error format.\n");
    }
    ret=-1;
  }else{
    low_lex();
    calc1();
    if(lookahead!='\n')
    {
      SKIPUPTO('\n');
      yyerror("Extra characters after #if expression.");
    }else{
      UNGETC('\n');
    }
    check_destructed(sp-1);
    ret=!IS_ZERO(sp-1);
    pop_stack();
  }
  UNSETJMP(recovery);

  return ret;
}

/*** Magic defines ***/
void insert_current_line()
{
  char buf[20];
  sprintf(buf," %ld ",(long)current_line);
  UNGETSTR(buf,strlen(buf));
}

void insert_current_file_as_string()
{
  UNGETSTR("\"",1);
  UNGETSTR(current_file->str, current_file->len);
  UNGETSTR("\"",1);
}

void insert_current_time_as_string()
{
  time_t tmp;
  char *buf;
  time(&tmp);
  buf=ctime(&tmp);

  UNGETSTR("\"",1);
  UNGETSTR(buf+11, 8);
  UNGETSTR("\"",1);
}

void insert_current_date_as_string()
{
  time_t tmp;
  char *buf;
  time(&tmp);
  buf=ctime(&tmp);

  UNGETSTR("\"",1);
  UNGETSTR(buf+19, 5);
  UNGETSTR(buf+4, 6);
  UNGETSTR("\"",1);
}

/*** ***/

static void start_new()
{
  struct pike_predef_s *tmpf;

  free_all_defines();

  simple_add_define("__PIKE__", "1",0);
  
  for (tmpf=pike_predefs; tmpf; tmpf=tmpf->next)
    simple_add_define(tmpf->name, tmpf->value,0);

  simple_add_define("__LINE__",0,insert_current_line);
  simple_add_define("__FILE__",0,insert_current_file_as_string);
  simple_add_define("__DATE__",0,insert_current_date_as_string);
  simple_add_define("__TIME__",0,insert_current_time_as_string);

  free_inputstate(istate);
  istate=NULL;
  link_inputstate(end_inputstate());
  old_line=current_line = 1;
  pragma_all_inline=0;
  nexpands=0;
  if(current_file) free_string(current_file);
  current_file=0;
}

void start_new_file(int fd,struct pike_string *filename)
{
  start_new();
  copy_shared_string(current_file,filename);
  
  link_inputstate(file_inputstate(fd));
  UNGETSTR("\n",1);
}

void start_new_string(char *s,INT32 len,struct pike_string *name)
{
  start_new();
  copy_shared_string(current_file,name);
  link_inputstate(prot_memory_inputstate(s,len));
  UNGETSTR("\n",1);
}

void end_new_file()
{
  if(current_file)
  {
    free_string(current_file);
    current_file=0;
  }

  free_inputstate(istate);
  istate=NULL;
  free_all_defines();
  total_lines+=current_line-old_line;
}


void add_predefine(char *s)
{
  char buffer1[100],buffer2[10000];
  struct pike_predef_s *tmp;

  if(sscanf(s,"%[^=]=%[ -~=]",buffer1,buffer2) ==2)
  {
    s=buffer1;
  }else{
    buffer2[0]='1';
    buffer2[1]=0;
  }
  tmp=ALLOC_STRUCT(pike_predef_s);
  tmp->name=(char *)xalloc(strlen(s)+1);
  strcpy(tmp->name,s);
  tmp->value=(char *)xalloc(strlen(buffer2)+1);
  strcpy(tmp->value,buffer2);
  tmp->next=pike_predefs;
  pike_predefs=tmp;
}
