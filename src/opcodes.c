/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: opcodes.c,v 1.166 2004/12/30 13:41:08 grubba Exp $
*/

#include "global.h"
#include "constants.h"
#include "interpret.h"
#include "opcodes.h"
#include "pike_embed.h"

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

void add_runned(PIKE_INSTR_T instr)
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

#ifdef PIKE_USE_MACHINE_CODE
#define ADDR(X) , (void *)PIKE_CONCAT(opcode_,X)
#define NULLADDR , 0
#define ALIASADDR(X)	, (void *)(X)

#define OPCODE0(OP,DESC,FLAGS) void PIKE_CONCAT(opcode_,OP)(void);
#define OPCODE1(OP,DESC,FLAGS) void PIKE_CONCAT(opcode_,OP)(INT32);
#define OPCODE2(OP,DESC,FLAGS) void PIKE_CONCAT(opcode_,OP)(INT32,INT32);

#ifdef OPCODE_RETURN_JUMPADDR
#define OPCODE0_JUMP(OP,DESC,FLAGS) void *PIKE_CONCAT(jump_opcode_,OP)(void);
#define OPCODE1_JUMP(OP,DESC,FLAGS) void *PIKE_CONCAT(jump_opcode_,OP)(INT32);
#define OPCODE2_JUMP(OP,DESC,FLAGS) void *PIKE_CONCAT(jump_opcode_,OP)(INT32,INT32);
#define JUMPADDR(X) , (void *)PIKE_CONCAT(jump_opcode_,X)
#else  /* !OPCODE_RETURN_JUMPADDR */
#define OPCODE0_JUMP(OP, DESC, FLAGS) OPCODE0(OP, DESC, FLAGS)
#define OPCODE1_JUMP(OP, DESC, FLAGS) OPCODE1(OP, DESC, FLAGS)
#define OPCODE2_JUMP(OP, DESC, FLAGS) OPCODE2(OP, DESC, FLAGS)
#define JUMPADDR(X) ADDR(X)
#endif	/* !OPCODE_RETURN_JUMPADDR */

#define OPCODE0_PTRJUMP(OP,DESC,FLAGS) OPCODE0_JUMP(OP, DESC, FLAGS)
#define OPCODE1_PTRJUMP(OP,DESC,FLAGS) OPCODE1_JUMP(OP, DESC, FLAGS)
#define OPCODE2_PTRJUMP(OP,DESC,FLAGS) OPCODE2_JUMP(OP, DESC, FLAGS)

#define OPCODE0_RETURN(OP,DESC,FLAGS) OPCODE0_JUMP(OP, DESC, FLAGS)
#define OPCODE1_RETURN(OP,DESC,FLAGS) OPCODE1_JUMP(OP, DESC, FLAGS)
#define OPCODE2_RETURN(OP,DESC,FLAGS) OPCODE2_JUMP(OP, DESC, FLAGS)

#define OPCODE0_ALIAS(OP, DESC, FLAGS, ADDR)
#define OPCODE1_ALIAS(OP, DESC, FLAGS, ADDR)
#define OPCODE2_ALIAS(OP, DESC, FLAGS, ADDR)

#ifdef OPCODE_INLINE_BRANCH
#define OPCODE0_BRANCH(OP,DESC,FLAGS) int PIKE_CONCAT(test_opcode_,OP)(void);
#define OPCODE1_BRANCH(OP,DESC,FLAGS) int PIKE_CONCAT(test_opcode_,OP)(INT32);
#define OPCODE2_BRANCH(OP,DESC,FLAGS) int PIKE_CONCAT(test_opcode_,OP)(INT32,INT32);
#define BRANCHADDR(X) , (void *)PIKE_CONCAT(test_opcode_,X)
#else /* !OPCODE_INLINE_BRANCH */
#define OPCODE0_BRANCH		OPCODE0_PTRJUMP
#define OPCODE1_BRANCH		OPCODE1_PTRJUMP
#define OPCODE2_BRANCH		OPCODE2_PTRJUMP
#define BRANCHADDR(X) JUMPADDR(X)
#endif /* OPCODE_INLINE_BRANCH */

