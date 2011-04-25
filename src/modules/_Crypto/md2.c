/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * A pike module for MD2 hashing.
 *
 * Andreas Sigfridsson 2000-08-01
 * Based on md5.c by Henrik Grubbström and Niels Möller
 *
 */


#include "global.h"
#include "svalue.h"
#include "string.h"
#include "pike_types.h"
#include "stralloc.h"
#include "object.h"
#include "interpret.h"
#include "program.h"
#include "pike_error.h"
#include "module_support.h"

#include <md2.h>

#include "crypto.h"


#define sp Pike_sp

#undef THIS
#define THIS ((struct md2_ctx *)(Pike_fp->current_storage))
#define OBTOCTX(o) ((struct md2_ctx *)(o->storage))

static struct program *md2mod_program;

/*! @module Crypto
 */

/*! @class md2
 *!
 *! Implementation of the MD2 message digest algorithm.
 */

/*! @decl string name()
 *!
 *! Returns the string @tt{"MD2"@}.
 */
static void f_name(INT32 args)
{
  if (args)
    Pike_error("Too many arguments to md2->name()\n");

  push_string(make_shared_string("MD2"));
}

/*! @decl void create(Crypto.md2|void init)
 *!
 *! Initialize an MD2 digest.
 *!
 *! If the argument @[init] is specified, the state from it
 *! will be copied.
 */
static void f_create(INT32 args)
{
  if (args)
    {
      if ( ((sp-args)->type != T_OBJECT)
	   || ((sp-args)->u.object->prog != md2mod_program) )
	Pike_error("Object not of md2 type.\n");
      md2_copy(THIS, OBTOCTX((sp-args)->u.object));
    }
  else
    md2_init(THIS);
  pop_n_elems(args);
}

/*! @decl Crypto.md2 update(string data)
 *!
 *! Feed some data to the digest algorithm.
 */
static void f_update(INT32 args)
{
  struct pike_string *s;
  get_all_args("_Crypto.md2->update", args, "%S", &s);

  md2_update(THIS, (unsigned INT8 *) s->str,
	     DO_NOT_WARN(s->len));
  pop_n_elems(args);
  push_object(this_object());
}

/* From RFC 1319
 *  md2 OBJECT IDENTIFIER ::=
 *    iso(1) member-body(2) US(840) rsadsi(113549) digestAlgorithm(2) 2}
 *
 * 0x2a86 4886 f70d 0202
 */
static unsigned char md2_id[] = {
  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x02,
};

/*! @decl string identifier()
 *!
 *! Returns the ASN1 identifier for an MD2 digest.
 */
static void f_identifier(INT32 args)
{
  pop_n_elems(args);
  push_string(make_shared_binary_string((char *)md2_id, 8));
}

/*! @decl string digest()
 *!
 *! Return the digest.
 *!
 *! @note
 *!   As a side effect the object will be reinitialized.
 */
static void f_digest(INT32 args)
{
  struct pike_string *s;

  s = begin_shared_string(16);

  md2_digest(THIS, s->str);
  md2_init(THIS);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*! @endclass
 */

/*! @endmodule
 */

void pike_md2_exit(void)
{
}

void pike_md2_init(void)
{
  start_new_program();
  ADD_STORAGE(struct md2_ctx);
  /* function(void:string) */
  ADD_FUNCTION("name", f_name, tFunc(tNone, tStr), 0);
  /* function(void|object:void) */
  ADD_FUNCTION("create", f_create, tFunc(tOr(tVoid,tObj), tVoid), 0);
  /* function(string:object) */
  ADD_FUNCTION("update", f_update, tFunc(tStr, tObj), 0);
  /* function(void:string) */
  ADD_FUNCTION("digest", f_digest, tFunc(tNone, tStr), 0);
  /* function(void:string) */
  ADD_FUNCTION("identifier", f_identifier, tFunc(tNone, tStr), 0);
  end_class("md2", 0);
}
