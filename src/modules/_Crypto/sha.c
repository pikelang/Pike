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

static void shamod_create(INT32 args)
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
	  
static void shamod_update(INT32 args)
{
  sha_update(THIS, (unsigned INT8 *) (sp-args)->u.string->str, (sp-args)->u.string->len);
  pop_n_elems(args);
}

static void shamod_digest(INT32 args)
{
  struct pike_string *s;
  
  s = begin_shared_string(SHA_DIGESTSIZE);

  sha_final(THIS);
  sha_digest(THIS, s->str);
  sha_init(THIS);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

void MOD_INIT2(sha)(void) {}
void MOD_EXIT(sha)(void)
{
  free_program(shamod_program);
}

void MOD_INIT(sha)(void)
{
  start_new_program();
  add_storage(sizeof(struct sha_ctx));
  add_function("create", shamod_create, "function(void|object:void)", 0);
  add_function("update", shamod_update, "function(string:void)", 0);
  add_function("digest", shamod_digest, "function(void:string)", 0);
  shamod_program = end_c_program(MODULE_PREFIX "sha");
  shamod_program->refs++;
}
