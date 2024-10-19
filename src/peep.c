/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#include "global.h"
#include "stralloc.h"
#include "buffer.h"
#include "program.h"
#include "las.h"
#include "docode.h"
#include "pike_embed.h"
#include "pike_error.h"
#include "lex.h"
#include "pike_memory.h"
#include "peep.h"
#include "bignum.h"
#include "opcodes.h"
#include "threads.h"
#include "builtin_functions.h"
#include "constants.h"
#include "interpret.h"
#include "pikecode.h"
#include "pike_compiler.h"
#include "pike_types.h"

static int hasarg(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_HASARG;
}

static int hasarg2(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_HASARG2;
}

static int arg_type(int opcode)
{
  return instrs[opcode-F_OFFSET].flags & I_ARG_T_MASK;
}

static int arg2_type(int opcode)
{
  return (instrs[opcode-F_OFFSET].flags >> I_ARG2_T_SHIFT) & I_ARG_T_MASK;
}

static void dump_arg(int arg, int argt)
{
  switch(argt) {
  case I_ARG_T_CONST:	/* FIXME */
  default:
    fprintf(stderr, "%d", arg);
    break;
  case I_ARG_T_LOCAL:
    fprintf(stderr, "$%d", arg);
    break;
  case I_ARG_T_RTYPE:
    fprintf(stderr, "%s", get_name_of_type(arg));
    break;
  case I_ARG_T_GLOBAL:
    safe_pike_fprintf(stderr, "%pS",
                      ID_FROM_INT(Pike_compiler->new_program, arg)->name);
    break;
  case I_ARG_T_STRING:
    safe_pike_fprintf(stderr, "%pq",
                      Pike_compiler->new_program->strings[arg]);
    break;
  }
}

static void dump_instr(p_instr *p)
{
  if(!p) return;
  fprintf(stderr,"%s",get_token_name(p->opcode));
  if(hasarg(p->opcode))
  {
    fprintf(stderr, "(");
    dump_arg(p->arg, arg_type(p->opcode));
    if(hasarg2(p->opcode)) {
      fprintf(stderr, ", ");
      dump_arg(p->arg2, arg2_type(p->opcode));
    }
    fprintf(stderr,")");
  }
}


static int asm_opt(void);

/* Output buffer. The optimization eye is at the end of the buffer. */
struct byte_buffer instrbuf;
long num_instrs = 0;


void init_bytecode(void)
{
  buffer_init(&instrbuf);
  buffer_ensure_space(&instrbuf, 512);
  num_instrs = 0;
}

void exit_bytecode(void)
{
  ptrdiff_t e, length;
  p_instr *c;

  c=(p_instr *)buffer_ptr(&instrbuf);
  length=buffer_content_length(&instrbuf) / sizeof(p_instr);

  for(e=0;e<length;e++) {
    free_string(dmalloc_touch_named(struct pike_string *, c[e].file,
				    "exit_bytecode"));
#ifdef PIKE_DEBUG
    c[e].file = (void *)(ptrdiff_t)~0;
#endif
  }

  buffer_free(&instrbuf);
}

/* insert_opcode{,0,1,2}() append an opcode to the instrbuf. */

ptrdiff_t insert_opcode(p_instr *opcode)
{
  /* Note: Steals references from opcode. */
  p_instr *p = (p_instr *)buffer_alloc(&instrbuf, sizeof(p_instr));
  if (!p) Pike_fatal("Out of memory in peep.\n");
  *p = *opcode;
  num_instrs++;
  return p - (p_instr *)buffer_ptr(&instrbuf);
}

ptrdiff_t insert_opcode2(unsigned int f,
			 INT32 b,
			 INT32 c,
			 INT_TYPE current_line,
			 struct pike_string *current_file)
{
  p_instr *p;

#ifdef PIKE_DEBUG
  if(!hasarg2(f) && c)
    Pike_fatal("hasarg2(%d) is wrong!\n",f);
#endif

