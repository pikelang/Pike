#include <global.h>
#include <svalue.h>
#include <array.h>
#include <stralloc.h>
#include <interpret.h>
#include <module_support.h>
#include <threads.h>
#include <mapping.h>

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
  if(p) perl_construct(p);
  mt_unlock(&perl_running);
  THREADS_DISALLOW();
  PERL=p;
  if(p) num_perl_interpreters++;
}

static void exit_perl_glue(struct object *o)
{
  if(PERL)
  {
    PerlInterpreter *p=PERL;
    THREADS_ALLOW();
    mt_lock(&perl_running);
    perl_destruct(p);
    perl_free(p);
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

  THREADS_ALLOW();
  mt_lock(&perl_running);
  i=perl_run(p);
  mt_unlock(&perl_running);
  THREADS_DISALLOW();
  push_int(i);
}

static void perlmod_parse(INT32 args)
{
  extern void xs_init(void);
  int e;
  struct mapping *env_mapping=0;
  PerlInterpreter *p=PERL;
  struct perlmod_storage *storage=THIS;
    
  check_all_args("Perl->parse",args,BIT_ARRAY, BIT_MAPPING|BIT_VOID, 0);
  if(!p) error("No perl interpreter available.\n");

  if(THIS->argv_strings)
    error("Perl->parse() can only be called once.\n");

  switch(args)
  {
    default:
      env_mapping=sp[1-args].u.mapping;
      mapping_fix_type_field(env_mapping);

      if(m_ind_types(env_mapping) & ~BIT_STRING)
	error("Bad argument 2 to Perl->parse().\n");
      if(m_val_types(env_mapping) & ~BIT_STRING)
	error("Bad argument 2 to Perl->parse().\n");
      
    case 1:
      THIS->argv_strings=sp[-args].u.array;
      add_ref(THIS->argv_strings);
      array_fix_type_field(THIS->argv_strings);

      if(THIS->argv_strings->type_field & ~BIT_STRING)
	error("Bad argument 2 to Perl->parse().\n");
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
  perl_destruct_level=1;
  start_new_program();
  add_storage(sizeof(struct perlmod_storage));
  add_function("parse",perlmod_parse,"function(array(string),void|mapping(string:string):int)",0);
  add_function("run",perlmod_run,"function(:int)",0);
  add_function("eval",perlmod_eval,"function(string:int)",0);
  add_function("call",perlmod_call,"function(string,mixed...:int)",0);
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
