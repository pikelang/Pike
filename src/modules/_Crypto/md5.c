/*
 * $Id: md5.c,v 1.2 1997/02/27 13:52:54 nisse Exp $
 *
 * A pike module for getting access to some common cryptos.
 *
 * /precompiled/crypto/md5
 *
 * Henrik Grubbström 1996-10-24
 *
 * SSL dependencies purged, 1997-0227. /Niels Möller
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

#include "md5.h"

/* Module specific includes */
#include "precompiled_crypto.h"

#define THIS ((struct md5_ctx *)(fp->current_storage))
#define OBTOCTX(o) ((struct md5_ctx *)(o->storage))

static struct program *md5mod_program;

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) 
    error("Too many arguments to md5->name()\n");
  
  push_string(make_shared_string("MD5"));
}

static void f_create(INT32 args)
{
  if (args)
    {
      if ( ((sp-args)->type != T_OBJECT)
	   || ((sp-args)->u.object->prog != md5mod_program) )
	error("Object not of md5 type.\n");
      md5_copy(THIS, OBTOCTX((sp-args)->u.object));
    }
  else
    md5_init(THIS);
  pop_n_elems(args);
}
	  
static void f_update(INT32 args)
{
  md5_update(THIS, (unsigned INT8 *) (sp-args)->u.string->str, (sp-args)->u.string->len);
  pop_n_elems(args);
}

static void f_digest(INT32 args)
{
  struct pike_string *s;
  
  s = begin_shared_string(MD5_DIGESTSIZE);

  md5_final(THIS);
  md5_digest(THIS, s->str);
  md5_init(THIS);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

void MOD_EXIT(md5)(void)
{
}

void MOD_INIT(md5)(void)
{
  start_new_program();
  add_storage(sizeof(struct md5_ctx));
  add_function("name", f_name, "function(void:string)", OPT_TRY_OPTIMIZE);
  add_function("create", f_create, "function(void|object:void)", 0);
  add_function("update", f_update, "function(string:void)", 0);
  add_function("digest", f_digest, "function(void:string)", 0);
  end_class(MODULE_PREFIX "md5", 0);
}
