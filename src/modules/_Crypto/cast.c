/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * CAST crypto module for Pike
 *
 * Niels Möller 1997-11-03
 *
 */

/*
 * Includes
 */

/* From the Pike distribution */
#include "global.h"
#include "stralloc.h"
#include "interpret.h"
#include "svalue.h"
#include "object.h"
#include "pike_error.h"
#include "las.h"

#include <cast.h>


#define sp Pike_sp

struct pike_crypto_cast {
  struct cast_key key;
  void (*crypt_fun)(struct cast_key* key, unsigned INT8* inblock,
		    unsigned INT8* outblock);
};

#undef THIS
#define THIS ((struct pike_crypto_cast *)(Pike_fp->current_storage))
#define OBTOCTX(o) ((struct pike_crypto_cast *)(o->storage))

/*
 * Globals
 */

struct program *pike_crypto_cast_program;

/*
 * Functions
 */

void init_pike_crypto_cast(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct pike_crypto_cast));
}

void exit_pike_crypto_cast(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct pike_crypto_cast));
}

/*
 * Methods
 */

/*! @module Crypto
 */

/*! @class cast
 *!
 *! Implementation of the CAST block crypto.
 */

/*! @decl string name()
 *!
 *! Returns the string @tt{"CAST"@}.
 */
static void f_name(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to cast->name()\n");
  }
  push_string(make_shared_string("CAST"));
}

/*! @decl int query_block_size()
 *!
 *! Return the encryption block size used by CAST.
 */
static void f_query_block_size(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to cast->query_block_size()\n");
  }
  push_int(CAST_BLOCKSIZE);
}

/*! @decl int query_key_length()
 *!
 *! Return the encryption key length used by CAST.
 */
static void f_query_key_length(INT32 args)
{
  if (args) {
    Pike_error("Too many arguments to cast->query_key_length()\n");
  }
  push_int(CAST_MAX_KEYSIZE);
}

/* Not public. */
static void set_key(INT32 args)
{
  if (args != 1) {
    Pike_error("Wrong number of arguments to des->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to des->set_key()\n");
  }
  if ( (sp[-1].u.string->len < CAST_MIN_KEYSIZE)
       || (sp[-1].u.string->len > CAST_MAX_KEYSIZE))
    Pike_error("Invalid key length to cast->set_key()\n");

  cast_setkey(&(THIS->key), (unsigned char *)sp[-1].u.string->str,
	      DO_NOT_WARN(sp[-1].u.string->len));
  
  pop_n_elems(args);
  push_object(this_object());
}

/*! @decl Crypto.cast set_encrypt_key(string key)
 *!
 *! Set the encryption key to @[key], and set the object
 *! to encryption mode.
 *!
 *! Returns the current object.
 */
static void f_set_encrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = cast_encrypt;
}

/*! @decl Crypto.cast set_decrypt_key(string key)
 *!
 *! Set the decryption key to @[key], and set the object
 *! to decryption mode.
 *!
 *! Returns the current object.
 */
static void f_set_decrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = cast_decrypt;
}

/*! @decl string crypt_block(string data)
 *!
 *! Encrypt/decrypt the string @[data] with the current key.
 *!
 *! If @[data] is longer than the block size, additional blocks
 *! will be encrypted the same way as the first one (ie ECB-mode).
 *!
 *! @note
 *!   Will throw errors if @code{sizeof(@[data])@} is not a multiple
 *!   of the block size.
 *!
 *! @seealso
 *!   @[set_encrypt_key()], @[set_decrypt_key()]
 */
static void f_crypt_block(INT32 args)
{
  ptrdiff_t len;
  struct pike_string *s;
  INT32 i;
  
  if (args != 1) {
    Pike_error("Wrong number of arguments to cast->crypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to cast->crypt()\n");
  }

  len = sp[-1].u.string->len;
  if (len % CAST_BLOCKSIZE) {
    Pike_error("Bad length of argument 1 to cast->crypt()\n");
  }

  if (!THIS->key.rounds)
    Pike_error("Crypto.cast->crypt_block: Key not set\n");
  s = begin_shared_string(len);
  for(i = 0; i < len; i += CAST_BLOCKSIZE)
    THIS->crypt_fun(&(THIS->key), 
		    (unsigned INT8 *) sp[-1].u.string->str + i,
		    (unsigned INT8 *) s->str + i);
  
  pop_n_elems(args);

  push_string(end_shared_string(s));
}

/*! @endclass
 */

/*! @endmodule
 */

/*
 * Module linkage
 */

void pike_cast_init(void)
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

  start_new_program();
  ADD_STORAGE(struct pike_crypto_cast);

  /* function(void:string) */
  ADD_FUNCTION("name", f_name, tFunc(tNone, tStr), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_block_size", f_query_block_size, tFunc(tNone, tInt), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length, tFunc(tNone, tInt), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_encrypt_key", f_set_encrypt_key, tFunc(tStr, tObj), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_decrypt_key", f_set_decrypt_key, tFunc(tStr, tObj), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt_block", f_crypt_block, tFunc(tStr, tStr), 0);

  set_init_callback(init_pike_crypto_cast);
  set_exit_callback(exit_pike_crypto_cast);

  end_class("cast", 0);
}

void pike_cast_exit(void)
{
}
