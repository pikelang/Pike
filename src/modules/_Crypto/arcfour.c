/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/* arcfour.c
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
#include "pike_error.h"
#include "las.h"

#include "arcfour.h"


#define sp Pike_sp

RCSID("$Id$");

#undef THIS
#define THIS ((struct arcfour_ctx *)(Pike_fp->current_storage))

struct program *pike_arcfour_program;

void init_pike_arcfour(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct arcfour_ctx));
}

void exit_pike_arcfour(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct arcfour_ctx));
}

/*! @module Crypto
 */

/*! @class arcfour
 *!
 *! Implementation of the arcfour stream-crypto.
 */

/*! @decl string name()
 *!
 *! Returns the string @tt{"ARCFOUR"@}.
 */
static void f_name(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to arcfour->name()\n");
  }
  push_string(make_shared_string("ARCFOUR"));
}

/*! @decl int query_key_length()
 *!
 *! Returns the minimum required encryption key length.
 *! Currently this is @tt{1@}.
 */
static void f_query_key_length(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to arcfour->query_key_length()\n");
  }
  push_int(1);
}

/*! @decl void set_encrypt_key(string key)
 *! @decl void set_decrypt_key(string key)
 *!
 *! Set the encryption/decryption key to @[key].
 */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    Pike_error("Wrong number of args to arcfour->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to arcfour->set_key()\n");
  }
  if (!sp[-1].u.string->len)
    Pike_error("Empty key to arcfour_set_key()\n");
  arcfour_set_key(THIS, (unsigned INT8 *) sp[-1].u.string->str,
		  DO_NOT_WARN(sp[-1].u.string->len));

  pop_n_elems(args);
  push_object(this_object());
}

/*! @decl string crypt(string data)
 *!
 *! Returns the string @[data] encrypted with the current
 *! encryption key.
 *!
 *! @seealso
 *!   @[set_encrypt_key()]
 */
static void f_arcfour_crypt(INT32 args)
{
  ptrdiff_t len;
  struct pike_string *s;
  
  if (args != 1) {
    Pike_error("Wrong number of arguments to arcfour->crypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to arcfour->crypt()\n");
  }

  len = sp[-1].u.string->len;

  s = begin_shared_string(len);
  arcfour_crypt(THIS,
		(unsigned INT8 *) s->str,
		(unsigned INT8 *) sp[-1].u.string->str,
		DO_NOT_WARN(len));
  
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*! @endclass
 */

/*! @endmodule
 */

void pike_arcfour_init(void)
{
  start_new_program();
  ADD_STORAGE(struct arcfour_ctx);

  /* function(:string) */
  ADD_FUNCTION("name", f_name, tFunc(tNone,tStr), 0);
  /* function(:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length, tFunc(tNone, tInt), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_encrypt_key", f_set_key, tFunc(tStr, tObj), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_decrypt_key", f_set_key, tFunc(tStr, tObj), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt", f_arcfour_crypt, tFunc(tStr, tStr), 0);

  set_init_callback(init_pike_arcfour);
  set_exit_callback(exit_pike_arcfour);

  end_class("arcfour", 0);
}

void pike_arcfour_exit(void)
{
}
