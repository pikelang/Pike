/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: peep.c,v 1.101 2004/08/25 09:36:47 grubba Exp $
*/

#include "global.h"
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
#include "builtin_functions.h"
#include "constants.h"
#include "interpret.h"
#include "pikecode.h"

RCSID("$Id: peep.c,v 1.101 2004/08/25 09:36:47 grubba Exp $");

static void asm_opt(void);

dynamic_buffer instrbuf;

#ifdef PIKE_DEBUG
static int hasarg(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_HASARG;
}

static int hasarg2(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_HASARG2;
}

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
  initialize_buf(&instrbuf);
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
    Pike_fatal("hasarg2(%d) is wrong!\n",f);
#endif

  p=(p_instr *)low_make_buf_space(sizeof(p_instr), &instrbuf);


#ifdef PIKE_DEBUG
  if(!instrbuf.s.len)
    Pike_fatal("Low make buf space failed!!!!!!\n");
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
    Pike_fatal("hasarg(%d) is wrong!\n",f);
#endif

  return insert_opcode2(f,b,0,current_line,current_file);
}

ptrdiff_t insert_opcode0(int f,int current_line, struct pike_string *current_file)
{
#ifdef PIKE_DEBUG
  if(hasarg(f))
    Pike_fatal("hasarg(%d) is wrong!\n",f);
#endif
  return insert_opcode1(f,0,current_line, current_file);
}


void update_arg(int instr,INT32 arg)
{
  p_instr *p;
#ifdef PIKE_DEBUG
  if(instr > (long)instrbuf.s.len / (long)sizeof(p_instr) || instr < 0)
    Pike_fatal("update_arg outside known space.\n");
#endif  
  p=(p_instr *)instrbuf.s.str;
  p[instr].arg=arg;
}

#ifndef FLUSH_CODE_GENERATOR_STATE
#define FLUSH_CODE_GENERATOR_STATE()
#endif

/**** Bytecode Generator *****/

static int relabel;

