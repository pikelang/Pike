#include "global.h"
#include "types.h"
#include "language.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "program.h"
#include "las.h"
#include "docode.h"
#include "main.h"
#include "error.h"
#include "lex.h"

struct p_instr_s
{
  short opcode;
  short line;
  struct pike_string *file;
  INT32 arg;
};

typedef struct p_instr_s p_instr;

dynamic_buffer instrbuf;

static int hasarg(int opcode)
{
  switch(opcode)
  {
  case F_NUMBER:
  case F_NEG_NUMBER:
  case F_CALL_LFUN:
  case F_CALL_LFUN_AND_POP:
  case F_SSCANF:
  case F_POP_N_ELEMS:

  case F_SIZEOF_LOCAL:

  case F_ASSIGN_GLOBAL:
  case F_ASSIGN_GLOBAL_AND_POP:
  case F_ASSIGN_LOCAL:
  case F_ASSIGN_LOCAL_AND_POP:
  case F_GLOBAL_LVALUE:
  case F_LOCAL_LVALUE:
  case F_CLEAR_LOCAL:
  case F_LOCAL:
  case F_GLOBAL:

  case F_INC_LOCAL:
  case F_DEC_LOCAL:
  case F_POST_INC_LOCAL:
  case F_POST_DEC_LOCAL:
  case F_INC_LOCAL_AND_POP:
  case F_DEC_LOCAL_AND_POP:

  case F_LFUN:
  case F_STRING:
  case F_ARROW:
  case F_ARROW_STRING:
  case F_STRING_INDEX:
  case F_LOCAL_INDEX:
  case F_POS_INT_INDEX:
  case F_NEG_INT_INDEX:
  case F_CONSTANT:
  case F_SWITCH:
  case F_APPLY:
  case F_CATCH:

  case F_BRANCH:
  case F_BRANCH_WHEN_ZERO:
  case F_BRANCH_WHEN_NON_ZERO:

  case F_BRANCH_WHEN_EQ:
  case F_BRANCH_WHEN_NE:
  case F_BRANCH_WHEN_LT:
  case F_BRANCH_WHEN_LE:
  case F_BRANCH_WHEN_GT:
  case F_BRANCH_WHEN_GE:

  case F_FOREACH:
  case F_INC_LOOP:
  case F_DEC_LOOP:
  case F_INC_NEQ_LOOP:
  case F_DEC_NEQ_LOOP:

  case F_LAND:
  case F_LOR:

  case F_ALIGN:
  case F_POINTER:
  case F_LABEL:
    return 1;
    
  default:
    return 0;
  }
}

void init_bytecode()
{
  low_reinit_buf(&instrbuf);
}

void exit_bytecode()
{
  INT32 e,length;
  p_instr *c;

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

  for(e=0;e<length;e++) free_string(c->file);
  
  toss_buffer(&instrbuf);
}

int insert_opcode(unsigned int f,
		  INT32 b,
		  INT32 current_line,
		  struct pike_string *current_file)
{
  p_instr *p;

#ifdef DEBUG
  if(!hasarg(f) && b)
    fatal("hasarg() is wrong!\n");
#endif

  p=(p_instr *)low_make_buf_space(sizeof(p_instr), &instrbuf);


#ifdef DEBUG
  if(!instrbuf.s.len)
    fatal("Low make buf space failed!!!!!!\n");
#endif

  p->opcode=f;
  p->line=current_line;
  copy_shared_string(p->file, current_file);
  p->arg=b;

  return p - (p_instr *)instrbuf.s.str;
}

int insert_opcode2(int f,int current_line, struct pike_string *current_file)
{
#ifdef DEBUG
  if(hasarg(f))
    fatal("hasarg() is wrong!\n");
#endif
  return insert_opcode(f,0,current_line, current_file);
}

void update_arg(int instr,INT32 arg)
{
  p_instr *p;
#ifdef DEBUG
  if(instr > (long)instrbuf.s.len / (long)sizeof(p_instr) || instr < 0)
    fatal("update_arg outside known space.\n");
#endif  
  p=(p_instr *)instrbuf.s.str;
  p[instr].arg=arg;
}


/**** Bytecode Generator *****/

