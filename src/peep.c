#include "global.h"
#include "language.h"
#include "stralloc.h"
#include "dynamic_buffer.h"
#include "program.h"
#include "las.h"
#include "docode.h"
#include "main.h"
#include "pike_error.h"
#include "lex.h"
#include "pike_memory.h"
#include "peep.h"
#include "dmalloc.h"
#include "stuff.h"
#include "bignum.h"
#include "opcodes.h"

RCSID("$Id: peep.c,v 1.48 2003/10/10 01:17:44 mast Exp $");

static void asm_opt(void);

dynamic_buffer instrbuf;

static int hasarg(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_HASARG;
}

static int hasarg2(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_HASARG2;
}

#ifdef PIKE_DEBUG
static void dump_instr(p_instr *p)
{
  if(!p) return;
  fprintf(stderr,"%s",get_token_name(p->opcode));
  if(hasarg(p->opcode))
  {
    fprintf(stderr,"(%d",p->arg);
    if(hasarg2(p->opcode))
      fprintf(stderr,",%d",p->arg2);
    fprintf(stderr,")");
  }
}
#endif



void init_bytecode(void)
{
  low_reinit_buf(&instrbuf);
}

void exit_bytecode(void)
{
  ptrdiff_t e, length;
  p_instr *c;

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

  for(e=0;e<length;e++) free_string(c->file);
  
  toss_buffer(&instrbuf);
}

ptrdiff_t insert_opcode2(unsigned int f,
			 INT32 b,
			 INT32 c,
			 INT32 current_line,
			 struct pike_string *current_file)
{
  p_instr *p;

#ifdef PIKE_DEBUG
  if(!hasarg2(f) && c)
    fatal("hasarg2(%d) is wrong!\n",f);
#endif

  p=(p_instr *)low_make_buf_space(sizeof(p_instr), &instrbuf);


#ifdef PIKE_DEBUG
  if(!instrbuf.s.len)
    fatal("Low make buf space failed!!!!!!\n");
#endif

  p->opcode=f;
  p->line=current_line;
  copy_shared_string(p->file, current_file);
  p->arg=b;
  p->arg2=c;

  return p - (p_instr *)instrbuf.s.str;
}

ptrdiff_t insert_opcode1(unsigned int f,
			 INT32 b,
			 INT32 current_line,
			 struct pike_string *current_file)
{
#ifdef PIKE_DEBUG
  if(!hasarg(f) && b)
    fatal("hasarg(%d) is wrong!\n",f);
#endif

  return insert_opcode2(f,b,0,current_line,current_file);
}

ptrdiff_t insert_opcode0(int f,int current_line, struct pike_string *current_file)
{
#ifdef PIKE_DEBUG
  if(hasarg(f))
    fatal("hasarg(%d) is wrong!\n",f);
#endif
  return insert_opcode1(f,0,current_line, current_file);
}


void update_arg(int instr,INT32 arg)
{
  p_instr *p;
#ifdef PIKE_DEBUG
  if(instr > (long)instrbuf.s.len / (long)sizeof(p_instr) || instr < 0)
    fatal("update_arg outside known space.\n");
#endif  
  p=(p_instr *)instrbuf.s.str;
  p[instr].arg=arg;
}


/**** Bytecode Generator *****/

void ins_f_byte(unsigned int b)
{
#ifdef PIKE_DEBUG
  if(store_linenumbers && b<F_MAX_OPCODE)
    ADD_COMPILED(b);
#endif /* PIKE_DEBUG */

  b-=F_OFFSET;
#ifdef PIKE_DEBUG
  if(b>255)
    Pike_error("Instruction too big %d\n",b);
#endif
  add_to_program((unsigned char)b);
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
      add_to_program((unsigned char)(b>>8));
    }else if(b < 256*256*256) {
      ins_f_byte(F_PREFIX_WORDX256);
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }else{
      ins_f_byte(F_PREFIX_24BITX256);
      add_to_program((unsigned char)(b>>24));
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }
  }
  ins_f_byte(a);
  add_to_program((unsigned char)b);
}

static void ins_f_byte_with_2_args(unsigned int a,
				   unsigned INT32 c,
				   unsigned INT32 b)
{

  switch(b >> 8)
  {
  case 0 : break;
  case 1 : ins_f_byte(F_PREFIX2_256); break;
  case 2 : ins_f_byte(F_PREFIX2_512); break;
  case 3 : ins_f_byte(F_PREFIX2_768); break;
  case 4 : ins_f_byte(F_PREFIX2_1024); break;
  default:
    if( b < 256*256)
    {
      ins_f_byte(F_PREFIX2_CHARX256);
      add_to_program((unsigned char)(b>>8));
    }else if(b < 256*256*256) {
      ins_f_byte(F_PREFIX2_WORDX256);
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }else{
      ins_f_byte(F_PREFIX2_24BITX256);
      add_to_program((unsigned char)(b>>24));
      add_to_program((unsigned char)(b>>16));
      add_to_program((unsigned char)(b>>8));
    }
  }
  ins_f_byte_with_arg(a,c);
  add_to_program((unsigned char)b);
}