INT32 assemble(int store_linenumbers)
{
  INT32 entry_point;
  INT32 max_label=-1,tmp;
  INT32 *labels, *jumps, *uses;
  ptrdiff_t e, length;
  p_instr *c;
#ifdef PIKE_PORTABLE_BYTECODE
  struct pike_string *tripples = NULL;
#endif /* PIKE_PORTABLE_BYTECODE */
#ifdef PIKE_DEBUG
  INT32 max_pointer=-1;
  int synch_depth = 0;
  size_t fun_start = Pike_compiler->new_program->num_program;
#endif

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

#ifdef PIKE_DEBUG
  if((a_flag > 1 && store_linenumbers) || a_flag > 2)
  {
    for (e = 0; e < length; e++) {
      if (c[e].opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr, "~~~%4d %4lx %*s", c[e].line,
	      DO_NOT_WARN((unsigned long)e), synch_depth, "");
      dump_instr(c+e);
      fprintf(stderr,"\n");
      if (c[e].opcode == F_SYNCH_MARK) synch_depth++;
    }
    if (synch_depth) {
      Pike_fatal("Unbalanced sync_mark/pop_sync_mark: %d\n", synch_depth);
    }
  }
#endif

#ifdef PIKE_PORTABLE_BYTECODE
  /* No need to do this for constant evaluations. */
  if (store_linenumbers) {
    p_wchar2 *current_tripple;
    struct pike_string *previous_file = NULL;
    int previous_line = 0;
    ptrdiff_t num_linedirectives = 0;

    /* Count the number of F_FILENAME/F_LINE pseudo-ops we need to add. */
    for (e=0; e < length; e++) {
      if (c[e].file != previous_file) {
	previous_file = c[e].file;
	num_linedirectives++;
      }
      if (c[e].line != previous_line) {
	previous_line = c[e].line;
	num_linedirectives++;
      }
    }

    /* fprintf(stderr, "length:%d directives:%d\n",
     *         length, num_linedirectives);
     */
      
    if (!(tripples = begin_wide_shared_string(3*(length+num_linedirectives),
					      2))) {
      Pike_fatal("Failed to allocate wide string of length %d 3*(%d + %d).\n",
		 3*(length+num_linedirectives), length, num_linedirectives);
    }
    previous_file = NULL;
    previous_line = 0;
    current_tripple = STR2(tripples);
    for (e = 0; e < length; e++) {
      if (c[e].file != previous_file) {
	current_tripple[0] = F_FILENAME;
	current_tripple[1] = store_prog_string(c[e].file);
	current_tripple[2] = 0;
	current_tripple += 3;
	previous_file = c[e].file;
      }
      if (c[e].line != previous_line) {
	current_tripple[0] = F_LINE;
	current_tripple[1] = c[e].line;
	current_tripple[2] = 0;
	current_tripple += 3;
	previous_line = c[e].line;
      }
      current_tripple[0] = c[e].opcode;
      current_tripple[1] = c[e].arg;
      current_tripple[2] = c[e].arg2;
      current_tripple += 3;
    }
#ifdef PIKE_DEBUG
    if (current_tripple != STR2(tripples) + 3*(length + num_linedirectives)) {
      Pike_fatal("Tripple length mismatch %d != %d 3*(%d + %d)\n",
		 current_tripple - STR2(tripples),
		 3*(length + num_linedirectives),
		 length, num_linedirectives);
    }
#endif /* PIKE_DEBUG */
    tripples = end_shared_string(tripples);
  }
  
#endif /* PIKE_PORTABLE_BYTECODE */

  for(e=0;e<length;e++,c++) {
    if(c->opcode == F_LABEL) {
      if(c->arg > max_label)
	max_label = c->arg;
    }
#ifdef PIKE_DEBUG
    else if (instrs[c->opcode - F_OFFSET].flags & I_POINTER) {
      if (c->arg > max_pointer)
	max_pointer = c->arg;
    }
#endif /* PIKE_DEBUG */
  }

#ifdef PIKE_DEBUG
  if (max_pointer > max_label) {
    fprintf(stderr,
	    "Reference to undefined label %d > %d\n"
	    "Bad instructions are marked with '***':\n",
	    max_pointer, max_label);
    c=(p_instr *)instrbuf.s.str;
    for(e=0;e<length;e++,c++) {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr, " * %4d %4lx ",
	      c->line, DO_NOT_WARN((unsigned long)e));
      dump_instr(c);
      if ((instrs[c->opcode - F_OFFSET].flags & I_POINTER) &&
	  (c->arg > max_label)) {
	fprintf(stderr, " ***\n");
      } else {
	fprintf(stderr, "\n");
      }
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
    }
    
    Pike_fatal("Reference to undefined label %d > %d\n",
	       max_pointer, max_label);
  }
#endif /* PIKE_DEBUG */

  labels=(INT32 *)xalloc(sizeof(INT32) * 3 * (max_label+2));
  jumps = labels + max_label + 2;
  uses = jumps + max_label + 2;

  int reoptimize = relabel = !(debug_options & NO_PEEP_OPTIMIZING);

  while(relabel)
  {
    /* First do the relabel pass. */
    for(e=0;e<=max_label;e++)
    {
      labels[e]=jumps[e]=-1;
      uses[e]=0;
    }
    
    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);
    for(e=0;e<length;e++)
      if(c[e].opcode == F_LABEL && c[e].arg>=0) {
	labels[c[e].arg]=DO_NOT_WARN((INT32)e);
      }
    
    for(e=0;e<length;e++)
    {
      if(instrs[c[e].opcode-F_OFFSET].flags & I_POINTER)
      {
	while(1)
	{
	  int tmp;
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
	  Pike_fatal ("Invalid index into uses: %d\n", c[e].arg);
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
	reoptimize = 1;
      }
    }

    relabel = 0;

    if(!reoptimize) break;

    /* Then do the optimize pass. */

    asm_opt();

    reoptimize = 0;

    if (!relabel) break;

#if 1
#ifdef PIKE_DEBUG
    if (a_flag > 3)
      fprintf(stderr, "Rerunning optimizer.\n");
#endif
#else /* !1 */
    relabel = 0;
#endif /* 1 */
  }

  /* Time to create the actual bytecode. */

  c=(p_instr *)instrbuf.s.str;
  length=instrbuf.s.len / sizeof(p_instr);

  for(e=0;e<=max_label;e++) labels[e]=jumps[e]=-1;
  