  p=(p_instr *)buffer_alloc(&instrbuf, sizeof(p_instr));


#ifdef PIKE_DEBUG
  if(!buffer_content_length(&instrbuf))
    Pike_fatal("Low make buf space failed!!!!!!\n");
#endif

  p->opcode=f;
  p->line=current_line;
  copy_shared_string(p->file, dmalloc_touch_named(struct pike_string *,
						  current_file,
						  "insert_opcode"));
  p->arg=b;
  p->arg2=c;

  return p - (p_instr *)buffer_ptr(&instrbuf);
}

ptrdiff_t insert_opcode1(unsigned int f,
			 INT32 b,
			 INT_TYPE current_line,
			 struct pike_string *current_file)
{
#ifdef PIKE_DEBUG
  if(!hasarg(f) && b)
    Pike_fatal("hasarg(%d) is wrong!\n",f);
#endif

  return insert_opcode2(f,b,0,current_line,current_file);
}

ptrdiff_t insert_opcode0(int f, INT_TYPE current_line,
			 struct pike_string *current_file)
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
  if(instr > (long)buffer_content_length(&instrbuf) / (long)sizeof(p_instr) || instr < 0)
    Pike_fatal("update_arg outside known space.\n");
#endif
  p=(p_instr *)buffer_ptr(&instrbuf);
  p[instr].arg=arg;
}

#ifndef FLUSH_CODE_GENERATOR_STATE
#define FLUSH_CODE_GENERATOR_STATE()
#endif

/**** Bytecode Generator *****/