void assemble(void)
{
  INT32 d,max_label,tmp;
  INT32 *labels, *jumps, *uses;
  ptrdiff_t e, length;
  p_instr *c;
  int reoptimize=1;

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

  max_label=-1;
  for(e=0;e<length;e++,c++)
    if(c->opcode == F_LABEL)
      if(c->arg > max_label)
	max_label = c->arg;


  labels=(INT32 *)xalloc(sizeof(INT32) * (max_label+2));
  jumps=(INT32 *)xalloc(sizeof(INT32) * (max_label+2));
  uses=(INT32 *)xalloc(sizeof(INT32) * (max_label+2));

  while(reoptimize)
  {
    reoptimize=0;
    for(e=0;e<=max_label;e++)
    {
      labels[e]=jumps[e]=-1;
      uses[e]=0;
    }
    
    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);
    for(e=0;e<length;e++)
      if(c[e].opcode == F_LABEL && c[e].arg>=0)
	labels[c[e].arg]=DO_NOT_WARN((INT32)e);
    
    for(e=0;e<length;e++)
    {
      if(instrs[c[e].opcode-F_OFFSET].flags & I_POINTER)
      {
	while(1)
	{
	  int tmp,tmp2;
	  tmp=labels[c[e].arg];
	  
	  while(tmp<length &&
		(c[tmp].opcode == F_LABEL ||
		 c[tmp].opcode == F_NOP)) tmp++;
	  
	  if(tmp>=length) break;
	  
	  if(c[tmp].opcode==F_BRANCH)
	  {
	    c[e].arg=c[tmp].arg;
	    continue;
	  }
	  
#define TWOO(X,Y) (((X)<<8)+(Y))
	  
	  switch(TWOO(c[e].opcode,c[tmp].opcode))
	  {
	    case TWOO(F_LOR,F_BRANCH_WHEN_NON_ZERO):
	      c[e].opcode=F_BRANCH_WHEN_NON_ZERO;
	    case TWOO(F_LOR,F_LOR):
	      c[e].arg=c[tmp].arg;
	      continue;
	      
	    case TWOO(F_LAND,F_BRANCH_WHEN_ZERO):
	      c[e].opcode=F_BRANCH_WHEN_ZERO;
	    case TWOO(F_LAND,F_LAND):
	      c[e].arg=c[tmp].arg;
	      continue;
	      
	    case TWOO(F_LOR, F_RETURN):
	      c[e].opcode=F_RETURN_IF_TRUE;
	      goto pointer_opcode_done;
	      
	    case TWOO(F_BRANCH, F_RETURN):
	    case TWOO(F_BRANCH, F_RETURN_0):
	    case TWOO(F_BRANCH, F_RETURN_1):
	    case TWOO(F_BRANCH, F_RETURN_LOCAL):
	      if(c[e].file) free_string(c[e].file);
	      c[e]=c[tmp];
	      if(c[e].file) add_ref(c[e].file);
	      goto pointer_opcode_done;
	  }
	  break;
	}
#ifdef PIKE_DEBUG
	if (c[e].arg < 0 || c[e].arg > max_label)
	  fatal ("Invalid index into uses: %d\n", c[e].arg);
#endif
	uses[c[e].arg]++;
      }
    pointer_opcode_done:;
    }
    
    for(e=0;e<=max_label;e++)
    {
      if(!uses[e] && labels[e]>=0)
      {
	c[labels[e]].opcode=F_NOP;
	reoptimize++;
      }
    }
    if(!reoptimize) break;
    
    asm_opt();
    reoptimize=0;
  }

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

  for(e=0;e<=max_label;e++) labels[e]=jumps[e]=-1;
  
  c=(p_instr *)instrbuf.s.str;
  for(e=0;e<length;e++)
  {
#ifdef PIKE_DEBUG
    if((a_flag > 2 && store_linenumbers) || a_flag > 3)
    {
      fprintf(stderr, "===%3d %4lx ", c->line,
	      DO_NOT_WARN((unsigned long)PIKE_PC));
      dump_instr(c);
      fprintf(stderr,"\n");
    }
#endif

    if(store_linenumbers)
      store_linenumber(c->line, c->file);

    switch(c->opcode)
    {
    case F_NOP:
    case F_NOTREACHED:
    case F_START_FUNCTION:
      break;
    case F_ALIGN:
      while(PIKE_PC % c->arg) add_to_program(0);
      break;

    case F_BYTE:
      add_to_program((unsigned char)(c->arg));
      break;

    case F_DATA:
      add_relocated_int_to_program(c->arg);
      break;

    case F_LABEL:
      if(c->arg == -1) break;
#ifdef PIKE_DEBUG
      if(c->arg > max_label || c->arg < 0)
	fatal("max_label calculation failed!\n");

      if(labels[c->arg] != -1)
	fatal("Duplicate label!\n");
#endif
      labels[c->arg] = DO_NOT_WARN((INT32)PIKE_PC);
      break;

    default:
      switch(instrs[c->opcode - F_OFFSET].flags)
      {
      case I_ISJUMP:
	ins_f_byte(c->opcode);

      case I_ISPOINTER:
#ifdef PIKE_DEBUG
	if(c->arg > max_label || c->arg < 0) fatal("Jump to unknown label?\n");
#endif
	tmp = DO_NOT_WARN((INT32)PIKE_PC);
	add_relocated_int_to_program(jumps[c->arg]);
	jumps[c->arg]=tmp;
	break;

	case I_TWO_ARGS:
	  ins_f_byte_with_2_args(c->opcode, c->arg,c->arg2);
	  break;
	  
	case I_HASARG:
	  ins_f_byte_with_arg(c->opcode, c->arg);
	  break;

      case 0:
	ins_f_byte(c->opcode);
	break;

#ifdef PIKE_DEBUG
      default:
	fatal("Unknown instruction type.\n");
#endif
      }
    }
    
    c++;
  }

  for(e=0;e<=max_label;e++)
  {
    int tmp2=labels[e];

    while(jumps[e]!=-1)
    {
#ifdef PIKE_DEBUG
      if(labels[e]==-1)
	fatal("Hyperspace error: unknown jump point %ld at %d (pc=%x).\n",
	      PTRDIFF_T_TO_LONG(e), labels[e], jumps[e]);
#endif
      tmp=read_int(jumps[e]);
      upd_int(jumps[e], tmp2 - jumps[e]);
      jumps[e]=tmp;
    }
  }

  free((char *)labels);
  free((char *)jumps);
  free((char *)uses);


  exit_bytecode();
}

