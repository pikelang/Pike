/*
 * $Id: rsa.c,v 1.1 2000/01/26 19:48:26 grubba Exp $
 *
 * Glue to RSA BSAFE's RSA implementation.
 *
 * Henrik Grubbström 2000-01-26
 */

#include "config.h"
#include "global.h"
#include "svalue.h"
#include "program.h"

#ifdef HAVE_B_RSA_PUBLIC_KEY_TYPE
#define HAVE_RSA_LIB
#endif /* HAVE_B_RSA_PUBLIC_KEY_TYPE */

#ifdef HAVE_RSA_LIB

#include <bsafe.h>

RSCID("$Id: rsa.c,v 1.1 2000/01/26 19:48:26 grubba Exp $");

void f_set_public_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_set_private_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_query_blocksize(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_rsa_pad(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_rsa_unpad(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_raw_sign(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_raw_verify(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_sign(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_verify(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_sha_sign(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_sha_verify(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_generate_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_encrypt(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_decrypt(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_set_encrypt_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_set_decrypt_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_crypt_block(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_rsa_size(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

void f_public_key_equal(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

#endif /* HAVE_RSA_LIB */

/*
 * Module linkage.
 */

void pike_rsa_init(void)
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
#ifdef HAVE_RSA_LIB
  start_new_program();

  ADD_FUNCTION("set_public_key", f_set_public_key,
	       tFunc(tObj tObj,tObj), 0);
  
  ADD_FUNCTION("set_private_key", f_set_private_key,
	       tFunc(tObj tOr(tArr(tObj),tVoid),tObj), 0);

  ADD_FUNCTION("query_blocksize", f_query_blocksize,
	       tFunc(tNone, tInt), 0);

  ADD_FUNCTION("rsa_pad", f_rsa_pad,
	       tFunc(tString tInt tOr(tMix,tVoid), tObj), 0);

  ADD_FUNCTION("rsa_unpad", f_rsa_unpad,
	       tFunc(tObj tInt, tString), 0);

  ADD_FUNCTION("raw_sign", f_raw_sign,
	       tFunc(tString, tObj), 0);

  ADD_FUNCTION("raw_verify", f_raw_verify,
	       tFunc(tString tObj, tInt), 0);

  ADD_FUNCTION("sign", f_sign,
	       tFunc(tString tProg tOr(tMix, tVoid), tObj), 0);

  ADD_FUNCTION("verify", f_verify,
	       tFunc(tString tProg tObj, tInt), 0);

  ADD_FUNCTION("sha_sign", f_sha_sign,
	       tFunc(tString tOr(tMix, tVoid), tString), 0);

  ADD_FUNCTION("sha_verify", f_sha_verify,
	       tFunc(tString tString, tInt), 0);

  ADD_FUNCTION("generate_key", f_generate_key,
	       tFunc(tInt tOr(tFunction, tVoid), tObj), 0);

  ADD_FUNCTION("encrypt", f_encrypt,
	       tFunc(tString tOr(tMix, tVoid), tString), 0);

  ADD_FUNCTION("decrypt", f_decrypt,
	       tFunc(tString, tString), 0);

  ADD_FUNCTION("set_encrypt_key", f_set_encrypt_key,
	       tFunc(tArr(tObj), tObj), 0);

  ADD_FUNCTION("set_decrypt_key", f_set_decrypt_key,
	       tFunc(tArr(tObj), tObj), 0);

  ADD_FUNCTION("crypt_block", f_crypt_block,
	       tFunc(tString, tString), 0);

  ADD_FUNCTION("rsa_size", f_rsa_size,
	       tFunc(tNone, tInt), 0);

  ADD_FUNCTION("public_key_equal", f_public_key_equal,
	       tFunc(tObj, tInt), 0);
  
  end_class("idea", 0);
#endif /* HAVE_RSA_LIB */
}

void pike_rsa_exit(void)
{
}
