/* sha.c
 *
 * Written by Niels Möller
 */


#include "global.h"
#include "svalue.h"
#include "string.h"
#include "pike_types.h"
#include "stralloc.h"
#include "object.h"
#include "interpret.h"
#include "program.h"
#include "error.h"

#include "sha.h"

/* Module specific includes */
#include "precompiled_crypto.h"

#define THIS ((struct sha_ctx *)(fp->current_storage))
#define OBTOCTX(o) ((struct sha_ctx *)(o->storage))

static struct program *shamod_program;

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) 
    error("Too many arguments to sha->name()\n");
  
  push_string(make_shared_string("SHA"));
}

static void f_create(INT32 args)
{
  if (args)
    {
      if ( ((sp-args)->type != T_OBJECT)
	   || ((sp-args)->u.object->prog != shamod_program) )
	error("Object not of sha type.\n");
      sha_copy(THIS, OBTOCTX((sp-args)->u.object));
    }
  else
    sha_init(THIS);
  pop_n_elems(args);
}
	  
static void f_update(INT32 args)
{
  sha_update(THIS, (unsigned INT8 *) (sp-args)->u.string->str, (sp-args)->u.string->len);
  pop_n_elems(args);
}

static void f_digest(INT32 args)
{
  struct pike_string *s;
  
  s = begin_shared_string(SHA_DIGESTSIZE);

  sha_final(THIS);
  sha_digest(THIS, s->str);
  sha_init(THIS);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

void MOD_EXIT(sha)(void)
{
}

void MOD_INIT(sha)(void)
{
  start_new_program();
  add_storage(sizeof(struct sha_ctx));
  add_function("name", f_name, "function(void:string)", OPT_TRY_OPTIMIZE);
  add_function("create", f_create, "function(void|object:void)", 0);
  add_function("update", f_update, "function(string:void)", 0);
  add_function("digest", f_digest, "function(void:string)", 0);
  end_class(MODULE_PREFIX "sha", 0);
}
