/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
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
#include "pike_error.h"
#include "module_support.h"

#include <md5.h>

#include "crypto.h"


#define sp Pike_sp

#undef THIS
#define THIS ((struct md5_ctx *)(Pike_fp->current_storage))
#define OBTOCTX(o) ((struct md5_ctx *)(o->storage))

static struct program *md5mod_program;

/*! @module Crypto
 */

/*! @class md5
 *!
 *! Implementation of the MD5 message digest algorithm.
 */

/*! @decl string name()
 *!
 *! Return the string @tt{"MD5"@}.
 */
static void f_name(INT32 args)
{
  if (args) 
    Pike_error("Too many arguments to md5->name()\n");
  
  push_string(make_shared_string("MD5"));
}

/*! @decl void create(Crypto.md5|void init)
 *!
 *! Initialize an MD5 digest.
 *!
 *! If the argument @[init] is specified, the state from it
 *! will be copied.
 */
static void f_create(INT32 args)
{
  if (args)
    {
      if ( ((sp-args)->type != T_OBJECT)
	   || ((sp-args)->u.object->prog != md5mod_program) )
	Pike_error("Object not of md5 type.\n");
      md5_copy(THIS, OBTOCTX((sp-args)->u.object));
    }
  else
    md5_init(THIS);
  pop_n_elems(args);
}
	  
/*! @decl Crypto.md5 update(string data)
 *!
 *! Feed some data to the digest algorithm.
 */
static void f_update(INT32 args)
{
  struct pike_string *s;
  get_all_args("_Crypto.md5->update", args, "%S", &s);

  md5_update(THIS, (unsigned INT8 *) s->str,
	     DO_NOT_WARN(s->len));
  pop_n_elems(args);
  push_object(this_object());
}

/* From RFC 1321 
 *  md5 OBJECT IDENTIFIER ::=
 *    iso(1) member-body(2) US(840) rsadsi(113549) digestAlgorithm(2) 5}
 *
 * 0x2a86 4886 f70d 0205
 */
static unsigned char md5_id[] = {
  0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05,
};

/*! @decl string identifier()
 *!
 *! Returns the ASN1 identifier for an MD5 digest.
 */
static void f_identifier(INT32 args)
{
  pop_n_elems(args);
  push_string(make_shared_binary_string((char *)md5_id, 8));
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
  
  s = begin_shared_string(MD5_DIGESTSIZE);

  md5_final(THIS);
  md5_digest(THIS, s->str);
  md5_init(THIS);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*! @endclass
 */

/*! @endmodule
 */

void pike_md5_exit(void)
{
}

void pike_md5_init(void)
{
  start_new_program();
  ADD_STORAGE(struct md5_ctx);
  /* function(void:string) */
  ADD_FUNCTION("name", f_name, tFunc(tNone, tStr), 0);
  /* function(void|object:void) */
  ADD_FUNCTION("create", f_create, tFunc(tOr(tVoid, tObj), tVoid), 0);
  /* function(string:object) */
  ADD_FUNCTION("update", f_update, tFunc(tStr, tObj), 0);
  /* function(void:string) */
  ADD_FUNCTION("digest", f_digest, tFunc(tNone, tStr), 0);
  /* function(void:string) */
  ADD_FUNCTION("identifier", f_identifier, tFunc(tNone, tStr), 0);
  end_class("md5", 0);
}
