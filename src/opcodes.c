/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
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
  struct instr_counter* next[MAX_SUPPORTED_INSTR + 1];
};

int last_instruction[MAX_SUPPORTED_INSTR+1];
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
  ((char **)(tmp))[0]++;
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

#define OPCODE0_RETURN(OP,DESC,FLAGS) OPCODE0_JUMP(OP, DESC, FLAGS | I_RETURN)
#define OPCODE1_RETURN(OP,DESC,FLAGS) OPCODE1_JUMP(OP, DESC, FLAGS | I_RETURN)
#define OPCODE2_RETURN(OP,DESC,FLAGS) OPCODE2_JUMP(OP, DESC, FLAGS | I_RETURN)

#define OPCODE0_ALIAS(OP, DESC, FLAGS, ADDR)
#define OPCODE1_ALIAS(OP, DESC, FLAGS, ADDR)
#define OPCODE2_ALIAS(OP, DESC, FLAGS, ADDR)

#ifdef OPCODE_INLINE_BRANCH
#define OPCODE0_BRANCH(OP,DESC,FLAGS) ptrdiff_t PIKE_CONCAT(test_opcode_,OP)(void);
#define OPCODE1_BRANCH(OP,DESC,FLAGS) ptrdiff_t PIKE_CONCAT(test_opcode_,OP)(INT32);
#define OPCODE2_BRANCH(OP,DESC,FLAGS) ptrdiff_t PIKE_CONCAT(test_opcode_,OP)(INT32,INT32);
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

#define OPCODE0(OP,DESC,FLAGS) { DESC, FLAGS ADDR(OP) },
#define OPCODE1(OP,DESC,FLAGS) { DESC, FLAGS | I_HASARG ADDR(OP) },
#define OPCODE2(OP,DESC,FLAGS) { DESC, FLAGS | I_TWO_ARGS ADDR(OP) },
#define OPCODE0_TAIL(OP,DESC,FLAGS) OPCODE0(OP,DESC,FLAGS)
#define OPCODE1_TAIL(OP,DESC,FLAGS) OPCODE1(OP,DESC,FLAGS)
#define OPCODE2_TAIL(OP,DESC,FLAGS) OPCODE2(OP,DESC,FLAGS)

#define OPCODE0_PTRJUMP(OP,DESC,FLAGS) { DESC, FLAGS | I_ISPTRJUMP JUMPADDR(OP) },
#define OPCODE1_PTRJUMP(OP,DESC,FLAGS) { DESC, FLAGS | I_ISPTRJUMPARG JUMPADDR(OP) },
#define OPCODE2_PTRJUMP(OP,DESC,FLAGS) { DESC, FLAGS | I_ISPTRJUMPARGS JUMPADDR(OP) },
#define OPCODE0_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE0_PTRJUMP(OP,DESC,FLAGS)
#define OPCODE1_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE1_PTRJUMP(OP,DESC,FLAGS)
#define OPCODE2_TAILPTRJUMP(OP,DESC,FLAGS) OPCODE2_PTRJUMP(OP,DESC,FLAGS)

#define OPCODE0_JUMP(OP,DESC,FLAGS) { DESC, FLAGS | I_ISJUMP JUMPADDR(OP) },
#define OPCODE1_JUMP(OP,DESC,FLAGS) { DESC, FLAGS | I_ISJUMPARG JUMPADDR(OP) },
#define OPCODE2_JUMP(OP,DESC,FLAGS) { DESC, FLAGS | I_ISJUMPARGS JUMPADDR(OP) },
#define OPCODE0_TAILJUMP(OP, DESC, FLAGS) OPCODE0_JUMP(OP,DESC,FLAGS)
#define OPCODE1_TAILJUMP(OP, DESC, FLAGS) OPCODE1_JUMP(OP,DESC,FLAGS)
#define OPCODE2_TAILJUMP(OP, DESC, FLAGS) OPCODE2_JUMP(OP,DESC,FLAGS)

