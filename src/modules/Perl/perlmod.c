/* $Id: perlmod.c,v 1.12 2000/03/14 21:33:24 leif Exp $ */

#include "builtin_functions.h"
#include "global.h"
#include "svalue.h"
#include "array.h"
#include "stralloc.h"
#include "interpret.h"
#include "module_support.h"
#include "threads.h"
#include "mapping.h"
#include "perl_machine.h"

  /* this is just for debugging */
#define _sv_2mortal(x) (x)

#ifdef HAVE_PERL

#include <EXTERN.h>
#include <perl.h>

/* Do not redefine my malloc macro you stupid Perl! */
#include "dmalloc.h"

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
  int constructed, parsed;
  int array_size_limit;
  PerlInterpreter *my_perl;
};

#define THIS ((struct perlmod_storage *)(fp->current_storage))
#define PERL THIS->my_perl

/* since both Perl and Pike likes to use "sp" as a stack pointer,
 * let's define some Pike macros as functions...
 */
static void _push_int(INT32 i) { push_int(i);}
static void _push_float(float f) { push_float(f);}
static void _push_string(struct pike_string *s) { push_string(s);}
static void _push_array(struct array *a) { push_array(a);}
static void _pop_n_elems(int n) { pop_n_elems(n);}
static struct svalue *_pikesp() { return Pike_sp;}
static void _pike_pop() { --sp;}
#undef sp

#ifndef BLOCKING

#define MT_PERMIT THREADS_ALLOW(); mt_lock(&perl_running);
#define MT_FORBID mt_unlock(&perl_running); THREADS_DISALLOW();

#endif

/* utility function: push a zero_type zero */
static void _push_zerotype()
{ push_int(0);
  Pike_sp[-1].subtype = 1;
}

static SV * _pikev2sv(struct svalue *s)
{ switch (s->type)
  { case T_INT:
      return newSViv(s->u.integer); break;
    case T_FLOAT:
      return newSVnv(s->u.float_number); break;
    case T_STRING:
      if (s->u.string->size_shift) break;
      return newSVpv(s->u.string->str, s->u.string->len); break;
  }
  error("Unsupported value type.\n");
  return 0;
}

static void _sv_to_svalue(SV *sv, struct svalue *sval)
{ if (sv && (SvOK(sv)))
  { if (SvIOKp(sv))
    { sval->type = T_INT; sval->subtype = 0;
      sval->u.integer = SvIV(sv);
      return;
    }
    else if (SvNOKp(sv))
    { sval->type = T_FLOAT; sval->subtype = 0;
      sval->u.float_number = SvNV(sv);
      return;
    }
    else if (SvPOKp(sv))
    { sval->type = T_STRING; sval->subtype = 0;
      sval->u.string = make_shared_binary_string(SvPVX(sv), SvCUR(sv));
      return;
    }
  }
  sval->type = T_INT; sval->u.integer = 0;
  sval->subtype = !sv; /* zero-type zero if NULL pointer */
}

static void _pikepush_sv(SV *sv)
{ if (!SvOK(sv))
     _push_int(0);
  else if (SvIOKp(sv))
     _push_int(SvIV(sv));
  else if (SvNOKp(sv))
     _push_float((float)(SvNV(sv)));
  else if (SvPOKp(sv))
     _push_string(make_shared_binary_string(SvPVX(sv), SvCUR(sv)));
  else
     _push_int(0);
}


static void init_perl_glue(struct object *o)
{ PerlInterpreter *p;

#ifdef PIKE_PERLDEBUG
  fprintf(stderr, "[init_perl_glue]\n");
#endif

  THIS->argv             = 0;
  THIS->env              = 0;
  THIS->env_block        = 0;
  THIS->argv_strings     = 0;
  THIS->constructed      = 0;
  THIS->parsed           = 0;
  THIS->array_size_limit = 500;

#ifndef MULTIPLICITY
  if(num_perl_interpreters>0)
  {
    PERL=0;
    fprintf(stderr,"num_perl_interpreters=%d\n",num_perl_interpreters);
    /*    error("Perl: There can be only one!\n"); */
    return;
  }
#endif
  MT_PERMIT;
  p=perl_alloc();
  MT_FORBID;
  PERL=p;
  if(p) num_perl_interpreters++;
}

