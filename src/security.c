#include "global.h"

/* To do:
 * controls for file->pipe()
 * controls for all/most functions in spider
 * controls for threads
 */

/*: <pikedoc>
 *: <section title="Internal security">
 *: Pike has an optional internal security system, which can be
 *: enabled with the configure-option <code language=sh>--with-security</code>.
 *: <p>
 *: The security system is based on attaching credential objects
 *: (<code language=pike>__builtin.security.Creds</code>) to objects,
 *: programs, arrays, mappings or multisets.
 *: <p>
 *: A credential object in essence holds three values:
 *: <ul>
 *: <li><code language=pike>user</code> -- The owner.
 *: <li><code language=pike>allow_bits</code> -- Run-time access permissions.
 *: <li><code language=pike>data_bits</code> -- Data access permissions.
 *: </ul>
 *: <p>
 *: The following security bits are currently defined:
 *: <ul>
 *: <li><code language=pike>BIT_INDEX</code> -- Allow indexing.
 *: <li><code language=pike>BIT_SET_INDEX</code> -- Allow setting of indices.
 *: <li><code language=pike>BIT_CALL</code> -- Allow calling of functions.
 *: <li><code language=pike>BIT_SECURITY</code> -- Allow usage of security
 *: related functions.
 *: <li><code language=pike>BIT_NOT_SETUID</code> -- Don't change active
 *: credentials on function call.
 *: <li><code language=pike>BIT_CONDITIONAL_IO</code> -- ??
 *: <li><code language=pike>BIT_DESTRUCT</code> -- Allow use of
 *: <code language=pike>destruct()</code>.
 *: </ul>
 *: </pikedoc>
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

/*: <pikedoc>
 *: <function name=call_with_creds title="call with credentials">
 *: <man_syntax>
 *: mixed call_with_creds(object(Creds) creds, mixed func, mixed ... args); 
 *: </man_syntax>
 *: <man_description>
 *: Sets the current credentials to <arg>creds</arg>, and calls
 *: <code language=pike><arg>func</arg>(@<arg>args</arg>)</code>.
 *: </man_description>
 *: </function>
 *: </pikedoc>
 */
static void f_call_with_creds(INT32 args)
{
  struct object *o;

  switch(sp[-args].type)
  {
    case T_INT:
      /* We might want allocate a bit for this so that we can
       * disallow this.
       * /hubbe
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

  /* FIXME: Does this work?
   * Won't mega_apply2() force current_creds to o->prot anyway?
   * /grubba 1999-07-09
   */
  f_call_function(args-1);

  /* FIXME: Shouldn't the original creds be restored here?
   * /grubba 1999-07-09
   */

  free_svalue(sp-2);
  sp[-2]=sp[-1];
  sp--;
}

/*: <pikedoc>
 *: <function name=get_current_creds title="get the current credentials">
 *: <man_syntax>
 *: object(Creds) get_current_creds();
 *: </man_syntax>
 *: <man_description>
 *: Returns the credentials that are currently active.
 *: Returns 0 if no credentials are active.
 *: </man_description>
 *: </function>
 *: </pikedoc>
 */
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

/*: <pikedoc>
 *: <class name=Creds>
 *: The credentials object.
 *:
 *: <method name=get_default_creds title="get the default credentials">
 *: <man_syntax>
 *: object(Creds) get_default_creds();
 *: </man_syntax>
 *: <man_description>
 *: Returns the default credentials object if it has been set.
 *: Returns 0 if it has not been set.
 *: </man_description>
 *: </method>
 *: </pikedoc>
 */
static void get_default_creds(INT32 args)
{
  pop_n_elems(args);
  if(THIS->default_creds && THIS->default_creds->prog)
    ref_push_object(THIS->default_creds);
  else
    push_int(0);
}

/*: <pikedoc>
 *: <method name=set_default_creds title="set the default credentials">
 *: <man_syntax>
 *: void set_default_creds(object(Creds) creds);
 *: </man_syntax>
 *: <man_description>
 *: Set the default credentials.
 *: </man_description>
 *: <man_note>
 *: The current creds must have the allow bit
 *: <code language=pike>BIT_SECURITY</code>.
 *: </man_note>
 *: </method>
 *: </pikedoc>
 */
static void set_default_creds(INT32 args)
{
  struct object *o;
  INT_TYPE may,data;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("set_default_creds: permission denied.\n"));

  get_all_args("init_creds",args,"%o",&o);
  
  if(THIS->default_creds) free_object(THIS->default_creds);
  add_ref(THIS->default_creds=o);
  pop_n_elems(args);
}