#ifdef ALIGN_PIKE_FUNCTION_BEGINNINGS
  while( ( (((INT32) PIKE_PC)+2) & (ALIGN_PIKE_JUMPS-1)))
    ins_byte(0);
#endif

#ifdef PIKE_PORTABLE_BYTECODE
  if (store_linenumbers) {
    ins_data(store_prog_string(tripples));
    free_string(tripples);
  } else {
    ins_data(NULL);
  }
#endif /* PIKE_PORTABLE_BYTECODE */

  entry_point = PIKE_PC;

#ifdef PIKE_DEBUG
  synch_depth = 0;
#endif
  FLUSH_CODE_GENERATOR_STATE();
  for(e=0;e<length;e++)
  {
#ifdef PIKE_DEBUG
    if (c != (((p_instr *)instrbuf.s.str)+e)) {
      Pike_fatal("Instruction loop deviates. 0x%04x != 0x%04x\n",
		 e, DO_NOT_WARN((INT32)(c - ((p_instr *)instrbuf.s.str))));
    }
    if((a_flag > 2 && store_linenumbers) || a_flag > 3)
    {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr, "===%4d %4lx %*s", c->line,
	      DO_NOT_WARN((unsigned long)PIKE_PC), synch_depth, "");
      dump_instr(c);
      fprintf(stderr,"\n");
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
    }