static void _free_arg_and_env()
{ if(THIS->argv)
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

static void exit_perl_glue(struct object *o)
{
#ifdef PIKE_PERLDEBUG
  fprintf(stderr, "[exit_perl_glue]\n");
#endif

  if(PERL)
  {
    struct perlmod_storage *storage=THIS;

    MT_PERMIT;
    if(storage->constructed)
    {
      perl_destruct(storage->my_perl);
      storage->constructed=0;
    }
    perl_free(storage->my_perl);
    MT_FORBID;
    num_perl_interpreters--;
  }
  _free_arg_and_env();
}

static void perlmod_create(INT32 args)
{ PerlInterpreter *p=PERL;
  struct perlmod_storage *storage=THIS;

#ifdef PIKE_PERLDEBUG
  fprintf(stderr, "[perlmod_create, %d args]\n", args);
#endif
    
  if (args != 0) error("Perl->create takes no arguments.");
  if(!p) error("No perl interpreter available.\n");

  MT_PERMIT;
  if(!storage->constructed)
  { perl_construct(p);
    storage->constructed++;
  }
  MT_FORBID;
  pop_n_elems(args);
  push_int(0);
}

static void perlmod_parse(INT32 args)
{
  extern void xs_init(void);
  int e;
  struct mapping *env_mapping=0;
  PerlInterpreter *p=PERL;
  struct perlmod_storage *storage=THIS;

#ifdef PIKE_PERLDEBUG
  fprintf(stderr, "[perlmod_parse, %d args]\n", args);
#endif
    
  check_all_args("Perl->parse",args,BIT_ARRAY, BIT_MAPPING|BIT_VOID, 0);
  if(!p) error("No perl interpreter available.\n");

  switch(args)
  {
    default:
      env_mapping = Pike_sp[1-args].u.mapping;
      mapping_fix_type_field(env_mapping);

      if(m_ind_types(env_mapping) & ~BIT_STRING)
	error("Bad argument 2 to Perl->create().\n");
      if(m_val_types(env_mapping) & ~BIT_STRING)
	error("Bad argument 2 to Perl->create().\n");
      
    case 1:
      if (THIS->argv_strings || THIS->env_block)
      { /* if we have already setup args/env, free the old values now */
        _free_arg_and_env();
      }

      THIS->argv_strings = Pike_sp[-args].u.array;
      add_ref(THIS->argv_strings);
      array_fix_type_field(THIS->argv_strings);

      if(THIS->argv_strings->size<2)
	   error("Perl: Too few elements in argv array.\n");

      if(THIS->argv_strings->type_field & ~BIT_STRING)
	   error("Bad argument 1 to Perl->parse().\n");
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
    THIS->env=(char **)xalloc(sizeof(char *)*(m_sizeof(env_mapping)+1));

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

	*(env_blockp++)=0;
      }
    THIS->env[d]=0;
  }
  else
  {
    /* Perl likes to be able to write in the environment block,
     * give it it's own copy to protect ourselves..  /Hubbe
     */
    INT32 d;
    int env_block_size=0;
    char *env_blockp;

#ifdef DECLARE_ENVIRON
    extern char **environ;
#endif

    for(d=0;environ[d];d++)
      env_block_size+=strlen(environ[d])+1;

    THIS->env_block=xalloc(env_block_size);
    THIS->env=(char **)xalloc(sizeof(char *)*(d+1));

    env_blockp=THIS->env_block;

    for(d=0;environ[d];d++)
    {
      int l=strlen(environ[d]);
      THIS->env[d]=env_blockp;
      MEMCPY(env_blockp,environ[d],l+1);
      env_blockp+=l+1;
    }

#ifdef PIKE_DEBUG
    if(env_blockp - THIS->env_block > env_block_size)
      fatal("Arglebargle glop-glyf.\n");
#endif

    THIS->env[d]=0;
  }
  

  THIS->parsed++;

  MT_PERMIT;
  e=perl_parse(p,
	       xs_init,
	       storage->argv_strings->size,
	       storage->argv,
	       storage->env);
  MT_FORBID;
  pop_n_elems(args);
  push_int(e);
}

