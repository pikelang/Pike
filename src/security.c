/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

#include "global.h"

/* To do:
 * controls for file->pipe()
 * controls for all/most functions in spider
 * controls for threads
 */

/*! @module Pike
 */

/*! @module Security
 *!
 *! Pike has an optional internal security system, which can be
 *! enabled with the configure-option @tt{--with-security@}.
 *! 
 *! The security system is based on attaching credential objects
 *! (@[Pike.Security.Creds]) to objects, programs, arrays,
 *! mappings or multisets.
 *!
 *! A credential object in essence holds three values:
 *! @dl
 *!   @item
 *!     @tt{user@} -- The owner.
 *!   @item
 *!     @tt{allow_bits@} -- Run-time access permissions.
 *!   @item
 *!     @tt{data_bits@} -- Data access permissions.
 *! @enddl
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
#define THIS ((struct pike_creds *)(CURRENT_STORAGE))

static int valid_creds_object(struct object *o)
{
  return o &&
    get_storage(o, creds_program) == o->storage &&
    OBJ2CREDS(o)->user;
}

static void restore_creds(struct object *creds)
{
  if(Pike_interpreter.frame_pointer)
  {
    if(Pike_interpreter.frame_pointer->current_creds)
      free_object(Pike_interpreter.frame_pointer->current_creds);
    Pike_interpreter.frame_pointer->current_creds = CHECK_VALID_UID(creds);
  }else{
    if(Pike_interpreter.current_creds)
      free_object(Pike_interpreter.current_creds);
    Pike_interpreter.current_creds = CHECK_VALID_UID(creds);
  }
}

/*! @decl mixed call_with_creds(object(Creds) creds, mixed func, @
 *!                             mixed ... args)
 *!
 *! Call with credentials.
 *!
 *! Sets the current credentials to @[creds], and calls
 *! @code{@[func](@@@[args])@}. If @[creds] is @tt{0@} (zero), the
 *! credentials from the current object will be used.
 *!
 *! @note
 *!   The current creds or the current object must have the allow bit
 *!   @tt{BIT_SECURITY@} set to allow calling with @[creds] other than
 *!   @tt{0@} (zero).
 */
static void f_call_with_creds(INT32 args)
{
  struct object *o;
  ONERROR tmp;

  switch(Pike_sp[-args].type)
  {
    case T_INT:
      /* We might want allocate a bit for this so that we can
       * disallow this.
       * /hubbe
       *
       * Indeed. Consider the case when this function is used as a callback.
       * /grubba 1999-07-12
       */
      o=Pike_fp->current_object->prot;
      break;

    case T_OBJECT:
      o=Pike_sp[-args].u.object;
      if(!CHECK_SECURITY(SECURITY_BIT_SECURITY) &&
	 !(Pike_fp->current_object->prot && 
	   (OBJ2CREDS(Pike_fp->current_object->prot)->may_always & SECURITY_BIT_SECURITY)))
	Pike_error("call_with_creds: permission denied.\n");
      
      break;

    default:
      Pike_error("Bad argument 1 to call_with_creds.\n");
  }
    
  if(!valid_creds_object(o))
    Pike_error("call_with_creds: Not a valid creds object.\n");

  if(CURRENT_CREDS) add_ref(CURRENT_CREDS);

  SET_ONERROR(tmp, restore_creds, CURRENT_CREDS);

  SET_CURRENT_CREDS(o);

  /* NOTE: This only works on objects that have no credentials, or have
   * the allow_bit BIT_NOT_SETUID. Otherwise mega_apply2() will restore
   * the credentials to that of the object.
   */
  f_call_function(args-1);

  CALL_AND_UNSET_ONERROR(tmp);

  free_svalue(Pike_sp-2);
  Pike_sp[-2]=Pike_sp[-1];
  Pike_sp--;
  dmalloc_touch_svalue(Pike_sp);
}

/*! @decl object(Creds) get_current_creds()
 *! 
 *! Get the current credentials
 *!
 *! Returns the credentials that are currently active.
 *! Returns @tt{0@} (zero) if no credentials are active.
 *!
 *! @seealso
 *!   @[call_with_creds()]
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

/*! @class Creds
 *! The credentials object.
 */

/*! @decl object(Creds) get_default_creds();
 *!
 *! Get the default credentials.
 *!
 *! Returns the default credentials object if it has been set.
 *! Returns @tt{0@} (zero) if it has not been set.
 *!
 *! @seealso
 *!   @[set_default_creds()]
 */
static void get_default_creds(INT32 args)
{
  pop_n_elems(args);
  if(THIS->default_creds && THIS->default_creds->prog)
    ref_push_object(THIS->default_creds);
  else
    push_int(0);
}