#endif

    if(store_linenumbers) {
      store_linenumber(c->line, c->file);
#ifdef PIKE_DEBUG
      if (c->opcode < F_MAX_OPCODE)
	ADD_COMPILED(c->opcode);
#endif /* PIKE_DEBUG */
    }

    switch(c->opcode)
    {
    case F_NOP:
    case F_NOTREACHED:
    case F_START_FUNCTION:
      break;
    case F_ALIGN:
      ins_align(c->arg);
      break;

    case F_BYTE:
      ins_byte((unsigned char)(c->arg));
      break;

    case F_DATA:
      ins_data(c->arg);
      break;

    case F_ENTRY:
#ifdef INS_ENTRY
      INS_ENTRY();
#endif /* INS_ENTRY */
      break;

    case F_LABEL:
      if(c->arg == -1) break;
#ifdef PIKE_DEBUG
      if(c->arg > max_label || c->arg < 0)
	Pike_fatal("max_label calculation failed!\n");

      if(labels[c->arg] != -1)
	Pike_fatal("Duplicate label!\n");
#endif
      FLUSH_CODE_GENERATOR_STATE();
      labels[c->arg] = DO_NOT_WARN((INT32)PIKE_PC);
      if ((e+1 < length) &&
	  (c[1].opcode != F_LABEL) &&
	  (c[1].opcode != F_BYTE) &&
	  (c[1].opcode != F_DATA)) {
	/* Don't add redundant code before labels or raw data. */
	UPDATE_PC();
      }

      break;

    case F_VOLATILE_RETURN:
      ins_f_byte(F_RETURN);
      break;

    case F_POINTER:
#ifdef PIKE_DEBUG
      if(c->arg > max_label || c->arg < 0)
	Pike_fatal("Jump to unknown label?\n");
#endif
      tmp = DO_NOT_WARN((INT32)PIKE_PC);
      ins_pointer(jumps[c->arg]);
      jumps[c->arg]=tmp;
      break;

    default:
      switch(instrs[c->opcode - F_OFFSET].flags & I_IS_MASK)
      {
      case I_ISPTRJUMP:
#ifdef INS_F_JUMP
	tmp=INS_F_JUMP(c->opcode, (labels[c->arg] != -1));
	if(tmp != -1)
	{
	  UPDATE_F_JUMP(tmp, jumps[c->arg]);
	  jumps[c->arg]=~tmp;
	  break;
	}
#endif

	ins_f_byte(c->opcode);

#ifdef PIKE_DEBUG
	if(c->arg > max_label || c->arg < 0)
	  Pike_fatal("Jump to unknown label?\n");
#endif
	tmp = DO_NOT_WARN((INT32)PIKE_PC);
	ins_pointer(jumps[c->arg]);
	jumps[c->arg]=tmp;
	break;

      case I_ISPTRJUMPARGS:
#ifdef INS_F_JUMP_WITH_TWO_ARGS
	tmp = INS_F_JUMP_WITH_TWO_ARGS(c->opcode, c->arg, c->arg2,
				       (labels[c[1].arg] != -1));
	if(tmp != -1)
	{
	  /* Step ahead to the pointer instruction, and inline it. */
#ifdef PIKE_DEBUG
	  if (c[1].opcode != F_POINTER) {
	    Pike_fatal("Expected opcode %s to be followed by a pointer\n",
		       instrs[c->opcode - F_OFFSET].name);
	  }
#endif /* PIKE_DEBUG */
	  c++;
	  e++;
	  UPDATE_F_JUMP(tmp, jumps[c->arg]);
	  jumps[c->arg]=~tmp;
	  break;
	}
#endif /* INS_F_JUMP_WITH_TWO_ARGS */

	/* FALL_THROUGH
	 *
	 * Note that the pointer in this case will be handled by the
	 * next turn through the loop.
	 */

      case I_TWO_ARGS:
      case I_ISJUMPARGS:
	ins_f_byte_with_2_args(c->opcode, c->arg, c->arg2);
	break;

      case I_ISPTRJUMPARG:
#ifdef INS_F_JUMP_WITH_ARG
	tmp = INS_F_JUMP_WITH_ARG(c->opcode, c->arg, (labels[c[1].arg] != -1));
	if(tmp != -1)
	{
	  /* Step ahead to the pointer instruction, and inline it. */
#ifdef PIKE_DEBUG
	  if (c[1].opcode != F_POINTER) {
	    Pike_fatal("Expected opcode %s to be followed by a pointer\n",
		       instrs[c->opcode - F_OFFSET].name);
	  }
#endif /* PIKE_DEBUG */
	  c++;
	  e++;
	  UPDATE_F_JUMP(tmp, jumps[c->arg]);
	  jumps[c->arg]=~tmp;
	  break;
	}
#endif /* INS_F_JUMP_WITH_ARG */

	/* FALL_THROUGH
	 *
	 * Note that the pointer in this case will be handled by the
	 * next turn through the loop.
	 */

      case I_HASARG:
      case I_ISJUMPARG:
	ins_f_byte_with_arg(c->opcode, c->arg);
	break;

      case 0:
      case I_ISJUMP:
	ins_f_byte(c->opcode);
	break;

#ifdef PIKE_DEBUG
      default:
	Pike_fatal("Unknown instruction type.\n");
#endif
      }
    }

#ifdef PIKE_DEBUG
    if (instrs[c->opcode - F_OFFSET].flags & I_HASPOINTER) {
      if ((e+1 >= length) || (c[1].opcode != F_POINTER)) {
	Pike_fatal("Expected instruction %s to be followed by a pointer.\n"
		   "Got %s (%d != %d)\n.",
		   instrs[c->opcode - F_OFFSET].name,
		   (e+1 < length)?instrs[c[1].opcode - F_OFFSET].name:"EOI",
		   (e+1 < length)?c[1].opcode:0, F_POINTER);
      }
    }