INT32 assemble(int store_linenumbers)
{
  INT32 entry_point;
  INT32 max_label=-1,tmp;
  INT32 *labels, *jumps, *uses, *aliases;
  ptrdiff_t e, length;
  p_instr *c;
  struct pike_string *triples = NULL;
#ifdef PIKE_DEBUG
  INT32 max_pointer=-1;
  int synch_depth = 0;
  size_t fun_start = Pike_compiler->new_program->num_program;
  struct pike_string *prev_file = NULL;
#endif
  int relabel;
  int reoptimize = relabel = !(debug_options & NO_PEEP_OPTIMIZING);
  struct lex *lex;
  CHECK_COMPILER();
  lex = &THIS_COMPILATION->lex;

  c=(p_instr *)buffer_ptr(&instrbuf);
  length=buffer_content_length(&instrbuf) / sizeof(p_instr);

#ifdef PIKE_DEBUG
  if((a_flag > 1 && store_linenumbers) || a_flag > 2)
  {
    for (e = 0; e < length; e++) {
      if (c[e].opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr, "~~~%4ld %4lx %*s", (long)c[e].line,
              (unsigned long)e, synch_depth, "");
      dump_instr(c+e);
      fprintf(stderr,"\n");
      if (c[e].opcode == F_SYNCH_MARK) synch_depth++;
    }
    if (synch_depth) {
      Pike_fatal("Unbalanced sync_mark/pop_sync_mark: %d\n", synch_depth);
    }
  }
#endif

  /* No need to do this for constant evaluations. */
  if (store_linenumbers) {
    p_wchar2 *current_triple;
    struct pike_string *previous_file = NULL;
    INT_TYPE previous_line = 0;
    ptrdiff_t num_linedirectives = 0;

    /* Count the number of F_FILENAME/F_LINE pseudo-ops we need to add. */
    for (e=0; e < length; e++) {
      if (c[e].file != previous_file) {
	previous_file = dmalloc_touch_named(struct pike_string *,
					    c[e].file, "prev_file");
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

    if (!(triples = begin_wide_shared_string(3*(length+num_linedirectives),
					      2))) {
      Pike_fatal("Failed to allocate wide string of length %"PRINTPTRDIFFT"d "
                 "3*(%"PRINTPTRDIFFT"d + %"PRINTPTRDIFFT"d).\n",
		 3*(length+num_linedirectives), length, num_linedirectives);
    }
    previous_file = NULL;
    previous_line = 0;
    current_triple = STR2(triples);
    for (e = 0; e < length; e++) {
      if (c[e].file != previous_file) {
	current_triple[0] = F_FILENAME;
	current_triple[1] =
	  store_prog_string(dmalloc_touch_named(struct pike_string *,
						c[e].file,
						"store_prog_string"));
	current_triple[2] = 0;
	current_triple += 3;
	previous_file = dmalloc_touch_named(struct pike_string *,
					    c[e].file, "prev_file");
      }
      if (c[e].line != previous_line) {
	current_triple[0] = F_LINE;
	current_triple[1] = c[e].line;
#if SIZEOF_INT_TYPE > 4
	current_triple[2] = c[e].line>>32;
#else
        current_triple[2] = 0;
#endif
	current_triple += 3;
	previous_line = c[e].line;
      }
      current_triple[0] = c[e].opcode;
      current_triple[1] = c[e].arg;
      current_triple[2] = c[e].arg2;
      current_triple += 3;
    }
#ifdef PIKE_DEBUG
    if (current_triple != STR2(triples) + 3*(length + num_linedirectives)) {
      Pike_fatal("Triple length mismatch %"PRINTPTRDIFFT"d "
                 "!= %"PRINTPTRDIFFT"d 3*(%"PRINTPTRDIFFT"d "
                 "+ %"PRINTPTRDIFFT"d)\n",
		 current_triple - STR2(triples),
		 3*(length + num_linedirectives),
		 length, num_linedirectives);
    }
#endif /* PIKE_DEBUG */
    triples = end_shared_string(triples);
  }

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
    c=(p_instr *)buffer_ptr(&instrbuf);
    for(e=0;e<length;e++,c++) {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr, " * %4ld %4lx ",
              (long)c->line, (unsigned long)e);
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

#ifndef INS_ENTRY
  /* Replace F_ENTRY with F_NOP if we have no entry prologue. */
  for (c = (p_instr *) buffer_ptr(&instrbuf), e = 0; e < length; e++, c++)
    if (c->opcode == F_ENTRY) c->opcode = F_NOP;
#endif

  labels=xalloc(sizeof(INT32) * 4 * (max_label+2));
  jumps = labels + max_label + 2;
  aliases = jumps + max_label + 2;
  uses = aliases + max_label + 2;
  while(relabel)
  {
    /* First do the relabel pass. */
    /* Set labels, jumps and aliases to all -1. */
    memset(labels, 0xff, ((max_label + 2) * 3) * sizeof(INT32));
    memset(uses, 0x00, (max_label + 2) * sizeof(INT32));

    c=(p_instr *)buffer_ptr(&instrbuf);
    length=buffer_content_length(&instrbuf) / sizeof(p_instr);
    for(e=0;e<length;e++)
      if(c[e].opcode == F_LABEL && c[e].arg>=0) {
	INT32 l = c[e].arg;
        labels[l]=(INT32)e;
	while (e+1 < length &&
	       c[e+1].opcode == F_LABEL && c[e+1].arg >= 0) {
	  /* aliases is used to compact several labels at the same
	   * position to one label. That's necessary for some peep
	   * optimizations to work well. */
	  e++;
          labels[c[e].arg] = (INT32)e;
	  aliases[c[e].arg] = l;
	}
      }

    for(e=0;e<length;e++)
    {
      if(instrs[c[e].opcode-F_OFFSET].flags & I_POINTER)
      {
	if (aliases[c[e].arg] >= 0) c[e].arg = aliases[c[e].arg];

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
	      /* FALLTHRU */
	    case TWOO(F_LOR,F_LOR):
	      c[e].arg=c[tmp].arg;
	      continue;

	    case TWOO(F_LAND,F_BRANCH_WHEN_ZERO):
	      c[e].opcode=F_BRANCH_WHEN_ZERO;
	      /* FALLTHRU */
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
	      if(c[e].file) {
		free_string(dmalloc_touch_named(struct pike_string *,
						c[e].file, "branch_opt 1"));
	      }
	      c[e]=c[tmp];
	      if(c[e].file) {
		add_ref(dmalloc_touch_named(struct pike_string *,
					    c[e].file, "branch_opt 2"));
	      }
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

    if(!reoptimize) {
#ifdef PIKE_DEBUG
      if (a_flag > 3) fprintf (stderr, "Optimizer done after relabel.\n");
#endif
      break;
    }

    /* Then do the optimize pass. */

    relabel = asm_opt();

    reoptimize = 0;

    if (!relabel) {
#ifdef PIKE_DEBUG
      if (a_flag > 3) fprintf (stderr, "Optimizer done after asm_opt.\n");
#endif
      break;
    }

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
  c=(p_instr *)buffer_ptr(&instrbuf);
  length=buffer_content_length(&instrbuf) / sizeof(p_instr);

  for(e=0;e<=max_label;e++) labels[e]=jumps[e]=-1;


#ifdef START_NEW_FUNCTION
  START_NEW_FUNCTION(store_linenumbers);
#endif

#ifdef ALIGN_PIKE_FUNCTION_BEGINNINGS
  while( ( (((INT32) PIKE_PC)+4) & (ALIGN_PIKE_JUMPS-1)))
    ins_byte(0);
#endif

  if (store_linenumbers) {
    ins_data(store_prog_string(triples));
    free_string(triples);
  } else {
    ins_data(0);
  }

  entry_point = PIKE_PC;

#ifdef PIKE_DEBUG
  synch_depth = 0;
#endif
  FLUSH_CODE_GENERATOR_STATE();
  for(e=0;e<length;e++)
  {
#ifdef DISASSEMBLE_CODE
    size_t opcode_start = PIKE_PC;
#endif
#ifdef PIKE_DEBUG
    if (c != (((p_instr *)buffer_ptr(&instrbuf))+e)) {
      Pike_fatal("Instruction loop deviates. "
		 "0x%04"PRINTPTRDIFFT"x != 0x%04"PRINTPTRDIFFT"x\n",
		 e, c - ((p_instr *)buffer_ptr(&instrbuf)));
    }
#endif
    if(
#ifdef PIKEDEBUG
       ((a_flag > 2) && store_linenumbers) ||
       (a_flag > 3) ||
#endif
       (lex->pragmas & ID_DISASSEMBLE))
    {
#ifdef PIKE_DEBUG
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      if (c->file != prev_file) {
        fprintf(stderr, "===           %*s.file \"%s\"\n",
                synch_depth, "", c->file?c->file->str:"");
        prev_file = c->file;
      }
      fprintf(stderr, "=== %4ld %4lx %*s", (long)c->line,
              (unsigned long)PIKE_PC, synch_depth, "");
#else
      fprintf(stderr, "=== %4ld %4lx ", (long)c->line, (unsigned long)PIKE_PC);
#endif
      dump_instr(c);
      fprintf(stderr,"\n");
#ifdef PIKE_DEBUG
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
#endif
    }

    if(store_linenumbers && (c->opcode != F_ALIGN)) {
      int new_line =
	store_linenumber(c->line, dmalloc_touch_named(struct pike_string *,
						      c->file,
						      "store_line"));
#ifdef PIKE_DEBUG
      if (c->opcode < F_MAX_OPCODE)
	ADD_COMPILED(c->opcode);
#endif /* PIKE_DEBUG */
#ifdef INS_TRACE_POINT
      if (new_line && (c->opcode != F_ENTRY) &&
	  (c->opcode != F_BYTE) && (c->opcode != F_DATA) &&
	  (c->opcode != F_LABEL) && (c->opcode != F_POINTER)) {
	INS_TRACE_POINT();
      }
#endif
    }

    switch(c->opcode)
    {
    case F_START_FUNCTION:
#ifdef INS_START_FUNCTION
      INS_START_FUNCTION();
#endif
      break;
    case F_NOP:
    case F_NOTREACHED:
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

    case F_SET_LOCAL_NAME:
      if (store_linenumbers) {
	store_linenumber_local_name(c->arg, c->arg2);
      }
      break;

    case F_SET_LOCAL_TYPE:
      if (store_linenumbers) {
	store_linenumber_local_type(c->arg, c->arg2);
      }
      break;

    case F_SET_LOCAL_FLAGS:
      Pike_compiler->compiler_frame->local_names[c->arg].flags |= c->arg2;
      break;

    case F_SET_LOCAL_END:
      if (store_linenumbers) {
	store_linenumber_local_end(c->arg);
      }
      break;

    case F_ENTRY:
#ifdef INS_ENTRY
      INS_ENTRY();
#else
      Pike_fatal ("F_ENTRY is supposed to be gone here.\n");
#endif
      break;

    case F_LABEL:
      if(c->arg == -1) {
#ifdef PIKE_DEBUG
	if (!(debug_options & NO_PEEP_OPTIMIZING))
	  Pike_fatal ("Failed to optimize away an unused label.\n");
#endif
	break;
      }
#ifdef PIKE_DEBUG
      if(c->arg > max_label || c->arg < 0)
	Pike_fatal("max_label calculation failed!\n");

      if(labels[c->arg] != -1)
	Pike_fatal("Duplicate label %d!\n", c->arg);
#endif
      FLUSH_CODE_GENERATOR_STATE();
      labels[c->arg] = (INT32)PIKE_PC;
      if ((e+1 < length) &&
	  (c[1].opcode != F_LABEL) &&
	  (c[1].opcode != F_BYTE) &&
	  (c[1].opcode != F_ENTRY) &&
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
      tmp = (INT32)PIKE_PC;
      ins_pointer(jumps[c->arg]);
      jumps[c->arg]=tmp;
      break;

    case F_VOLATILE_RECUR:
      c->opcode = F_RECUR;
      /* FALLTHRU */

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
        tmp = (INT32)PIKE_PC;
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

	/* FALLTHRU */
	/*
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

	/* FALLTHRU */
	/*
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

#ifdef DISASSEMBLE_CODE
    if((lex->pragmas & ID_DISASSEMBLE) && PIKE_PC > opcode_start)
    {
        DISASSEMBLE_CODE(Pike_compiler->new_program->program + opcode_start,
                         (PIKE_PC - opcode_start)*sizeof(PIKE_OPCODE_T));
    }
#endif

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
                   (long)e, (long)max_label, labels[e], jumps[e]);
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

  free(labels);

#ifdef END_FUNCTION
  END_FUNCTION(store_linenumbers);
#endif

#ifdef PIKE_DEBUG
  if (a_flag > 6) {
    size_t len = (Pike_compiler->new_program->num_program - fun_start)*
      sizeof(PIKE_OPCODE_T);
    fprintf(stderr, "Code at offset %"PRINTSIZET"d through %"PRINTSIZET"d:\n",
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

static void do_optimization(int topop, int topush, ...);
static inline int opcode(int offset);
static inline int argument(int offset);
static inline int argument2(int offset);
static inline unsigned int check_local_var_flag(int var, int flag);

#include "peep_engine.c"

#ifndef PEEP_STACK_SIZE
#define PEEP_STACK_SIZE 256
#endif

/* Stack holding pending instructions.
 * Note that instructions must be pushed in reverse order.
 */
static long stack_depth = 0;
static p_instr instrstack[PEEP_STACK_SIZE];

int remove_clear_locals=0x7fffffff;
static ptrdiff_t eye, len;
static p_instr *instructions;

/* insopt{0,1,2} push an instruction on instrstack. */

static inline p_instr *insopt2(int f, INT32 a, INT32 b,
			       INT_TYPE cl, struct pike_string *cf)
{
  p_instr *p;

#ifdef PIKE_DEBUG
  if(!hasarg2(f) && b)
    Pike_fatal("hasarg2(%d /*%s */) is wrong!\n",f,get_f_name(f));
#endif

  p = instrstack + stack_depth++;

#ifdef PIKE_DEBUG
  if(stack_depth > PEEP_STACK_SIZE)
    Pike_fatal("Instructions stacked too high!!!!!!\n");
#endif

  p->opcode=f;
  p->line=cl;
  copy_shared_string(p->file, dmalloc_touch_named(struct pike_string *,
						  cf, "insopt2"));
  p->arg=a;
  p->arg2=b;

  return p;
}

static inline p_instr *insopt1(int f, INT32 a, INT_TYPE cl,
			       struct pike_string *cf)
{
#ifdef PIKE_DEBUG
  if(!hasarg(f) && a)
    Pike_fatal("hasarg(%d /* %s */) is wrong!\n",f,get_f_name(f));
#endif

  return insopt2(f,a,0,cl, cf);
}

static inline p_instr *insopt0(int f, INT_TYPE cl, struct pike_string *cf)
{
#ifdef PIKE_DEBUG
  if(hasarg(f))
    Pike_fatal("hasarg(%d /* %s */) is wrong!\n",f,get_f_name(f));
#endif
  return insopt2(f,0,0,cl, cf);
}

#ifdef PIKE_DEBUG
static void debug(void)
{
  if (num_instrs != (long)buffer_content_length(&instrbuf) / (long)sizeof(p_instr)) {
    Pike_fatal("PEEP: instrbuf lost count (%ld != %ld)\n",
	       num_instrs, (long)buffer_content_length(&instrbuf) / (long)sizeof(p_instr));
  }
  if(buffer_content_length(&instrbuf))
  {
    p_instr *p;
    p=(p_instr *)buffer_dst(&instrbuf);
    if(!p[-1].file)
      Pike_fatal("No file name on last instruction!\n");
  }
}
#else
#define debug()
#endif


/* Offset from the end of instrbuf backwards. */
static inline p_instr *instr(int offset)
{
  if (offset >= num_instrs) return NULL;
  return ((p_instr *)buffer_dst(&instrbuf)) - (offset + 1);
}

static inline int opcode(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->opcode;
  return -1;
}

static inline int argument(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->arg;
  return -1;
}

static inline int argument2(int offset)
{
  p_instr *a;
  a=instr(offset);
  if(a) return a->arg2;
  return -1;
}

/* This is used from two peep.in rules, merely because coverity did not
 * like accessing ...->local_names[arguments(1)], as arguments() could in theory
 * return a negative value. Neither very pretty nor generic... */
static inline unsigned int check_local_var_flag(int var, int flag)
{
  if (var >= 0)
    return Pike_compiler->compiler_frame->local_names[var].flags & flag;

  return 0;
}


static int advance(void)
{
  p_instr *p;
  if(stack_depth)
  {
    p = instrstack + --stack_depth;
  }else{
    if (eye >= len) return 0;
    p = instructions + eye;
    eye++;
  }
  insert_opcode(p);
  debug_malloc_touch_named(p->file, "advance");
  debug();
  return 1;
}

static void pop_n_opcodes(int n)
{
  int e;
  p_instr *p;

#ifdef PIKE_DEBUG
  if (n > num_instrs)
    Pike_fatal("Popping out of instructions.\n");
#endif

  p = ((p_instr *)buffer_dst(&instrbuf)) - n;
  for (e = 0; e < n; e++) {
    free_string(dmalloc_touch_named(struct pike_string *, p[e].file,
				    "pop_n_opcodes"));
  }
  num_instrs -= n;
  buffer_remove(&instrbuf, sizeof(p_instr)*n);
}


/* NOTE: Called with opcodes in reverse order! */
static void do_optimization(int topop, int topush, ...)
{
  va_list arglist;
  int q=0;
  int oplen;
  struct pike_string *cf;
  INT_TYPE cl=instr(0)->line;

#ifdef PIKE_DEBUG
  if(a_flag>5)
  {
    int e;
    fprintf(stderr, "PEEP at %ld:", cl);
    for(e = topop; e--;)
    {
      fprintf(stderr," ");
      dump_instr(instr(e));
    }
    fprintf(stderr," => ");
  }
#endif

  if (stack_depth + topush > PEEP_STACK_SIZE) {
    /* No place left on stack. Ignore the optimization. */
#ifdef PIKE_DEBUG
    if (a_flag) {
      fprintf(stderr, "PEEP stack full.\n");
    }
#endif
    return;
  }

  copy_shared_string(cf,dmalloc_touch_named(struct pike_string *,
					    instr(0)->file,
					    "do_optimization"));
  pop_n_opcodes(topop);

  va_start(arglist, topush);

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

  /*fifo_len+=q;*/
  free_string(dmalloc_touch_named(struct pike_string *, cf,
				  "do_optimization"));
  debug();

#ifdef PIKE_DEBUG
  if(a_flag>5)
  {
    p_instr *p = instrstack + stack_depth;
    int e;
    for(e=0;e<q;e++)
    {
      fprintf(stderr," ");
      dump_instr(p-(e+1));
    }
    fprintf(stderr,"\n");
  }
  if (q != topush) {
    Pike_fatal("PEEP: Lost track of instructions to push (%d != %d)\n",
	       q, topush);
  }
#endif

  /* Note: The 5 below is the longest
   *       match prefix in the ruleset
   */
  /*fifo_len += q + 5;*/
}

static int asm_opt(void)
{
  int relabel = 0;

#ifdef PIKE_DEBUG
  if(a_flag > 3)
  {
    p_instr *c;
    ptrdiff_t e, length;
    int synch_depth = 0;

    c=(p_instr *)buffer_ptr(&instrbuf);
    length=buffer_content_length(&instrbuf) / sizeof(p_instr);

    fprintf(stderr,"Before peep optimization:\n");
    for(e=0;e<length;e++,c++)
    {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr,"<<<%4ld: %*s",(long)c->line,synch_depth,"");
      dump_instr(c);
      fprintf(stderr,"\n");
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
    }
  }
#endif

  len=buffer_content_length(&instrbuf)/sizeof(p_instr);
  instructions=(p_instr *)buffer_ptr(&instrbuf);
  init_bytecode();

  for(eye = 0; advance();)
  {
#ifdef PIKE_DEBUG
    if(a_flag>6) {
      int e;
      fprintf(stderr, "#%ld,%ld:", (long)eye, stack_depth);
      for(e = 4;e--;) {
        fprintf(stderr," ");
        dump_instr(instr(e));
      }
      /* FIXME: Show instrstack too? */
      fprintf(stderr,"\n");
    }
#endif

    relabel |= low_asm_opt();
  }

  free(instructions);

#ifdef PIKE_DEBUG
  if(a_flag > 4)
  {
    p_instr *c;
    ptrdiff_t e, length;
    int synch_depth = 0;

    c=(p_instr *)buffer_ptr(&instrbuf);
    length=buffer_content_length(&instrbuf) / sizeof(p_instr);

    fprintf(stderr,"After peep optimization:\n");
    for(e=0;e<length;e++,c++)
    {
      if (c->opcode == F_POP_SYNCH_MARK) synch_depth--;
      fprintf(stderr,">>>%4ld: %*s",(long)c->line,synch_depth,"");
      dump_instr(c);
      fprintf(stderr,"\n");
      if (c->opcode == F_SYNCH_MARK) synch_depth++;
    }
  }
#endif

  return relabel;
}
