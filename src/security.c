#include "global.h"

/* To do:
 * controls for file->connect() & file->open_socket()
 * controls for all/most functions in the system module
 * controls for all/most functions in files/efun.c
 * controls for all/most functions in spider
 * controls for threads
 * controls for destruct
 */

#ifdef PIKE_SECURITY

#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "object.h"
#include "mapping.h"
#include "multiset.h"
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

static void f_set_current_creds(INT32 args)
{
  struct object *o;

  if(!args)
  {
    /* We might want allocate a bit for this so that we can
     * disallow this
     */
    SET_CURRENT_CREDS(fp->current_object->prot);
  }else{
    CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("set_current_creds: permission denied.\n"));

    get_all_args("set_current_creds",args,"%o",&o);
    if(!valid_creds_object(o))
      error("set_current_creds: Not a valid creds object.\n");
    
    SET_CURRENT_CREDS(o);
    pop_n_elems(args);
  }
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

static void init_creds(INT32 args)
{
  struct object *o;
  INT_TYPE may,data;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("init_creds: permission denied.\n"));

  get_all_args("init_creds",args,"%o%i%i",&o,&may,&data);
  if(THIS->user)
    error("You may only call init_creds once.\n");
  
  add_ref(THIS->user=o);
  THIS->may_always=may;
  THIS->data_bits=data;
  pop_n_elems(args);
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

static void creds_gc_check(struct object *o)
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
  start_new_program();

  start_new_program();
  add_storage(sizeof(struct pike_creds));
  add_function("set_default_creds",set_default_creds,"function(object:void)",0);
  add_function("get_default_creds",get_default_creds,"function(:object)",0);
  add_function("init_creds",init_creds,"function(object,int,int:void)",0);
  add_function("apply_creds",apply_creds,"function(object:void)",0);
  set_init_callback(init_creds_object);
  set_exit_callback(exit_creds_object);
  set_gc_check_callback(creds_gc_check);
  set_gc_check_callback(creds_gc_mark);
  creds_program=end_program();

  add_efun("set_current_creds",f_set_current_creds,"function(object:void)",OPT_SIDE_EFFECT);

#define CONST(X) add_integer_constant("BIT_" #X,SECURITY_BIT_##X,0)
  CONST(INDEX);
  CONST(SET_INDEX);
  CONST(CALL);
  CONST(SECURITY);
  CONST(NOT_SETUID);
  CONST(CONDITIONAL_IO);

  end_class("security",0);
}

#endif
