/*
 * $Id: md5.c,v 1.8 1998/04/24 00:59:26 hubbe Exp $
 *
 * A pike module for getting access to some common cryptos.
 *
 * /precompiled/crypto/md5
 *
 * Henrik Grubbström 1996-10-24
 *
 * SSL dependencies purged, 1997-02-27. /Niels Möller
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

#include <md5.h>

#undef THIS
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
  struct pike_string *s;
  get_all_args("_Crypto.md5->update", args, "%S", &s);

  md5_update(THIS, (unsigned INT8 *) s->str, s->len);
  pop_n_elems(args);
  push_object(this_object());
}

static void f_identifier(INT32 args)
{
  pop_n_elems(args);
  push_string(make_shared_binary_string(
    "\x2a\x86\x48\x86\xf7\x0d\x02\x05", 8));
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

void pike_md5_exit(void)
{
}

void pike_md5_init(void)
{
  start_new_program();
  add_storage(sizeof(struct md5_ctx));
  add_function("name", f_name, "function(void:string)", 0);
  add_function("create", f_create, "function(void|object:void)", 0);
  add_function("update", f_update, "function(string:object)", 0);
  add_function("digest", f_digest, "function(void:string)", 0);
  add_function("identifier", f_identifier, "function(void:string)", 0);
  end_class("md5", 0);
}