/**** Peephole optimizer ****/

int remove_clear_locals=0x7fffffff;
static int fifo_len;
static ptrdiff_t eye, len;
static p_instr *instructions;

static INLINE ptrdiff_t insopt2(int f, INT32 a, INT32 b,
				int cl, struct pike_string *cf)
{
  p_instr *p;

#ifdef PIKE_DEBUG
  if(!hasarg2(f) && b)
    fatal("hasarg(%d) is wrong!\n",f);
#endif

  p=(p_instr *)low_make_buf_space(sizeof(p_instr), &instrbuf);

  if(fifo_len)
  {
    MEMMOVE(p-fifo_len+1,p-fifo_len,fifo_len*sizeof(p_instr));
    p-=fifo_len;
  }

#ifdef PIKE_DEBUG
  if(!instrbuf.s.len)
    fatal("Low make buf space failed!!!!!!\n");
#endif

  p->opcode=f;
  p->line=cl;
  copy_shared_string(p->file, cf);
  p->arg=a;
  p->arg2=b;

  return p - (p_instr *)instrbuf.s.str;
}

static INLINE ptrdiff_t insopt1(int f, INT32 a, int cl, struct pike_string *cf)
{
#ifdef PIKE_DEBUG
  if(!hasarg(f) && a)
    fatal("hasarg(%d) is wrong!\n",f);
#endif

  return insopt2(f,a,0,cl, cf);
}

static INLINE ptrdiff_t insopt0(int f, int cl, struct pike_string *cf)
{
#ifdef PIKE_DEBUG
  if(hasarg(f))
    fatal("hasarg(%d) is wrong!\n",f);
#endif
  return insopt2(f,0,0,cl, cf);
}

static void debug(void)
{
  if(fifo_len > (long)instrbuf.s.len / (long)sizeof(p_instr))
    fifo_len=(long)instrbuf.s.len / (long)sizeof(p_instr);
#ifdef PIKE_DEBUG
  if(eye < 0)
    fatal("Popped beyond start of code.\n");

  if(instrbuf.s.len)
  {
    p_instr *p;
    p=(p_instr *)low_make_buf_space(0, &instrbuf);
    if(!p[-1].file)
      fatal("No file name on last instruction!\n");
  }
#endif
}