#define OPCODE0_TAIL(OP,DESC,FLAGS) OPCODE0(OP,DESC,FLAGS)
#define OPCODE1_TAIL(OP,DESC,FLAGS) OPCODE1(OP,DESC,FLAGS)
#define OPCODE2_TAIL(OP,DESC,FLAGS) OPCODE2(OP,DESC,FLAGS)
#define OPCODE0_TAILJUMP(OP,DESC,FLAGS) OPCODE0_JUMP(OP,DESC,FLAGS)
#define OPCODE1_TAILJUMP(OP,DESC,FLAGS) OPCODE1_JUMP(OP,DESC,FLAGS)
#define OPCODE2_TAILJUMP(OP,DESC,FLAGS) OPCODE2_JUMP(OP,DESC,FLAGS)
#define OPCODE0_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE0_PTRJUMP(OP, DESC, FLAGS)
#define OPCODE1_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE1_PTRJUMP(OP, DESC, FLAGS)
#define OPCODE2_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE2_PTRJUMP(OP, DESC, FLAGS)
#define OPCODE0_TAILRETURN(OP,DESC,FLAGS) OPCODE0_RETURN(OP, DESC, FLAGS)
#define OPCODE1_TAILRETURN(OP,DESC,FLAGS) OPCODE1_RETURN(OP, DESC, FLAGS)
#define OPCODE2_TAILRETURN(OP,DESC,FLAGS) OPCODE2_RETURN(OP, DESC, FLAGS)
#define OPCODE0_TAILBRANCH(OP,DESC,FLAGS) OPCODE0_BRANCH(OP,DESC,FLAGS)
#define OPCODE1_TAILBRANCH(OP,DESC,FLAGS) OPCODE1_BRANCH(OP,DESC,FLAGS)
#define OPCODE2_TAILBRANCH(OP,DESC,FLAGS) OPCODE2_BRANCH(OP,DESC,FLAGS)

#include "interpret_protos.h"

#undef OPCODE0
#undef OPCODE1
#undef OPCODE2
#undef OPCODE0_TAIL
#undef OPCODE1_TAIL
#undef OPCODE2_TAIL

#undef OPCODE0_JUMP
#undef OPCODE1_JUMP
#undef OPCODE2_JUMP
#undef OPCODE0_TAILJUMP
#undef OPCODE1_TAILJUMP
#undef OPCODE2_TAILJUMP

#undef OPCODE0_PTRJUMP
#undef OPCODE1_PTRJUMP
#undef OPCODE2_PTRJUMP
#undef OPCODE0_TAILPTRJUMP
#undef OPCODE1_TAILPTRJUMP
#undef OPCODE2_TAILPTRJUMP

#undef OPCODE0_RETURN
#undef OPCODE1_RETURN
#undef OPCODE2_RETURN
#undef OPCODE0_TAILRETURN
#undef OPCODE1_TAILRETURN
#undef OPCODE2_TAILRETURN

#undef OPCODE0_ALIAS
#undef OPCODE1_ALIAS
#undef OPCODE2_ALIAS

#undef OPCODE0_BRANCH
#undef OPCODE1_BRANCH
#undef OPCODE2_BRANCH
#undef OPCODE0_TAILBRANCH
#undef OPCODE1_TAILBRANCH
#undef OPCODE2_TAILBRANCH

#else
#define ADDR(X)
#define BRANCHADDR(X)
#define JUMPADDR(X)
#define NULLADDR
#define ALIASADDR(X)
#endif

#define OPCODE0(OP,DESC,FLAGS) { DESC, OP, FLAGS ADDR(OP) },
#define OPCODE1(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_HASARG ADDR(OP) },
#define OPCODE2(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_TWO_ARGS ADDR(OP) },
#define OPCODE0_TAIL(OP,DESC,FLAGS) OPCODE0(OP,DESC,FLAGS)
#define OPCODE1_TAIL(OP,DESC,FLAGS) OPCODE1(OP,DESC,FLAGS)
#define OPCODE2_TAIL(OP,DESC,FLAGS) OPCODE2(OP,DESC,FLAGS)

