/*
 * $Id: rsa.c,v 1.2 2000/01/27 00:23:42 grubba Exp $
 *
 * Glue to RSA BSAFE's RSA implementation.
 *
 * Henrik Grubbström 2000-01-26
 */

#include "config.h"
#include "global.h"
#include "svalue.h"
#include "program.h"

#include "crypto.h"

#if defined(HAVE_B_RSA_PUBLIC_KEY_TYPE) && defined(AUTO_BIGNUM)
#define HAVE_RSA_LIB
#endif /* HAVE_B_RSA_PUBLIC_KEY_TYPE && AUTO_BIGNUM */

#ifdef HAVE_RSA_LIB

#include <bsafe.h>

RSCID("$Id: rsa.c,v 1.2 2000/01/27 00:23:42 grubba Exp $");

struct pike_rsa_data
{
  B_ALGORITHM_OBJ cipher;
  B_KEY_OBJ key;
};

#define THIS ((struct pike_rsa_data *)(fp->current_storage))

static void init_pike_rsa(struct object *o)
{
  int code;

  MEMSET(THIS, 0, sizeof(struct pike_rsa_data));

  if ((code = B_CreateKeyObject(&(THIS->key)))) {
    error("Crypto.rsa(): Failed to create key object.\n");
  }
  if ((code = B_CreateAlgorithmObject(&(THIS->cipher)))) {
    error("Crypto.rsa(): Failed to create cipher object.\n");
  }
}

static void exit_pike_rsa(struct object *o)
{
  if (THIS->cipher) {
    B_DestroyAlgorithmObject(&(THIS->cipher));
  }
  if (THIS->key) {
    B_DestroyKeyObject(&(THIS->key));
  }
  MEMSET(THIS, 0, sizeof(struct pike_rsa_data));
}

/* object set_public_key(bignum modulo, bignum pub) */
static void f_set_public_key(INT32 args)
{
  A_RSA_KEY key_info;
  ONERROR tmp;
  struct object *modulo = NULL;
  struct object *pub = NULL;
  int code;

  get_all_args("set_public_key", args, "%o%o", &modulo, &pub);

  push_int(256);
  apply(modulo, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from modulo->digits().\n");
  }

  key_info.modulus.data = sp[-1].u.string->str;
  key_info.modulus.len = sp[-1].u.string->len;

  push_int(256);
  apply(pub, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from pub->digits().\n");
  }

  key_info.exponent.data = sp[-1].u.string->str;
  key_info.exponent.len = sp[-1].u.string->len;

  if ((code = B_SetKeyInfo(THIS->key, KI_RSAPublic, (POINTER)&A_RSA_KEY))) {
    error("Failed to set public key.\n");
  }
  
  pop_n_elems(args + 2);
  ref_push_object(fp->current_object);
}

/* object set_private_key(bignum priv, array(bignum)|void extra) */
static void f_set_private_key(INT32 args)
{
  A_RSA_KEY key_info;
  ONERROR tmp;
  struct object *priv = NULL;
  struct array *extra = NULL;
  int code;

  get_all_args("set_private_key", args, "%o%a", &priv, &extra);

#if 0
  push_int(256);
  apply(pub, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from pub->digits().\n");
  }

  key_info.modulus.data = sp[-1].u.string->str;
  key_info.modulus.len = sp[-1].u.string->len;
#endif /* 0 */

  push_int(256);
  apply(priv, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from priv->digits().\n");
  }

  key_info.exponent.data = sp[-1].u.string->str;
  key_info.exponent.len = sp[-1].u.string->len;

  if ((code = B_SetKeyInfo(THIS->key, KI_RSAPrivate, (POINTER)&A_RSA_KEY))) {
    error("Failed to set public key.\n");
  }
  
  pop_n_elems(args + 2);
  ref_push_object(fp->current_object);
}

static void f_query_blocksize(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_rsa_pad(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_rsa_unpad(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_raw_sign(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_raw_verify(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_sign(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_verify(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_sha_sign(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_sha_verify(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_generate_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_encrypt(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_decrypt(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_set_encrypt_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_set_decrypt_key(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_crypt_block(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_rsa_size(INT32 args)
{
  pop_n_elems(args);
  push_int(0);
}

static void f_public_key_equal(INT32 args)
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

  ADD_STORAGE(struct pike_rsa_data);

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

  set_init_callback(init_pike_rsa);
  set_exit_callback(exit_pike_rsa);
  
  end_class("rsa", 0);
#endif /* HAVE_RSA_LIB */
}

void pike_rsa_exit(void)
{
}