#endif /* PIKE_DEBUG */

#ifdef ALIGN_PIKE_JUMPS
    if(e+1 < length)
    {
      /* FIXME: Note that this code won't work for opcodes of type
       *        I_ISPTRJUMPARG or I_ISPTRJUMPARGS, since c may already
       *        have been advanced to the corresponding F_POINTER.
       *        With the current opcode set this is a non-issue, but...
       * /grubba 2002-11-02
       */
      switch(c->opcode)
      {
	case F_RETURN:
	case F_VOLATILE_RETURN:
	case F_BRANCH:
	case F_RETURN_0:
	case F_RETURN_1:
	case F_RETURN_LOCAL:
	  
#define CALLS(X) \
      case PIKE_CONCAT3(F_,X,_AND_RETURN): \
      case PIKE_CONCAT3(F_MARK_,X,_AND_RETURN):
	  
	  CALLS(APPLY)
	    CALLS(CALL_FUNCTION)
	    CALLS(CALL_LFUN)
	    CALLS(CALL_BUILTIN)
	    while( ((INT32) PIKE_PC & (ALIGN_PIKE_JUMPS-1) ) )
	      ins_byte(0);
      }
    }
#endif
    
    c++;
  }

  for(e=0;e<=max_label;e++)
  {
    INT32 tmp2=labels[e];

    while(jumps[e]!=-1)
    {
#ifdef PIKE_DEBUG
      if(labels[e]==-1)
	Pike_fatal("Hyperspace error: unknown jump point %ld(%ld) at %d (pc=%x).\n",
		   PTRDIFF_T_TO_LONG(e), PTRDIFF_T_TO_LONG(max_label),
		   labels[e], jumps[e]);
#endif
#ifdef INS_F_JUMP
      if(jumps[e] < 0)
      {
	tmp = READ_F_JUMP(~jumps[e]);
	UPDATE_F_JUMP(~jumps[e], tmp2);
	jumps[e]=tmp;
	continue;
      }
#endif

      tmp = read_pointer(jumps[e]);
      upd_pointer(jumps[e], tmp2 - jumps[e]);
      jumps[e]=tmp;
    }
  }

  free((char *)labels);

#ifdef PIKE_DEBUG
  if (a_flag > 6) {
    size_t len = (Pike_compiler->new_program->num_program - fun_start)*
      sizeof(PIKE_OPCODE_T);
    fprintf(stderr, "Code at offset %d through %d:\n",
	    fun_start, Pike_compiler->new_program->num_program-1);
#ifdef DISASSEMBLE_CODE
    DISASSEMBLE_CODE(Pike_compiler->new_program->program + fun_start, len);
#else /* !DISASSEMBLE_CODE */
    {
      /* FIXME: Hexdump here. */
    }
#endif /* DISASSEMBLE_CODE */
  }
#endif /* PIKE_DEBUG */

  exit_bytecode();

  return entry_point;
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
    Pike_fatal("hasarg2(%d /*%s */) is wrong!\n",f,get_f_name(f));
#endif

  p=(p_instr *)low_make_buf_space(sizeof(p_instr), &instrbuf);

  if(fifo_len)
  {
    MEMMOVE(p-fifo_len+1,p-fifo_len,fifo_len*sizeof(p_instr));
    p-=fifo_len;
  }

#ifdef PIKE_DEBUG
  if(!instrbuf.s.len)
    Pike_fatal("Low make buf space failed!!!!!!\n");
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
    Pike_fatal("hasarg(%d /* %s */) is wrong!\n",f,get_f_name(f));
#endif

  return insopt2(f,a,0,cl, cf);
}

static INLINE ptrdiff_t insopt0(int f, int cl, struct pike_string *cf)
{
#ifdef PIKE_DEBUG
  if(hasarg(f))
    Pike_fatal("hasarg(%d /* %s */) is wrong!\n",f,get_f_name(f));
#endif
  return insopt2(f,0,0,cl, cf);
}