static INLINE p_instr *instr(int offset)
{
  p_instr *p;

  debug();

  if(offset < fifo_len)
  {
    p=(p_instr *)low_make_buf_space(0, &instrbuf);
    p-=fifo_len;
    p+=offset;
    if(((char *)p)<instrbuf.s.str)  return 0;
    return p;
  }else{
    offset-=fifo_len;
    offset+=eye;
    if(offset >= len) return 0;
    return instructions+offset;
  }
}

static INLINE int opcode(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->opcode;
  return -1;
}

static INLINE int argument(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->arg;
  return -1;
}

static INLINE int argument2(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->arg2;
  return -1;
}

static void advance(void)
{
  if(fifo_len)
  {
    fifo_len--;
  }else{
    p_instr *p;
    if((p=instr(0)))
      insert_opcode2(p->opcode, p->arg, p->arg2, p->line, p->file);
    eye++;
  }
  debug();
}

static void pop_n_opcodes(int n)
{
  int e,d;
  if(fifo_len)
  {
    p_instr *p;

    d=n;
    if(d>fifo_len) d=fifo_len;
#ifdef PIKE_DEBUG
    if((long)d > (long)instrbuf.s.len / (long)sizeof(p_instr))
      fatal("Popping out of instructions.\n");
#endif

    /* FIXME: It looks like the fifo could be optimized.
     *	/grubba 2000-11-21 (in Versailles)
     */

    p=(p_instr *)low_make_buf_space(0, &instrbuf);
    p-=fifo_len;
    for(e=0;e<d;e++) free_string(p[e].file);
    fifo_len-=d;
    if(fifo_len) MEMMOVE(p,p+d,fifo_len*sizeof(p_instr));
    n-=d;
    low_make_buf_space(-((INT32)sizeof(p_instr))*d, &instrbuf);
  }
  eye+=n;
}

#define DO_OPTIMIZATION_PREQUEL(topop) do {	\
    struct pike_string *cf; 			\
    INT32 cl=instr(0)->line;			\
  						\
    DO_IF_DEBUG(				\
      if(a_flag>5)				\
      {						\
  	int e;					\
  	fprintf(stderr,"PEEP at %d:",cl);	\
  	for(e=0;e<topop;e++)			\
  	{					\
  	  fprintf(stderr," ");			\
  	  dump_instr(instr(e));			\
  	}					\
  	fprintf(stderr," => ");			\
      }						\
    )						\
    						\
    copy_shared_string(cf,instr(0)->file);	\
    pop_n_opcodes(topop)

#define DO_OPTIMIZATION_POSTQUEL(q)	\
    fifo_len+=q;			\
    free_string(cf);			\
    debug();				\
  					\
    DO_IF_DEBUG(			\
      if(a_flag>5)			\
      {					\
  	int e;				\
  	for(e=0;e<q;e++)		\
  	{				\
  	  fprintf(stderr," ");		\
  	  dump_instr(instr(e));		\
  	}				\
  	fprintf(stderr,"\n");		\
      }					\
    )					\
  					\
    fifo_len += q + 3;			\
  }  while(0)


static void do_optimization(int topop, ...)
{
  va_list arglist;
  int q=-1;

  DO_OPTIMIZATION_PREQUEL(topop);

  va_start(arglist, topop);
  
  while(1)
  {
    q++;
    switch(va_arg(arglist, int))
    {
      case 0:
	break;
      case 1:
      {
	int i=va_arg(arglist, int);
	insopt0(i,cl,cf);
	continue;
      }
      case 2:
      {
	int i=va_arg(arglist, int);
	int j=va_arg(arglist, int);
	insopt1(i,j,cl,cf);
	continue;
      }
      case 3:
      {
	int i=va_arg(arglist, int);
	int j=va_arg(arglist, int);
	int k=va_arg(arglist, int);
	insopt2(i,j,k,cl,cf);
	continue;
      }
    }
    break;
  }

  va_end(arglist);

  DO_OPTIMIZATION_POSTQUEL(q);
}


static void asm_opt(void)
{
#ifdef PIKE_DEBUG
  if(a_flag > 3)
  {
    p_instr *c;
    ptrdiff_t e, length;

    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);

    fprintf(stderr,"Optimization begins: \n");
    for(e=0;e<length;e++,c++)
    {
      fprintf(stderr,"---%3d: ",c->line);
      dump_instr(c);
      fprintf(stderr,"\n");
    }
  }
#endif

#ifndef IN_TPIKE
#include "peep_engine.c"
#endif /* IN_TPIKE */

#ifdef PIKE_DEBUG
  if(a_flag > 4)
  {
    p_instr *c;
    ptrdiff_t e, length;

    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);

    fprintf(stderr,"Optimization begins: \n");
    for(e=0;e<length;e++,c++)
    {
      fprintf(stderr,">>>%3d: ",c->line);
      dump_instr(c);
      fprintf(stderr,"\n");
    }
  }
#endif
}