#define OPCODE0_PTRJUMP(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISPTRJUMP JUMPADDR(OP) },
#define OPCODE1_PTRJUMP(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISPTRJUMPARG JUMPADDR(OP) },
#define OPCODE2_PTRJUMP(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISPTRJUMPARGS JUMPADDR(OP) },
#define OPCODE0_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE0_PTRJUMP(OP,DESC,FLAGS)
#define OPCODE1_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE1_PTRJUMP(OP,DESC,FLAGS)
#define OPCODE2_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE2_PTRJUMP(OP,DESC,FLAGS)

#define OPCODE0_JUMP(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISJUMP JUMPADDR(OP) },
#define OPCODE1_JUMP(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISJUMPARG JUMPADDR(OP) },
#define OPCODE2_JUMP(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISJUMPARGS JUMPADDR(OP) },
#define OPCODE0_TAILJUMP(OP, DESC, FLAGS) OPCODE0_JUMP(OP,DESC,FLAGS)
#define OPCODE1_TAILJUMP(OP, DESC, FLAGS) OPCODE1_JUMP(OP,DESC,FLAGS)
#define OPCODE2_TAILJUMP(OP, DESC, FLAGS) OPCODE2_JUMP(OP,DESC,FLAGS)

#define OPCODE0_RETURN(OP, DESC, FLAGS) OPCODE0_JUMP(OP,DESC,FLAGS)
#define OPCODE1_RETURN(OP, DESC, FLAGS) OPCODE1_JUMP(OP,DESC,FLAGS)
#define OPCODE2_RETURN(OP, DESC, FLAGS) OPCODE2_JUMP(OP,DESC,FLAGS)
#define OPCODE0_TAILRETURN(OP, DESC, FLAGS) OPCODE0_RETURN(OP,DESC,FLAGS)
#define OPCODE1_TAILRETURN(OP, DESC, FLAGS) OPCODE1_RETURN(OP,DESC,FLAGS)
#define OPCODE2_TAILRETURN(OP, DESC, FLAGS) OPCODE2_RETURN(OP,DESC,FLAGS)

#define OPCODE0_ALIAS(OP, DESC, FLAGS, A) { DESC, OP, FLAGS ALIASADDR(A) },
#define OPCODE1_ALIAS(OP, DESC, FLAGS, A) { DESC, OP, FLAGS | I_HASARG ALIASADDR(A) },
#define OPCODE2_ALIAS(OP, DESC, FLAGS, A) { DESC, OP, FLAGS | I_TWO_ARGS ALIASADDR(A) },

#define OPCODE0_BRANCH(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISBRANCH BRANCHADDR(OP) },
#define OPCODE1_BRANCH(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISBRANCHARG BRANCHADDR(OP) },
#define OPCODE2_BRANCH(OP,DESC,FLAGS) { DESC, OP, FLAGS | I_ISBRANCHARGS BRANCHADDR(OP) },
#define OPCODE0_TAILBRANCH(OP,DESC,FLAGS) OPCODE0_BRANCH(OP,DESC,FLAGS)
#define OPCODE1_TAILBRANCH(OP,DESC,FLAGS) OPCODE1_BRANCH(OP,DESC,FLAGS)
#define OPCODE2_TAILBRANCH(OP,DESC,FLAGS) OPCODE2_BRANCH(OP,DESC,FLAGS)