#define OPCODE0_RETURN(OP, DESC, FLAGS) OPCODE0_JUMP(OP,DESC,FLAGS | I_RETURN)
#define OPCODE1_RETURN(OP, DESC, FLAGS) OPCODE1_JUMP(OP,DESC,FLAGS | I_RETURN)
#define OPCODE2_RETURN(OP, DESC, FLAGS) OPCODE2_JUMP(OP,DESC,FLAGS | I_RETURN)
#define OPCODE0_TAILRETURN(OP, DESC, FLAGS) OPCODE0_RETURN(OP,DESC,FLAGS)
#define OPCODE1_TAILRETURN(OP, DESC, FLAGS) OPCODE1_RETURN(OP,DESC,FLAGS)
#define OPCODE2_TAILRETURN(OP, DESC, FLAGS) OPCODE2_RETURN(OP,DESC,FLAGS)

#define OPCODE0_ALIAS(OP, DESC, FLAGS, A) { DESC, FLAGS ALIASADDR(A) },
#define OPCODE1_ALIAS(OP, DESC, FLAGS, A) { DESC, FLAGS | I_HASARG ALIASADDR(A) },
#define OPCODE2_ALIAS(OP, DESC, FLAGS, A) { DESC, FLAGS | I_TWO_ARGS ALIASADDR(A) },

#define OPCODE0_BRANCH(OP,DESC,FLAGS) { DESC, FLAGS | I_ISBRANCH BRANCHADDR(OP) },
#define OPCODE1_BRANCH(OP,DESC,FLAGS) { DESC, FLAGS | I_ISBRANCHARG BRANCHADDR(OP) },
#define OPCODE2_BRANCH(OP,DESC,FLAGS) { DESC, FLAGS | I_ISBRANCHARGS BRANCHADDR(OP) },
#define OPCODE0_TAILBRANCH(OP,DESC,FLAGS) OPCODE0_BRANCH(OP,DESC,FLAGS)
#define OPCODE1_TAILBRANCH(OP,DESC,FLAGS) OPCODE1_BRANCH(OP,DESC,FLAGS)
#define OPCODE2_TAILBRANCH(OP,DESC,FLAGS) OPCODE2_BRANCH(OP,DESC,FLAGS)

#define OPCODE_NOCODE(DESC, OP, FLAGS)   { DESC, FLAGS NULLADDR },

const struct instr instrs[] = {
  { "offset", 0 NULLADDR },
#include "opcode_list.h"
};

#ifdef PIKE_DEBUG
unsigned long pike_instrs_compiles[F_MAX_OPCODE-F_OFFSET];

const char *low_get_f_name(int n, struct program *p)
{
  static char buf[30];

  if (n<F_MAX_OPCODE)
  {
    if ((n >= F_OFFSET) && instrs[n-F_OFFSET].name)
      return instrs[n-F_OFFSET].name;
    sprintf(buf, "<OTHER %d>", n);
    return buf;
  }

  if(p &&
     (int)p->num_constants > (int)(n-F_MAX_OPCODE) &&
     (TYPEOF(p->constants[n-F_MAX_OPCODE].sval) == T_FUNCTION) &&
     (SUBTYPEOF(p->constants[n-F_MAX_OPCODE].sval) == FUNCTION_BUILTIN) &&
     p->constants[n-F_MAX_OPCODE].sval.u.efun) {
    return p->constants[n-F_MAX_OPCODE].sval.u.efun->name->str;
  }

  sprintf(buf, "Call efun %d", n - F_MAX_OPCODE);
  return buf;
}

const char *get_f_name(int n)
{
  if (Pike_fp && Pike_fp->context)
    return low_get_f_name(n, Pike_fp->context->prog);
  return low_get_f_name(n, NULL);
}

#endif /* PIKE_DEBUG */

const char *get_token_name(int n)
{
  static char buf[30];
  if ((n<F_MAX_INSTR) && (n >= F_OFFSET) && instrs[n-F_OFFSET].name)
  {
    return instrs[n-F_OFFSET].name;
  } else if ((n >= ' ') && (n <= 0x7f)) {
    sprintf(buf, "<OTHER '%c'>", n);
  }else{
    sprintf(buf, "<OTHER %d>", n);
  }
  return buf;
}

void init_opcodes(void)
{
#ifdef PIKE_DEBUG
#ifdef INSTR_PROFILING
  instr_counter_storage=init_instr_storage_pointers(p_flag);
#endif
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
	      (long)pike_instrs_compiles[e],
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