static void perlmod_run(INT32 args)
{
  INT32 i;
  PerlInterpreter *p=PERL;
  if(!p) error("No perl interpreter available.\n");
  pop_n_elems(args);

  if(!THIS->constructed || !THIS->parsed)
    error("No Perl program loaded (run() called before parse()).\n");

  MT_PERMIT;
  i=perl_run(p);
  MT_FORBID;

  push_int(i);
}

static void _perlmod_eval(INT32 args, int perlflags)
{ PerlInterpreter *p = PERL;
  struct pike_string *arg1;
  struct perlmod_storage *storage = THIS;
  int i, n;
#define sp _perlsp
  dSP;

  if (!p) error("Perl interpreter not available.\n");

  check_all_args("Perl->eval", args, BIT_STRING, 0);
  arg1 = _pikesp()[-args].u.string;

  ENTER;
  SAVETMPS;
  PUSHMARK(sp);

  PUTBACK;
#undef sp
  MT_PERMIT;

  if (!storage->parsed)
  { static char *dummyargv[] = { "perl", "-e", "1", 0 };
    extern void xs_init(void);
    perl_parse(p, xs_init, 3, dummyargv, NULL);
    storage->parsed++;
  }

  n = perl_eval_sv(newSVpv(arg1->str, arg1->len), perlflags | G_EVAL);

  MT_FORBID;

  _pop_n_elems(args);

#define sp _perlsp
  SPAGAIN;

  if (SvTRUE(GvSV(errgv)))
  { char errtmp[256];
    memset(errtmp, 0, sizeof(errtmp));
    strcpy(errtmp, "Error from Perl: ");
    strncpy(errtmp+strlen(errtmp), SvPV(GvSV(errgv), na), 254-strlen(errtmp));
    POPs;
    PUTBACK; FREETMPS; LEAVE;
    error(errtmp);
  }

  if (perlflags & G_ARRAY)
  { struct array *a = allocate_array(n);
    for(i = 0; i < n; ++i)
         _sv_to_svalue(POPs, &(a->item[(n-1)-i]));
    _push_array(a);
  }
  else if (n > 0)
  { for(; n > 1; --n) POPs;
    _pikepush_sv(POPs);
  }
  else _push_zerotype();

  PUTBACK; FREETMPS; LEAVE;
#undef sp
}

static void perlmod_eval(INT32 args)
  { return _perlmod_eval(args, G_SCALAR);}

static void perlmod_eval_list(INT32 args)
  { return _perlmod_eval(args, G_ARRAY);}

