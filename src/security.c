#include "global.h"

/* To do:
 * controls for file->pipe()
 * controls for all/most functions in spider
 * controls for threads
 */

#ifdef PIKE_SECURITY

#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "object.h"
#include "mapping.h"
#include "multiset.h"
#include "gc.h"
#include "security.h"
#include "module_support.h"
#include "constants.h"

static struct program *creds_program;

struct object *current_creds=0;
#undef THIS
#define THIS ((struct pike_creds *)(fp->current_storage))

static int valid_creds_object(struct object *o)
{
  return o &&
    get_storage(o, creds_program) == o->storage &&
    OBJ2CREDS(o)->user;
}

static void f_call_with_creds(INT32 args)
{
  struct object *o;

  switch(sp[-args].type)
  {
    case T_INT:
      /* We might want allocate a bit for this so that we can
       * disallow this
       */
      o=fp->current_object->prot;
      break;

    case T_OBJECT:
      o=sp[-args].u.object;
      if(!CHECK_SECURITY(SECURITY_BIT_SECURITY) &&
	 !(fp->current_object->prot && 
	   (OBJ2CREDS(fp->current_object->prot)->may_always & SECURITY_BIT_SECURITY)))
	error("call_with_creds: permission denied.\n");
      
      break;

    default:
      error("Bad argument 1 to call_with_creds.\n");
  }
    
  if(!valid_creds_object(o))
    error("call_with_creds: Not a valid creds object.\n");
  SET_CURRENT_CREDS(o);

  f_call_function(args-1);
  free_svalue(sp-2);
  sp[-2]=sp[-1];
  sp--;
}

static void f_get_current_creds(INT32 args)
{
  pop_n_elems(args);
  if(current_creds)
    ref_push_object(current_creds);
  else
    push_int(0);
}

/* Should be no need for special security for these. obj->creds
 * should say what we can do with it.
 */
static void get_default_creds(INT32 args)
{
  pop_n_elems(args);
  if(THIS->default_creds && THIS->default_creds->prog)
    ref_push_object(THIS->default_creds);
  else
    push_int(0);
}

static void set_default_creds(INT32 args)
{
  struct object *o;
  INT_TYPE may,data;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("set_default_creds: permission denied.\n"));

  get_all_args("init_creds",args,"%o",&o);
  
  if(THIS->default_creds) free_object(THIS->default_creds);
  add_ref(THIS->default_creds=o);
  pop_n_elems(args);
}

static void creds_create(INT32 args)
{
  struct object *o;
  INT_TYPE may,data;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("creds_create: permission denied.\n"));

  get_all_args("init_creds",args,"%o%i%i",&o,&may,&data);
  if(THIS->user)
    error("You may only call creds_create once.\n");
  
  add_ref(THIS->user=o);
  THIS->may_always=may;
  THIS->data_bits=data;
  pop_n_elems(args);
}

static void creds_get_user(INT32 args)
{
  pop_n_elems(args);
  if(THIS->user)
    ref_push_object(THIS->user);
  else
    push_int(0);
}

static void creds_get_allow_bits(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->may_always);
}

static void creds_get_data_bits(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->data_bits);
}

static void creds_apply(INT32 args)
{
  if(args < 0 || sp[-args].type > MAX_COMPLEX)
    error("Bad argument 1 to creds->apply()\n");

  if( CHECK_SECURITY(SECURITY_BIT_SECURITY) ||
      (sp[-args].u.array->prot && 
       OBJ2CREDS(sp[-args].u.array->prot)->user == THIS->user))
  {
    if(sp[-args].u.array->prot)
      free_object(sp[-args].u.array->prot);
    add_ref( sp[-args].u.array->prot=fp->current_object );
  }else{
    error("creds->apply(): permission denied.\n");
  }
  pop_n_elems(args);
}

static void f_get_object_creds(INT32 args)
{
  struct object *o;
  if(args < 0 || sp[-args].type > MAX_COMPLEX)
    error("Bad argument 1 to get_object_creds\n");
  if((o=sp[-args].u.array->prot))
  {
    add_ref(o);
    pop_n_elems(args);
    push_object(o);
  }else{
    pop_n_elems(args);
    push_int(0);
  }
}

static void init_creds_object(struct object *o)
{
  THIS->user=0;
  THIS->default_creds=0;
  THIS->data_bits=0;
  THIS->may_always=0;
}

static void creds_gc_check(struct object *o)
{
  if(THIS->user) debug_gc_check(THIS->user,T_OBJECT,o);
  if(THIS->default_creds) debug_gc_check(THIS->default_creds,T_OBJECT,o);
}

static void creds_gc_mark(struct object *o)
{
  if(THIS->user) gc_mark_object_as_referenced(THIS->user);
  if(THIS->default_creds) gc_mark_object_as_referenced(THIS->default_creds);
}

static void exit_creds_object(struct object *o)
{
  if(THIS->user)
  {
    free_object(THIS->user);
    THIS->user=0;
  }

  if(THIS->default_creds)
  {
    free_object(THIS->default_creds);
    THIS->default_creds=0;
  }
}

void init_pike_security(void)
{
  struct program *tmpp;
  struct object *tmpo;

  start_new_program();

  start_new_program();
  ADD_STORAGE(struct pike_creds);
  /* function(object:void) */
  ADD_FUNCTION("set_default_creds",set_default_creds,tFunc(tObj,tVoid),0);
  /* function(:object) */
  ADD_FUNCTION("get_default_creds",get_default_creds,tFunc(tNone,tObj),0);
  /* function(:object) */
  ADD_FUNCTION("get_user",creds_get_user,tFunc(tNone,tObj),0);
  /* function(:int) */
  ADD_FUNCTION("get_allow_bits",creds_get_allow_bits,tFunc(tNone,tInt),0);
  /* function(:int) */
  ADD_FUNCTION("get_data_bits",creds_get_data_bits,tFunc(tNone,tInt),0);
  /* function(object,int,int:void) */
  ADD_FUNCTION("create",creds_create,tFunc(tObj tInt tInt,tVoid),0);
  /* function(mixed:void) */
  ADD_FUNCTION("apply",creds_apply,tFunc(tMix,tVoid),0);
  set_init_callback(init_creds_object);
  set_exit_callback(exit_creds_object);
  set_gc_check_callback(creds_gc_check);
  set_gc_mark_callback(creds_gc_mark);
  creds_program=end_program();
  add_program_constant("Creds",creds_program, 0);

  
/* function(object,mixed...:mixed) */
  ADD_EFUN("call_with_creds",f_call_with_creds,tFuncV(tObj,tMix,tMix),OPT_SIDE_EFFECT);
  
/* function(:object) */
  ADD_EFUN("get_current_creds",f_get_current_creds,tFunc(tNone,tObj),OPT_EXTERNAL_DEPEND);
  
/* function(mixed:object) */
  ADD_EFUN("get_object_creds",f_get_object_creds,tFunc(tMix,tObj),OPT_EXTERNAL_DEPEND);

#define CONST(X) add_integer_constant("BIT_" #X,SECURITY_BIT_##X,0)
  CONST(INDEX);
  CONST(SET_INDEX);
  CONST(CALL);
  CONST(SECURITY);
  CONST(NOT_SETUID);
  CONST(CONDITIONAL_IO);

  tmpp=end_program();
  add_object_constant("security",tmpo=clone_object(tmpp,0),0);
  free_object(tmpo);
  free_program(tmpp);
}

void exit_pike_security()
{
#ifdef DO_PIKE_CLEANUP
  if(creds_program)
  {
    free_program(creds_program);
    creds_program=0;
  }
#endif
}

#endif