/*: <pikedoc>
 *: <method name=create title="initialize a new credentials object">
 *: <man_syntax>
 *: void create(object user, int allow_bits, int data_bits);
 *: </man_syntax>
 *: <man_description>
 *: Initialize a new credentials object.
 *: </man_description>
 *: <man_note>
 *: The current creds must have the allow bit
 *: <code language=pike>BIT_SECURITY</code>.
 *: </man_note>
 *: </method>
 *: </pikedoc>
 */
static void creds_create(INT32 args)
{
  struct object *o;
  INT_TYPE may,data;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("creds_create: permission denied.\n"));

  get_all_args("init_creds",args,"%o%i%i",&o,&may,&data);
  if(THIS->user)
    error("You may only call creds_create once.\n");
  
  add_ref(THIS->user=o);
  THIS->may_always=may;
  THIS->data_bits=data;
  pop_n_elems(args);
}

/*: <pikedoc type=txt>
 *: METHOD get_user get the user part
 *: SYNTAX
 *: 	object get_user();
 *: DESCRIPTION
 *: 	Returns the user part.
 *: </pikedoc>
 */
static void creds_get_user(INT32 args)
{
  pop_n_elems(args);
  if(THIS->user)
    ref_push_object(THIS->user);
  else
    push_int(0);
}

/*: <pikedoc type=txt>
 *: METHOD get_allow_bits get the allow_bit part
 *: SYNTAX
 *: 	int get_allow_bits();
 *: DESCRIPTION
 *: 	Returns the allow_bit bitmask.
 *: </pikedoc>
 */
static void creds_get_allow_bits(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->may_always);
}

/*: <pikedoc type=txt>
 *: METHOD get_data_bits get the data_bits part
 *: SYNTAX
 *: 	int get_data_bits();
 *: DESCRIPTION
 *: 	Returns the data_bits bitmask.
 *: </pikedoc>
 */
static void creds_get_data_bits(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->data_bits);
}

/*: <pikedoc type=txt>
 *: METHOD apply set the credentials for an object, program etc.
 *: SYNTAX
 *: 	void creds->apply(object|program|function|array|mapping|multiset o);
 *: DESCRIPTION
 *: 	Sets the credentials for <arg>o</arg>.
 *: NOTE
 *: 	To perform this operation the current credentials needs to have the bit
 *:	<code language=pike>BIT_SECURITY</code> set, or have the same user
 *: 	as the old credentials and not change the user by performing the
 *: 	operation.
 *: </pikedoc>
 */
static void creds_apply(INT32 args)
{
  if(args < 0 || sp[-args].type > MAX_COMPLEX)
    error("Bad argument 1 to creds->apply()\n");

  if( CHECK_SECURITY(SECURITY_BIT_SECURITY) ||
      (sp[-args].u.array->prot &&
       (OBJ2CREDS(current_creds)->user == THIS->user) &&
       (OBJ2CREDS(sp[-args].u.array->prot)->user == THIS->user)))
  {
    if(sp[-args].u.array->prot)
      free_object(sp[-args].u.array->prot);
    add_ref( sp[-args].u.array->prot=fp->current_object );
  }else{
    error("creds->apply(): permission denied.\n");
  }
  pop_n_elems(args);
}

/*: <pikedoc>
 *: </class>
 *: </pikedoc>
 */

/*: <pikedoc type=txt>
 *: FUNCTION get_object_creds get the credentials from an object, program etc.
 *: SYNTAX
 *: 	object(Creds) get_object_creds(object|program|function|array|mapping|multiset o)
 *: DESCRIPTION
 *: 	Retuns the credentials from <arg>o</arg>.
 *: 	Returns 0 if <arg>o</arg> does not have any credentials.
 *: </pikedoc>
 */
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

#define CONST(X) add_integer_constant("BIT_" #X,PIKE_CONCAT(SECURITY_BIT_,X),0)
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

/*: <pikedoc>
 *: </section>
 *: </pikedoc>
 */

#endif