static void _perlmod_call(INT32 args, int perlflags)
{ PerlInterpreter *p = PERL;
  int i, n; char *pv;
#define sp _perlsp
  dSP;

#ifdef PIKE_PERLDEBUG
  fprintf(stderr, "[perlmod_call: args=%d]\n", args);
#endif

  if (!p) error("No perl interpreter available.\n");

  if (args <   1) error("Too few arguments.\n");
  if (args > 201) error("Too many arguments.\n");

  if (_pikesp()[-args].type != T_STRING ||
      _pikesp()[-args].u.string->size_shift)
       error("bad Perl function name (must be an 8-bit string)");

  ENTER;
  SAVETMPS;
  PUSHMARK(sp);

  for(n = 1; n < args; ++n)
  { struct svalue *s = &(_pikesp()[n-args]);
    char *msg;
    switch (s->type)
    { case T_INT:
        XPUSHs(sv_2mortal(newSViv(s->u.integer)));
        break;
      case T_FLOAT:
        XPUSHs(sv_2mortal(newSVnv((double)(s->u.float_number))));
        break;
      case T_STRING:
        if (s->u.string->size_shift)
        { PUTBACK; FREETMPS; LEAVE;
          error("widestrings not supported in Pike-to-Perl call interface");
          return;
        }
        XPUSHs(sv_2mortal(newSVpv(s->u.string->str, s->u.string->len)));
        break;
      case T_MAPPING:
        msg = "Mapping argument not allowed here.\n"; if (0)
      case T_OBJECT:
        msg = "Object argument not allowed here.\n"; if (0)
      case T_MULTISET:
        msg = "Multiset argument not allowed here.\n"; if (0)
      case T_ARRAY:
        msg = "Array argument not allowed here.\n"; if (0)
      default:
        msg = "Unsupported argument type.\n";
        PUTBACK; FREETMPS; LEAVE;
        error(msg);
        return;
    }
  }
  PUTBACK;

  pv = Pike_sp[-args].u.string->str;  
#undef sp
  MT_PERMIT;

  n = perl_call_pv(pv, perlflags);

  MT_FORBID;
#define sp _perlsp

  _pop_n_elems(args);

  SPAGAIN;

  if (SvTRUE(GvSV(errgv)))
  { char errtmp[256];
    memset(errtmp, 0, sizeof(errtmp));
    strcpy(errtmp, "Error from Perl: ");
    strncpy(errtmp+strlen(errtmp), SvPV(GvSV(errgv), na), 254-strlen(errtmp));
    POPs;
    PUTBACK; FREETMPS; LEAVE;
    error(errtmp);
  }

  if (n < 0)
  { PUTBACK; FREETMPS; LEAVE;
    error("Internal error: perl_call_pv returned a negative number.\n");
  }

  if (!(perlflags & G_ARRAY) && n > 1)
       while (n > 1) --n, POPs;

  if (n > THIS->array_size_limit)
  { PUTBACK; FREETMPS; LEAVE;
    error("Perl function returned too many values.\n");
  }

  if (perlflags & G_ARRAY)
  { struct array *a = allocate_array(n);
    for(i = 0; i < n; ++i)
         _sv_to_svalue(POPs, &(a->item[(n-1)-i]));
    _push_array(a);
  }
  else if (n == 1)
     _pikepush_sv(POPs);
  else /* shouldn't happen unless we put G_DISCARD in perlflags */
     _push_zerotype();

  PUTBACK; FREETMPS; LEAVE;
#undef sp
}

static void perlmod_call_list(INT32 args)
{ _perlmod_call(args, G_ARRAY | G_EVAL);
}

static void perlmod_call(INT32 args)
{ _perlmod_call(args, G_SCALAR | G_EVAL);
}

static void _perlmod_varop(INT32 args, int op, int type)
{ int i, wanted_args;

  wanted_args = type == 'S' ? 1 : 2;
  if (op == 'W') ++wanted_args;

  if (!(PERL)) error("No Perl interpreter available.\n");

  if (args != wanted_args) error("Wrong number of arguments.\n");
  if (Pike_sp[-args].type != T_STRING ||
      Pike_sp[-args].u.string->size_shift != 0)
       error("Variable name must be an 8-bit string.\n");

  if (type == 'S') /* scalar */
  { SV *sv = perl_get_sv(Pike_sp[-args].u.string->str, TRUE | GV_ADDMULTI);
    if (op == 'W')
      { sv_setsv(sv, sv_2mortal(_pikev2sv(Pike_sp-1)));}
    pop_n_elems(args);
    if (op == 'R') _pikepush_sv(sv);
  }
  else if (type == 'A') /* array */
  { AV *av = perl_get_av(Pike_sp[-args].u.string->str, TRUE | GV_ADDMULTI);
    SV **svp;
    if (Pike_sp[1-args].type != T_INT || (i = Pike_sp[1-args].u.integer) < 0)
          error("Array subscript must be a non-negative integer.\n");
    if (op == 'W')
         av_store(av, i, _sv_2mortal(_pikev2sv(Pike_sp+2-args)));
    pop_n_elems(args);
    if (op == 'R')
    { if ((svp = av_fetch(av, i, 0))) _pikepush_sv(*svp);
                                 else _push_zerotype();
    }
  }
  else if (type == 'H') /* hash */
  { HV *hv = perl_get_hv(Pike_sp[-args].u.string->str, TRUE | GV_ADDMULTI);
    SV *key = sv_2mortal(_pikev2sv(Pike_sp+1-args));
    HE *he;
    if (op == 'W')
    { if ((he = hv_store_ent
                   (hv, key, _sv_2mortal(_pikev2sv(Pike_sp+2-args)), 0)))
         sv_setsv(HeVAL(he), _sv_2mortal(_pikev2sv(Pike_sp+2-args)));
      else
         error("Internal error: hv_store_ent returned NULL.\n");
    }
    pop_n_elems(args);
    if (op == 'R')
    { if ((he = hv_fetch_ent(hv, key, 0, 0)))
         _pikepush_sv(HeVAL(he));
      else
         _push_zerotype();
    }
  }
  else error("Internal error in _perlmod_varop.\n");

  if (op != 'R') push_int(0);
}

