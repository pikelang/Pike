#include "global.h"
#include "svalue.h"
#include "array.h"
#include "stralloc.h"
#include "interpret.h"
#include "module_support.h"
#include "threads.h"
#include "mapping.h"
#include "perl_machine.h"

#ifdef HAVE_PERL

#include <EXTERN.h>
#include <perl.h>


static int num_perl_interpreters=0;
DEFINE_MUTEX(perl_running);

#ifdef MULTIPLICITY
#endif

struct perlmod_storage
{
  char **argv;
  char **env;
  char *env_block;
  struct array *argv_strings;
  int parsed;
  PerlInterpreter *my_perl;
};

#define THIS ((struct perlmod_storage *)(fp->current_storage))
#define PERL THIS->my_perl

static void init_perl_glue(struct object *o)
{
  PerlInterpreter *p;
  THIS->argv=0;
  THIS->env=0;
  THIS->env_block=0;
  THIS->argv_strings=0;
  THIS->parsed=0;

#ifndef MULTIPLICITY
  if(num_perl_interpreters>0)
  {
    PERL=0;
    fprintf(stderr,"num_perl_interpreters=%d\n",num_perl_interpreters);
/*    error("Perl: There can be only one!\n"); */
    return;
  }
#endif
  THREADS_ALLOW();
  mt_lock(&perl_running);
  p=perl_alloc();
  mt_unlock(&perl_running);
  THREADS_DISALLOW();
  PERL=p;
  if(p) num_perl_interpreters++;
}

static void exit_perl_glue(struct object *o)
{
  if(PERL)
  {
    struct perlmod_storage *storage=THIS;

    THREADS_ALLOW();
    mt_lock(&perl_running);
    if(storage->parsed)
    {
      perl_destruct(storage->my_perl);
      storage->parsed=0;
    }
    perl_free(storage->my_perl);
    mt_unlock(&perl_running);
    THREADS_DISALLOW();
    num_perl_interpreters--;
  }
  if(THIS->argv)
  {
    free((char *)THIS->argv);
    THIS->argv=0;
  }
  if(THIS->argv_strings)
  {
    free_array(THIS->argv_strings);
    THIS->argv_strings=0;
  }
  if(THIS->env)
  {
    free((char *)THIS->env);
    THIS->env=0;
  }
  if(THIS->env_block)
  {
    free((char *)THIS->env_block);
    THIS->env_block=0;
  }
}

static void perlmod_run(INT32 args)
{
  INT32 i;
  PerlInterpreter *p=PERL;
  if(!p) error("No perl interpreter available.\n");
  pop_n_elems(args);

  if(!THIS->argv_strings)
    error("Perl->create() must be called first.\n");

  THREADS_ALLOW();
  mt_lock(&perl_running);
  i=perl_run(p);
  mt_unlock(&perl_running);
  THREADS_DISALLOW();
  push_int(i);
}

static void perlmod_create(INT32 args)
{
  extern void xs_init(void);
  int e;
  struct mapping *env_mapping=0;
  PerlInterpreter *p=PERL;
  struct perlmod_storage *storage=THIS;
    
  check_all_args("Perl->create",args,BIT_ARRAY, BIT_MAPPING|BIT_VOID, 0);
  if(!p) error("No perl interpreter available.\n");

  if(THIS->argv_strings)
    error("Perl->create() can only be called once.\n");

  switch(args)
  {
    default:
      env_mapping=sp[1-args].u.mapping;
      mapping_fix_type_field(env_mapping);

      if(m_ind_types(env_mapping) & ~BIT_STRING)
	error("Bad argument 2 to Perl->create().\n");
      if(m_val_types(env_mapping) & ~BIT_STRING)
	error("Bad argument 2 to Perl->create().\n");
      
    case 1:
      THIS->argv_strings=sp[-args].u.array;
      add_ref(THIS->argv_strings);
      array_fix_type_field(THIS->argv_strings);

      if(THIS->argv_strings->size<2)
	error("Perl: Too few elements in argv array.\n");

      if(THIS->argv_strings->type_field & ~BIT_STRING)
	error("Bad argument 1 to Perl->create().\n");
  }

  THIS->argv=(char **)xalloc(sizeof(char *)*THIS->argv_strings->size);
  for(e=0;e<THIS->argv_strings->size;e++)
    THIS->argv[e]=ITEM(THIS->argv_strings)[e].u.string->str;

  if(env_mapping)
  {
    INT32 d;
    int env_block_size=0;
    char *env_blockp;
    struct keypair *k;
    MAPPING_LOOP(env_mapping)
      env_block_size+=k->ind.u.string->len+k->val.u.string->len+2;

    THIS->env_block=xalloc(env_block_size);
    THIS->env=(char **)xalloc(m_sizeof(env_mapping)+1);

    env_blockp=THIS->env_block;
    d=0;
    MAPPING_LOOP(env_mapping)
      {
	THIS->env[d++]=env_blockp;
	MEMCPY(env_blockp,k->ind.u.string->str,k->ind.u.string->len);
	env_blockp+=k->ind.u.string->len;

	*(env_blockp++)='=';

	MEMCPY(env_blockp,k->val.u.string->str,k->ind.u.string->len);
	env_blockp+=k->val.u.string->len;

	*(env_blockp++)='0';
      }
    THIS->env[d]=0;
  }
  
  THREADS_ALLOW();
  mt_lock(&perl_running);
  if(!storage->parsed)
  {
    perl_construct(p);
    storage->parsed++;
  }
  e=perl_parse(p,
	       xs_init,
	       storage->argv_strings->size,
	       storage->argv,
	       storage->env);
  mt_unlock(&perl_running);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(e);
}

static void perlmod_eval(INT32 args)
{
  error("Perl->eval not yet implemented.\n");
}

static void perlmod_call(INT32 args)
{
  error("Perl->call not yet implemented.\n");
}

void pike_module_init(void)
{
  perl_destruct_level=2;
  start_new_program();
  ADD_STORAGE(struct perlmod_storage);
  /* function(array(string),void|mapping(string:string):int) */
  ADD_FUNCTION("create",perlmod_create,tFunc(tArr(tStr) tOr(tVoid,tMap(tStr,tStr)),tInt),0);
  /* function(:int) */
  ADD_FUNCTION("run",perlmod_run,tFunc(,tInt),0);
  /* function(string:int) */
  ADD_FUNCTION("eval",perlmod_eval,tFunc(tStr,tInt),0);
  /* function(string,mixed...:int) */
  ADD_FUNCTION("call",perlmod_call,tFuncV(tStr,tMix,tInt),0);
  set_init_callback(init_perl_glue);
  set_exit_callback(exit_perl_glue);
  end_class("Perl",0);

  add_integer_constant("MULTIPLICITY",
#ifdef MULTIPLICITY
		       1,
#else
		       0,
#endif
		       0);
}

void pike_module_exit(void)
{
}

#else /* HAVE_PERL */
void pike_module_init(void) {}
void pike_module_exit(void) {}
#endif
