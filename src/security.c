#include "global.h"

#ifdef PIKE_SECURITY

#include "interpret.h"
#include "svalue.h"
#include "array.h"
#include "object.h"
#include "mapping.h"
#include "multiset.h"
#include "security.h"
#include "module_support.h"

static struct program *uid_program;
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

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY, ("set_current_creds: permission denied.\n"));

  get_all_args("set_current_creds",args,"%o",&o);
  if(!valid_creds_object(o))
    error("set_current_creds: Not a valid creds object.\n");

  SET_CURRENT_CREDS(o);
  pop_n_elems(args);
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

  get_all_args("set_default_creds",args,"%o",&o);

  if(get_storage(o, creds_program) != o->storage)
    error("set_default_creds: Not a valid creds object.\n");

  /* There is really no techincal reason for this, should we allow it? */
  if(OBJ2CREDS(o)->user != THIS->user)
    error("set_default_creds: You cannot use somebody elses creds!\n");

  if(THIS->default_creds) free_object(THIS->default_creds);
  if(o == fp->current_object)
  {
    add_ref(THIS->default_creds = o);
  }else{
    THIS->default_creds=0;
  }
  pop_n_elems(args);
}

void init_pike_security(void)
{
  start_new_program();

  start_new_program();
  add_storage(sizeof(struct pike_creds));
  add_function("set_default_creds",set_default_creds,"function(object:void)",0);
  add_function("get_default_creds",get_default_creds,"function(:object)",0);
  creds_program=end_program();

  start_new_program();
  add_storage(sizeof(struct pike_uid));
  uid_program=end_program();

  end_class("security",0);
}

#endif
