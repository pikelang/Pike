/* $Id: sha.c,v 1.15 1999/02/10 21:51:41 hubbe Exp $
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
#include "module_support.h"
#include "las.h"

RCSID("$Id: sha.c,v 1.15 1999/02/10 21:51:41 hubbe Exp $");

#include <sha.h>

#undef THIS
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
  struct pike_string *s;
  get_all_args("_Crypto.sha->update", args, "%S", &s);

  sha_update(THIS, (unsigned INT8 *) s->str, s->len);
  pop_n_elems(args);
  push_object(this_object());
}

  /*
   *  SHA1 OBJECT IDENTIFIER ::= {
   *    iso(1) identified-organization(3) oiw(14) secsig(3) 
   *      algorithm(2) 26 
   *  }
   *
   * 0x2b0e 0302 1a
   */
static char sha_id[] = {
  0x2b, 0x0e, 0x03, 0x02, 0x1a,
};

static void f_identifier(INT32 args)
{
  pop_n_elems(args);
  push_string(make_shared_binary_string(sha_id, 5));
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

void pike_sha_exit(void)
{
}

void pike_sha_init(void)
{
  start_new_program();
  ADD_STORAGE(struct sha_ctx);
  /* function(void:string) */
  ADD_FUNCTION("name", f_name,tFunc(tVoid,tStr), OPT_TRY_OPTIMIZE);
  /* function(void|object:void) */
  ADD_FUNCTION("create", f_create,tFunc(tOr(tVoid,tObj),tVoid), 0);
  /* function(string:object) */
  ADD_FUNCTION("update", f_update,tFunc(tStr,tObj), 0);
  /* function(void:string) */
  ADD_FUNCTION("digest", f_digest,tFunc(tVoid,tStr), 0);
  end_class("sha", 0);
}