static void debug(void)
{
  if(fifo_len > (long)instrbuf.s.len / (long)sizeof(p_instr))
    fifo_len=(long)instrbuf.s.len / (long)sizeof(p_instr);
#ifdef PIKE_DEBUG
  if(eye < 0)
    Pike_fatal("Popped beyond start of code.\n");

  if(instrbuf.s.len)
  {
    p_instr *p;
    p=(p_instr *)low_make_buf_space(0, &instrbuf);
    if(!p[-1].file)
      Pike_fatal("No file name on last instruction!\n");
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
      Pike_fatal("Popping out of instructions.\n");
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
    /* Note: The 5 below is the longest	\
     *       match prefix in the ruleset\
     */					\
    fifo_len += q + 5;			\
  }  while(0)


static void do_optimization(int topop, ...)
{
  va_list arglist;
  int q=0;
  int oplen;

  DO_OPTIMIZATION_PREQUEL(topop);

  va_start(arglist, topop);
  
  while((oplen = va_arg(arglist, int)))
  {
    q++;
    switch(oplen)
    {
#ifdef PIKE_DEBUG
      default:
	Pike_fatal("Unsupported argument number: %d\n", oplen);
	break;
#endif /* PIKE_DEBUG */

      case 1:
	{
	  int i=va_arg(arglist, int);
	  insopt0(i,cl,cf);
	}
	break;

      case 2:
	{
	  int i=va_arg(arglist, int);
	  int j=va_arg(arglist, int);
	  insopt1(i,j,cl,cf);
	}
	break;

      case 3:
	{
	  int i=va_arg(arglist, int);
	  int j=va_arg(arglist, int);
	  int k=va_arg(arglist, int);
	  insopt2(i,j,k,cl,cf);
	}
	break;
    }
  }

  va_end(arglist);

  DO_OPTIMIZATION_POSTQUEL(q);

  relabel = 1;
}

#include "peep_engine.c"

static void asm_opt(void)
{
#ifdef PIKE_DEBUG
  if(a_flag > 3)
  {
    p_instr *c;
    ptrdiff_t e, length;
    int synch_depth = 0;

    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);

    fprintf(stderr,"Before peep optimization:\n");
    for(e=0;e<length;e++,c++)
    {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr,"<<<%4d: %*s",c->line,synch_depth,"");
      dump_instr(c);
      fprintf(stderr,"\n");
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
    }
  }
#endif

  len=instrbuf.s.len/sizeof(p_instr);
  instructions=(p_instr *)instrbuf.s.str;
  instrbuf.s.str=0;
  fifo_len=0;
  init_bytecode();

  for(eye=0;eye<len || fifo_len;)
  {

#ifdef PIKE_DEBUG
    if(a_flag>6) {
      int e;
      fprintf(stderr, "#%ld,%d:",
              DO_NOT_WARN((long)eye),
              fifo_len);
      for(e=0;e<4;e++) {
        fprintf(stderr," ");
        dump_instr(instr(e));
      }
      fprintf(stderr,"\n");
    }
#endif

    low_asm_opt();
    advance();
  }

  for(eye=0;eye<len;eye++) free_string(instructions[eye].file);
  free((char *)instructions);

#ifdef PIKE_DEBUG
  if(a_flag > 4)
  {
    p_instr *c;
    ptrdiff_t e, length;
    int synch_depth = 0;

    c=(p_instr *)instrbuf.s.str;
    length=instrbuf.s.len / sizeof(p_instr);

    fprintf(stderr,"After peep optimization:\n");
    for(e=0;e<length;e++,c++)
    {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr,">>>%4d: %*s",c->line,synch_depth,"");
      dump_instr(c);
      fprintf(stderr,"\n");
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
    }
  }
#endif
}
