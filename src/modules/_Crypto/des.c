/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * A pike module for getting access to some common cryptos.
 *
 * /precompiled/crypto/des
 *
 * Henrik Grubbström 1996-10-24
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
/* #include "builtin_functions.h"
 */
/* System includes */
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>

#include <des.h>


#define sp Pike_sp

struct pike_crypto_des {
  unsigned INT32 method[DES_EXPANDED_KEYLEN];
  void (*crypt_fun)(unsigned INT8 *dest,
		    unsigned INT32 *method, unsigned INT8 *src);
};

#undef THIS
#define THIS ((struct pike_crypto_des *) Pike_fp->current_storage)

/*
 * Globals
 */

struct program *pike_crypto_des_program;

/*
 * Functions
 */

static void init_pike_crypto_des(struct object *o)
{
  memset(THIS, 0, sizeof(struct pike_crypto_des));
}

static void exit_pike_crypto_des(struct object *o)
{
  memset(THIS, 0, sizeof(struct pike_crypto_des));
}

/*
 * efuns and the like
 */

/*! @module Crypto
 */

/*! @class des
 *!
 *! Implementation of the Data Encryption Standard (DES).
 */

/*! @decl string name()
 *!
 *! Return the string @tt{"DES"@}.
 */
static void f_name(INT32 args)
{
  pop_n_elems(args);
  push_constant_text("DES");
}

/*! @decl int query_block_size()
 *!
 *! Return the block size used by DES.
 */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(DES_BLOCKSIZE);
}

/*! @decl int query_key_length()
 *!
 *! Return the key length used by DES.
 */
static void f_query_key_length(INT32 args)
{
  pop_n_elems(args);
  push_int(DES_KEYSIZE);
}

/* Internal function */
static void set_key(INT32 args)
{
  if (args != 1) {
    Pike_error("Wrong number of arguments to des->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to des->set_key()\n");
  }
  if (sp[-1].u.string->len != 8)
    Pike_error("Invalid key length to des->set_key()\n");
  switch (DesMethod(THIS->method, (unsigned INT8 *)sp[-1].u.string->str))
    {
    case -1:
      Pike_error("des->set_key: parity error\n");
      break;
    case -2:
      Pike_error("des->set_key: key is weak!\n");
      break;
    case 0:
      break;
    default:
      Pike_error("des->set_key: invalid return value from desMethod, can't happen\n");
    }
  pop_n_elems(args);
  push_object(this_object());
}

/*! @decl void set_encrypt_key(string key)
 *!
 *! Set the encryption key.
 */
static void f_set_encrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = DesSmallFipsEncrypt;
}

/*! @decl void set_decrypt_key(string key)
 *!
 *! Set the decryption key.
 */
static void f_set_decrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = DesSmallFipsDecrypt;
}

/*! @decl string crypt_block(string data)
 *!
 *! En/decrypt @[data] with DES using the current key.
 */
static void f_crypt_block(INT32 args)
{
  size_t len;
  struct pike_string *s;
  size_t i;
  
  if (args != 1) {
    Pike_error("Wrong number of arguments to des->crypt_block()\n");
  }
  if (!THIS->crypt_fun)
    Pike_error("des->crypt_block: must set key first\n");
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to des->crypt_block()\n");
  }
  if ((len = sp[-1].u.string->len) % DES_BLOCKSIZE) {
    Pike_error("Bad string length in des->crypt_block()\n");
  }
  s = begin_shared_string(len);
  for(i = 0; i < len; i += DES_BLOCKSIZE)
    THIS->crypt_fun((unsigned INT8 *) s->str + i,
		    THIS->method,
		    (unsigned INT8 *) sp[-1].u.string->str + i);

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

void pike_des_init(void)
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

  /* /precompiled/crypto/des */
  start_new_program();
  ADD_STORAGE(struct pike_crypto_des);

  /* function(:string) */
  ADD_FUNCTION("name", f_name, tFunc(tNone, tString), 0);
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
  set_init_callback(init_pike_crypto_des);
  set_exit_callback(exit_pike_crypto_des);

  end_class("des", 0);
}

void pike_des_exit(void)
{
}