static void perlmod_get_scalar(INT32 args)
   { _perlmod_varop(args, 'R', 'S');}
static void perlmod_set_scalar(INT32 args)
   { _perlmod_varop(args, 'W', 'S');}
static void perlmod_get_array_item(INT32 args)
   { _perlmod_varop(args, 'R', 'A');}
static void perlmod_set_array_item(INT32 args)
   { _perlmod_varop(args, 'W', 'A');}
static void perlmod_get_hash_item(INT32 args)
   { _perlmod_varop(args, 'R', 'H');}
static void perlmod_set_hash_item(INT32 args)
   { _perlmod_varop(args, 'W', 'H');}

static void perlmod_array_size(INT32 args)
{ AV *av;
  if (args != 1) error("Wrong number of arguments.\n");
  if (Pike_sp[-args].type != T_STRING ||
      Pike_sp[-args].u.string->size_shift != 0)
      error("Array name must be given as an 8-bit string.\n");

  av = perl_get_av(Pike_sp[-args].u.string->str, TRUE | GV_ADDMULTI);
  if (!av) error("Interal error: perl_get_av() return NULL.\n");
  pop_n_elems(args);
  /* Return av_len()+1, since av_len() returns the value of the highest
   * index, which is 1 less than the size. */
  _push_int(av_len(av)+1);
}

static void perlmod_get_whole_array(INT32 args)
{ AV *av; int i, n; struct array *arr;
  if (args != 1) error("Wrong number of arguments.\n");
  if (Pike_sp[-args].type != T_STRING ||
      Pike_sp[-args].u.string->size_shift != 0)
      error("Array name must be given as an 8-bit string.\n");

  av = perl_get_av(Pike_sp[-args].u.string->str, TRUE | GV_ADDMULTI);
  if (!av) error("Interal error: perl_get_av() returned NULL.\n");
  n = av_len(av) + 1;

  if (n > THIS->array_size_limit)
     error("The array is larger than array_size_limit.\n");

  arr = allocate_array(n);
  for(i = 0; i < n; ++i)
  { SV **svp = av_fetch(av, i, 0);
    _sv_to_svalue(svp ? *svp : NULL, &(arr->item[i]));
  }
  pop_n_elems(args);
  push_array(arr);
}

static void perlmod_get_hash_keys(INT32 args)
{ HV *hv; HE *he; SV *sv; int i, n; I32 len; struct array *arr;
  if (args != 1) error("Wrong number of arguments.\n");
  if (Pike_sp[-args].type != T_STRING ||
      Pike_sp[-args].u.string->size_shift != 0)
      error("Hash name must be given as an 8-bit string.\n");

  hv = perl_get_hv(Pike_sp[-args].u.string->str, TRUE | GV_ADDMULTI);
  if (!hv) error("Interal error: perl_get_av() return NULL.\n");

  /* count number of elements in hash */
  for(n = 0, hv_iterinit(hv); (he = hv_iternext(hv)); ++n);

  if (n > THIS->array_size_limit)
     error("The array is larger than array_size_limit.\n");

  arr = allocate_array(n);
  for(i = 0, hv_iterinit(hv); (he = hv_iternext(hv)); ++i)
       _sv_to_svalue(hv_iterkey(he, &len),
                     &(arr->item[i]));

  pop_n_elems(args);
  push_array(arr);
}