void ins_f_byte(unsigned int b)
{
  if(store_linenumbers && b<F_MAX_OPCODE)
    ADD_COMPILED(b);

  b-=F_OFFSET;
  if(b>255)
  {
    switch(b >> 8)
    {
    case 1: ins_f_byte(F_ADD_256); break;
    case 2: ins_f_byte(F_ADD_512); break;
    case 3: ins_f_byte(F_ADD_768); break;
    case 4: ins_f_byte(F_ADD_1024); break;
    default:
      ins_f_byte(F_ADD_256X);
      ins_byte(b/256,A_PROGRAM);
    }
    b&=255;
  }
  ins_byte((unsigned char)b,A_PROGRAM);
}

static void ins_f_byte_with_arg(unsigned int a,unsigned INT32 b)
{
  switch(b >> 8)
  {
  case 0 : break;
  case 1 : ins_f_byte(F_PREFIX_256); break;
  case 2 : ins_f_byte(F_PREFIX_512); break;
  case 3 : ins_f_byte(F_PREFIX_768); break;
  case 4 : ins_f_byte(F_PREFIX_1024); break;
  default:
    if( b < 256*256)
    {
      ins_f_byte(F_PREFIX_CHARX256);
      ins_byte(b>>8, A_PROGRAM);
    }else if(b < 256*256*256) {
      ins_f_byte(F_PREFIX_WORDX256);
      ins_byte(b >> 16, A_PROGRAM);
      ins_byte(b >> 8, A_PROGRAM);
    }else{
      ins_f_byte(F_PREFIX_24BITX256);
      ins_byte(b >> 24, A_PROGRAM);
      ins_byte(b >> 16, A_PROGRAM);
      ins_byte(b >> 8, A_PROGRAM);
    }
  }
  ins_f_byte(a);
  ins_byte(b, A_PROGRAM);
}

void assemble()
{
  INT32 e,d,length,max_label,tmp;
  INT32 *labels, *jumps, *point;
  p_instr *c;

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

  max_label=0;
  for(e=0;e<length;e++,c++)
    if(c->opcode == F_LABEL)
      if(c->arg > max_label)
	max_label = c->arg;


  labels=(INT32 *)xalloc(sizeof(INT32) * (max_label+1));
  jumps=(INT32 *)xalloc(sizeof(INT32) * (max_label+1));
  point=(INT32 *)xalloc(sizeof(INT32) * (max_label+1));

  for(e=0;e<=max_label;e++) point[e]=labels[e]=jumps[e]=-1;

  c=(p_instr *)instrbuf.s.str;
  for(e=0;e<length;e++)
  {
#ifdef DEBUG
    if(a_flag > 2 && store_linenumbers)
    {
      if(hasarg(c->opcode))
	fprintf(stderr,"===%3d %4x %s(%d)\n",c->line,PC,get_token_name(c->opcode),c->arg);
      else
	fprintf(stderr,"===%3d %4x %s\n",c->line,PC,get_token_name(c->opcode));
    }
#endif

    if(store_linenumbers)
      store_linenumber(c->line, c->file);

    switch(c->opcode)
    {
    case F_ALIGN:
      while(PC % c->arg) ins_byte(0, A_PROGRAM);
      break;

    case F_LABEL:
#ifdef DEBUG
      if(c->arg > max_label || c->arg < 0)
	fatal("max_label calculation failed!\n");

      if(labels[c->arg] != -1)
	fatal("Duplicate label!\n");
#endif
      labels[c->arg]=PC;

      for(d=1;e+d<length;d++)
      {
	switch(c[d].opcode)
	{
	case F_LABEL: continue;
	case F_BRANCH: point[c->arg]=c[d].arg;
	}
	break;
      }
      break;

    case F_BRANCH_WHEN_EQ:
    case F_BRANCH_WHEN_NE:
    case F_BRANCH_WHEN_LT:
    case F_BRANCH_WHEN_LE:
    case F_BRANCH_WHEN_GT:
    case F_BRANCH_WHEN_GE:
    case F_BRANCH_WHEN_ZERO:
    case F_BRANCH_WHEN_NON_ZERO:
    case F_BRANCH:
    case F_INC_LOOP:
    case F_DEC_LOOP:
    case F_INC_NEQ_LOOP:
    case F_DEC_NEQ_LOOP:
    case F_LAND:
    case F_LOR:
    case F_CATCH:
    case F_FOREACH:
      ins_f_byte(c->opcode);

    case F_POINTER:
#ifdef DEBUG
      if(c->arg > max_label || c->arg < 0) fatal("Jump to unknown label?\n");
#endif
      tmp=PC;
      ins_int(jumps[c->arg],A_PROGRAM);
      jumps[c->arg]=tmp;
      break;

    case F_APPLY:
      ins_f_byte(c->arg + F_MAX_OPCODE);
      break;

    default:
      if(hasarg(c->opcode))
	ins_f_byte_with_arg(c->opcode, c->arg);
      else
	ins_f_byte(c->opcode);
      break;
    }
    
    c++;
  }

  for(e=0;e<=max_label;e++)
  {
    int tmp2;
    tmp2=e;
    while(point[tmp2]!=-1) tmp2=point[tmp2];
    tmp2=labels[tmp2];

    while(jumps[e]!=-1)
    {
#ifdef DEBUG
      if(labels[e]==-1)
	fatal("Hyperspace error: unknown jump point.\n");
#endif
      tmp=read_int(jumps[e]);
      upd_int(jumps[e], tmp2 - jumps[e]);
      jumps[e]=tmp;
    }
  }

  free((char *)labels);
  free((char *)jumps);
  free((char *)point);

  exit_bytecode();
}