/*! @decl void set_default_creds(object(Creds) creds)
 *!
 *! Set the default credentials
 *!
 *! @note
 *!   The current creds must have the allow bit
 *!   @tt{BIT_SECURITY@} set.
 *!
 *! @seealso
 *!   @[get_default_creds()]
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

/*! @decl void create(object user, int allow_bits, int data_bits)
 *!
 *! Initialize a new credentials object
 *!
 *! @note
 *!   The current creds must have the allow bit
 *!   @tt{BIT_SECURITY@} set.
 */
static void creds_create(INT32 args)
{
  struct object *o;
  INT_TYPE may,data;

  CHECK_SECURITY_OR_ERROR(SECURITY_BIT_SECURITY,
			  ("creds_create: permission denied.\n"));

  get_all_args("init_creds",args,"%o%i%i",&o,&may,&data);
  if(THIS->user)
    Pike_error("You may only call creds_create once.\n");
  
  add_ref(THIS->user=o);
  THIS->may_always=may;
  THIS->data_bits=data;
  pop_n_elems(args);
}

/*! @decl object get_user()
 *!
 *! Get the user part.
 */
static void creds_get_user(INT32 args)
{
  pop_n_elems(args);
  if(THIS->user)
    ref_push_object(THIS->user);
  else
    push_int(0);
}

/*! @decl int get_allow_bits()
 *!
 *! Get the allow_bit bitmask.
 */
static void creds_get_allow_bits(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->may_always);
}

/*! @decl int get_data_bits()
 *!
 *! Get the data_bits bitmask.
 */
static void creds_get_data_bits(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->data_bits);
}

/*! @decl void apply(object|program|function|array|mapping|multiset o)
 *!
 *! Set the credentials for @[o] to this credentials object.
 *!
 *! @note
 *!   To perform this operation the current credentials needs to have the bit
 *!   @tt{BIT_SECURITY@} set, or have the same user as the old credentials
 *!   and not change the user by performing the operation.
 */
static void creds_apply(INT32 args)
{
  if(args < 0 || Pike_sp[-args].type > MAX_COMPLEX)
    Pike_error("Bad argument 1 to creds->apply()\n");

  if( CHECK_SECURITY(SECURITY_BIT_SECURITY) ||
      (Pike_sp[-args].u.array->prot &&
       (OBJ2CREDS(current_creds)->user == THIS->user) &&
       (OBJ2CREDS(Pike_sp[-args].u.array->prot)->user == THIS->user)))
  {
    if(Pike_sp[-args].u.array->prot)
      free_object(Pike_sp[-args].u.array->prot);
    add_ref( Pike_sp[-args].u.array->prot=Pike_fp->current_object );
  }else{
    Pike_error("creds->apply(): permission denied.\n");
  }
  pop_n_elems(args);
}

/*! @endclass
 */

/*! @decl object(Creds) @
 *!          get_object_creds(object|program|function|array|mapping|multiset o)
 *!
 *! Get the credentials from @[o].
 *!
 *! Returns @tt{0@} if @[o] does not have any credentials.
 */
static void f_get_object_creds(INT32 args)
{
  struct object *o;
  if(args < 0 || Pike_sp[-args].type > MAX_COMPLEX)
    Pike_error("Bad argument 1 to get_object_creds\n");
  if((o=Pike_sp[-args].u.array->prot))
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
  if(THIS->user) debug_gc_check2(THIS->user,T_OBJECT,o,
				 " as user of Creds object");
  if(THIS->default_creds) debug_gc_check2(THIS->default_creds,T_OBJECT,o,
					  " as default creds of Creds object");
}

static void creds_gc_recurse(struct object *o)
{
  if(THIS->user) gc_recurse_object(THIS->user);
  if(THIS->default_creds) gc_recurse_object(THIS->default_creds);
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

/*! @decl constant BIT_INDEX
 *!   Allow indexing.
 */

/*! @decl constant BIT_SET_INDEX
 *!   Allow setting of indices.
 */

/*! @decl constant BIT_CALL
 *!   Allow calling of functions.
 */

/*! @decl constant BIT_SECURITY
 *!   Allow usage of security related functions.
 */

/*! @decl constant BIT_NOT_SETUID
 *!   Don't change active credentials on function call.
 */

/*! @decl constant BIT_CONDITIONAL_IO
 *! @fixme
 *!   Document this constant.
 */

/*! @decl constant BIT_DESTRUCT
 *!   Allow use of @[destruct].
 */

/*! @endmodule
 */

/*! @endmodule
 */

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
  set_gc_recurse_callback(creds_gc_recurse);
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

void exit_pike_security(void)
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