const struct keyword instr_names[]=
{
#ifndef PIKE_PRECOMPILER
#include "interpret_protos.h"
#endif /* !PIKE_PRECOMPILER */
{ "%=",			F_MOD_EQ,0 NULLADDR },	
{ "&=",			F_AND_EQ,0 NULLADDR },	
{ "|=",			F_OR_EQ,0 NULLADDR },	
{ "*=",			F_MULT_EQ,0 NULLADDR },	
{ "+=",			F_ADD_EQ,0 NULLADDR },	
{ "-=",			F_SUB_EQ,0 NULLADDR },	
{ "/=",			F_DIV_EQ,0 NULLADDR },	
{ "<<=",		F_LSH_EQ,0 NULLADDR },	
{ ">>=",		F_RSH_EQ,0 NULLADDR },	
{ "^=",			F_XOR_EQ,0 NULLADDR },	
{ "arg+=1024",		F_PREFIX_1024,0 NULLADDR },
{ "arg+=256",		F_PREFIX_256,0 NULLADDR },
{ "arg+=256*X",		F_PREFIX_CHARX256,0 NULLADDR },
{ "arg+=256*XX",	F_PREFIX_WORDX256,0 NULLADDR },
{ "arg+=256*XXX",	F_PREFIX_24BITX256,0 NULLADDR },
{ "arg+=512",		F_PREFIX_512,0 NULLADDR },
{ "arg+=768",		F_PREFIX_768,0 NULLADDR },

{ "arg+=1024",		F_PREFIX2_1024,0 NULLADDR },
{ "arg+=256",		F_PREFIX2_256,0 NULLADDR },
{ "arg+=256*X",		F_PREFIX2_CHARX256,0 NULLADDR },
{ "arg+=256*XX",	F_PREFIX2_WORDX256,0 NULLADDR },
{ "arg+=256*XXX",	F_PREFIX2_24BITX256,0 NULLADDR },
{ "arg+=512",		F_PREFIX2_512,0 NULLADDR },
{ "arg+=768",		F_PREFIX2_768,0 NULLADDR },

{ "break",		F_BREAK,0 NULLADDR },	
{ "case",		F_CASE,0 NULLADDR },	
{ "continue",		F_CONTINUE,0 NULLADDR },	
{ "default",		F_DEFAULT,0 NULLADDR },	
{ "do-while",		F_DO,0 NULLADDR },	
{ "for",		F_FOR,0 NULLADDR },

{ "pointer",		F_POINTER, I_ISPOINTER NULLADDR },
{ "data",		F_DATA, I_DATA NULLADDR },
{ "byte",		F_BYTE, I_DATA NULLADDR },
{ "lvalue_list",	F_LVALUE_LIST,0 NULLADDR },	
{ "label",		F_LABEL,I_HASARG NULLADDR },
{ "align",		F_ALIGN, I_HASARG NULLADDR },
{ "nop",                F_NOP,0 NULLADDR },
{ "entry",		F_ENTRY,0 NULLADDR },
{ "function start",     F_START_FUNCTION,0 NULLADDR },
{ "notreached!",        F_NOTREACHED, 0 NULLADDR },
};

struct instr instrs[F_MAX_INSTR - F_OFFSET];
#ifdef PIKE_USE_MACHINE_CODE
size_t instrs_checksum;
#endif /* PIKE_USE_MACHINE_CODE */

const char *low_get_f_name(int n, struct program *p)
{
  static char buf[30];
  
  if (n<F_MAX_OPCODE)
  {
    if ((n >= 0) && instrs[n-F_OFFSET].name)
      return instrs[n-F_OFFSET].name;
    sprintf(buf, "<OTHER %d>", n);
    return buf;
  }

  if(p &&
     (int)p->num_constants > (int)(n-F_MAX_OPCODE) &&
     p->constants[n-F_MAX_OPCODE].sval.type==T_FUNCTION &&
     (p->constants[n-F_MAX_OPCODE].sval.subtype == FUNCTION_BUILTIN) &&
     p->constants[n-F_MAX_OPCODE].sval.u.efun) {
    return p->constants[n-F_MAX_OPCODE].sval.u.efun->name->str;
  }

  sprintf(buf, "Call efun %d", n - F_MAX_OPCODE);
  return buf;
}

const char *get_f_name(int n)
{
  if (Pike_fp && Pike_fp->context.prog)
    return low_get_f_name(n, Pike_fp->context.prog);
  return low_get_f_name(n, NULL);
}

#ifdef HAVE_COMPUTED_GOTO
char *get_opcode_name(PIKE_INSTR_T n)
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

const char *get_token_name(int n)
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

void init_opcodes(void)
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
#ifdef PIKE_USE_MACHINE_CODE
    instrs[instr_names[i].token - F_OFFSET].address=instr_names[i].address;
#endif
  }

#ifdef PIKE_USE_MACHINE_CODE
  instrs_checksum = hashmem((const unsigned char*)instrs, sizeof(instrs),
			    sizeof(instrs));
  /* fprintf(stderr, "Instruction checksum: %d\n", instrs_checksum); */
#endif /* PIKE_USE_MACHINE_CODE */

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
    Pike_fatal("Found %d errors in instrs.\n",fatal_later);

#endif

}

void exit_opcodes(void)
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