/**** Peephole optimizer ****/

static int fifo_len, eye,len;
static p_instr *instructions;

#ifdef DEBUG
static void debug()
{
  p_instr *p;

  if(fifo_len > (long)instrbuf.s.len / (long)sizeof(p_instr))
    fatal("Fifo too long.\n");

  if(eye < 0)
    fatal("Popped beyond start of code.\n");

  if(instrbuf.s.len)
  {
    p=(p_instr *)low_make_buf_space(0, &instrbuf);
    if(!p[-1].file)
      fatal("No file name on last instruction!\n");
  }
}
#else
#define debug()
#endif


static p_instr *instr(int offset)
{
  p_instr *p;

  debug();

  if(offset >= 0)
  {
    if(offset < fifo_len)
    {
      p=(p_instr *)low_make_buf_space(0, &instrbuf);
      p-=fifo_len;
      p+=offset;
      return p;
    }else{
      offset-=fifo_len;
      offset+=eye;
      if(offset >= len) return 0;
      return instructions+offset;
    }
  }else{
    fatal("Can't handle negative offsets in peephole optimizer!\n");
    return 0; /* Make GCC happy */
  }
}

static int opcode(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->opcode;
  return -1;
}

static int argument(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->arg;
  return -1;
}

static void advance()
{
  if(fifo_len)
  {
    fifo_len--;
  }else{
    p_instr *p;
    if(p=instr(0))
      insert_opcode(p->opcode, p->arg, p->line, p->file);
    eye++;
  }
  debug();
}

static void pop_n_opcodes(int n)
{
  while(n>0)
  {
    if(fifo_len)
    {
      p_instr *p;

#ifdef DEBUG
      if(instrbuf.s.len <= 0)
	fatal("Popping out of opcodes.\n");
#endif
      low_make_buf_space(-((INT32)sizeof(p_instr)), &instrbuf);
      p=(p_instr *)low_make_buf_space(0, &instrbuf);
      
      free_string(p->file);
      fifo_len--;
    }else{
      eye++;
    }
    n--;
  }
}

#define insert(X,Y) insert_opcode((X),(Y),current_line, current_file),dofix()
#define insert2(X) insert_opcode2((X),current_line, current_file),dofix()

static void dofix()
{
  p_instr *p,tmp;
  int e;

  if(fifo_len)
  {
    p=(p_instr *)low_make_buf_space(0, &instrbuf);
    tmp=p[-1];
    for(e=0;e<fifo_len;e++)
      p[-1-e]=p[-2-e];
    p[-1-e]=tmp;
  }
}


void asm_opt()
{
#ifdef DEBUG
  if(a_flag > 3)
  {
    p_instr *c;
    INT32 e,length;
    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);

    fprintf(stderr,"Optimization begins: \n");
    for(e=0;e<length;e++,c++)
    {
      if(hasarg(c->opcode))
	fprintf(stderr,"---%3d: %s(%d)\n",c->line,get_token_name(c->opcode),c->arg);
      else
	fprintf(stderr,"---%3d: %s\n",c->line,get_token_name(c->opcode));
    }
  }
#endif

#include "peep_engine.c"


#ifdef DEBUG
  if(a_flag > 4)
  {
    p_instr *c;
    INT32 e,length;
    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);

    fprintf(stderr,"Optimization begins: \n");
    for(e=0;e<length;e++,c++)
    {
      if(hasarg(c->opcode))
	fprintf(stderr,">>>%3d: %s(%d)\n",c->line,get_token_name(c->opcode),c->arg);
      else
	fprintf(stderr,">>>%3d: %s\n",c->line,get_token_name(c->opcode));
    }
  }
#endif
}