static void perlmod_array_size_limit(INT32 args)
{ int i;
  switch (args)
  { case 0:
      break;
    case 1:
      if (Pike_sp[-args].type != T_INT || Pike_sp[-args].u.integer < 1)
           error("Argument must be a integer in range 1 to 2147483647.");
      THIS->array_size_limit = Pike_sp[-args].u.integer;
      break;
    default:
      error("Wrong number of arguments.\n");
  }
  pop_n_elems(args);
  _push_int(THIS->array_size_limit);
}

void pike_module_init(void)
{
#ifdef PIKE_PERLDEBUG
  fprintf(stderr, "[perl: module init]\n");
#endif

  perl_destruct_level=1;

  start_new_program();
  ADD_STORAGE(struct perlmod_storage);
  /* function(void:int) */
  ADD_FUNCTION("create",perlmod_create,tFunc(tVoid,tInt),0);
  /* function(array(string),void|mapping(string:string):int) */
  ADD_FUNCTION("parse",perlmod_parse,tFunc(tArr(tStr) tOr(tVoid,tMap(tStr,tStr)),tInt),0);
  /* function(:int) */
  ADD_FUNCTION("run",perlmod_run,tFunc(tNone,tInt),0);

  /* function(string,mixed...:mixed) */
  ADD_FUNCTION("call",perlmod_call,tFuncV(tStr,tMix,tMix),0);

  /* function(string,mixed...:mixed) */
  ADD_FUNCTION("call_list",perlmod_call_list,tFuncV(tStr,tMix,tMix),0);

  /* function(string:mixed) */
  ADD_FUNCTION("eval",perlmod_eval,tFunc(tStr,tMix),0);

  /* function(string:array) */
  ADD_FUNCTION("eval_list",perlmod_eval_list,tFunc(tStr,tArr(tMix)),0);

  /* function(string:mixed) */
  ADD_FUNCTION("get_scalar",perlmod_get_scalar,tFunc(tStr,tMix),0);

  /* function(string,mixed:mixed) */
  ADD_FUNCTION("set_scalar",perlmod_set_scalar,tFunc(tStr tMix,tMix),0);

  /* function(string,int:mixed) */
  ADD_FUNCTION("get_array_item",perlmod_get_array_item,
               tFunc(tStr tInt,tMix),0);

  /* function(string,int,mixed:mixed) */
  ADD_FUNCTION("set_array_item",perlmod_set_array_item,
               tFunc(tStr tInt tMix,tMix),0);

  /* function(string,mixed:mixed) */
  ADD_FUNCTION("get_hash_item",perlmod_get_hash_item,
               tFunc(tStr tMix,tMix),0);

  /* function(string,mixed,mixed:mixed) */
  ADD_FUNCTION("set_hash_item",perlmod_set_hash_item,
               tFunc(tStr tMix tMix,tMix),0);

  /* function(string:int) */
  ADD_FUNCTION("array_size",perlmod_array_size,
               tFunc(tStr,tInt),0);

  /* function(string:int) */
  ADD_FUNCTION("get_array",perlmod_get_whole_array,
               tFunc(tStr,tArr(tMix)),0);

  /* function(string:int) */
  ADD_FUNCTION("get_hash_keys",perlmod_get_whole_array,
               tFunc(tStr,tArr(tMix)),0);

#if 0
  /* function(string,array:array) */
  ADD_FUNCTION("set_array", perlmod_set_whole_array,
               tFunc(tStr tArr(tMix),tArr(tMix)),0);
#endif

  /* function(void|int:int) */
  ADD_FUNCTION("array_size_limit",perlmod_array_size_limit,
        tFunc(tOr(tVoid,tInt),tInt),0);

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
