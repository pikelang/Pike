/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: invert.c,v 1.17 2003/04/07 17:20:45 nilsson Exp $
*/

/*
 * INVERT crypto module for Pike
 *
 * /precompiled/crypto/invert
 *
 * Henrik Grubbström 1996-11-08
 */

/*
 * Includes
 */

/* From the Pike distribution */
#include "global.h"
#include "stralloc.h"
#include "interpret.h"
#include "svalue.h"
#include "constants.h"
#include "pike_macros.h"
#include "threads.h"
#include "object.h"
#include "stralloc.h"
#include "builtin_functions.h"


#define sp Pike_sp

/*
 * Globals
 */

struct program *pike_crypto_invert_program;

/*
 * Functions
 */

void init_pike_crypto_invert(struct object *o)
{
}

void exit_pike_crypto_invert(struct object *o)
{
}

/*
 * efuns and the like
 */

/*! @module Crypto
 */

/*! @class invert
 *!
 *! Inversion crypto module.
 */

/*! @decl string name()
 *!
 *! Returns the string @expr{"INVERT"@}.
 */
static void f_name(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to invert->name()\n");
  }
  push_string(make_shared_string("INVERT"));
}

/*! @decl int query_block_size()
 *!
 *! Returns the block size for the invert crypto (currently 8).
 */
static void f_query_block_size(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to invert->query_block_size()\n");
  }
  push_int(8);
}

/*! @decl int query_key_length()
 *!
 *! Returns the minimum key length for the invert crypto.
 *!
 *! Since this crypto doesn't use a key, this will be 0.
 */
static void f_query_key_length(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to invert->query_key_length()\n");
  }
  push_int(0);
}

/*! @decl void set_key(string key)
 *!
 *! Set the encryption key (currently a no op).
 */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    Pike_error("Wrong number of args to invert->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to invert->set_key()\n");
  }
  pop_n_elems(args);
  push_object(this_object());
}

/*! @decl string crypt_block(string data)
 *!
 *! De/encrypt the string @[data] with the invert crypto
 *! (ie invert the string).
 */
static void f_crypt_block(INT32 args)
{
  char *buffer;
  ptrdiff_t i;
  ptrdiff_t len;

  if (args != 1) {
    Pike_error("Wrong number of arguments to invert->crypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to invert->crypt_block()\n");
  }
  if (sp[-1].u.string->len % 8) {
    Pike_error("Bad length of argument 1 to invert->crypt_block()\n");
  }

  if (!(buffer = alloca(len = sp[-1].u.string->len))) {
    Pike_error("invert->crypt_block(): Out of memory\n");
  }

  for (i=0; i<len; i++) {
    buffer[i] = ~sp[-1].u.string->str[i];
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string(buffer, len));

  MEMSET(buffer, 0, len);
}

/*! @endclass
 */

/*! @endmodule
 */

/*
 * Module linkage
 */

void pike_invert_init(void)
{
  /*
   * start_new_program();
   *
   * add_storage();
   *
   * add_function();
   * add_function();
   * ...
   *
   * set_init_callback();
   * set_exit_callback();
   *
   * program = end_c_program();
   * program->refs++;
   *
   */

  /* /precompiled/crypto/invert */
  start_new_program();

  /* function(void:string) */
  ADD_FUNCTION("name", f_name, tFunc(tNone, tStr), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_block_size", f_query_block_size, tFunc(tNone, tInt), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length, tFunc(tNone, tInt), 0);
  /* function(string:void) */
  ADD_FUNCTION("set_encrypt_key", f_set_key, tFunc(tStr, tVoid), 0);
  /* function(string:void) */
  ADD_FUNCTION("set_decrypt_key", f_set_key, tFunc(tStr, tVoid), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt_block", f_crypt_block, tFunc(tStr, tStr), 0);

  set_init_callback(init_pike_crypto_invert);
  set_exit_callback(exit_pike_crypto_invert);

  end_class("invert", 0);
}

void pike_invert_exit(void)
{
}
