/*
 * $Id: rsa.c,v 1.4 2000/01/31 20:09:00 grubba Exp $
 *
 * Glue to RSA BSAFE's RSA implementation.
 *
 * Henrik Grubbström 2000-01-26
 */

#include "config.h"
#include "global.h"
#include "svalue.h"
#include "program.h"
#include "object.h"
#include "interpret.h"
#include "error.h"

#include "module_support.h"

#include "crypto.h"

#ifdef HAVE_LIBCRYPTOCI
#ifndef HAVE_RSA_LIB
#define HAVE_RSA_LIB
#endif /* !HAVE_RSA_LIB */
#endif /* HAVE_LIBCRYPTOCI */

#ifdef HAVE_RSA_LIB

#include <bsafe.h>

RCSID("$Id: rsa.c,v 1.4 2000/01/31 20:09:00 grubba Exp $");

struct pike_rsa_data
{
  B_ALGORITHM_OBJ cipher;
  B_KEY_OBJ key;
  unsigned int flags;
  struct pike_string *n;	/* modulo */
  struct pike_string *e;	/* public exponent */
  struct pike_string *d;	/* private exponent (if known) */
};

#define THIS ((struct pike_rsa_data *)(fp->current_storage))

/*
 * RSA memory handling glue code.
 */

void T_free(POINTER block)
{
  free(block);
}

POINTER T_malloc(unsigned int len)
{
  return malloc(len);
}

int T_memcmp(POINTER firstBlock, POINTER secondBlock, unsigned int len)
{
  return MEMCMP(firstBlock, secondBlock, len);
}

void T_memcpy(POINTER output, POINTER input, unsigned int len)
{
  MEMCPY(output, input, len);
}

void T_memmove(POINTER output, POINTER input, unsigned int len)
{
  MEMMOVE(output, input, len);
}

void T_memset(POINTER output, int value, unsigned int len)
{
  MEMSET(output, value, len);
}

POINTER T_realloc(POINTER block, unsigned int len)
{
  return realloc(block, len);
}

/*
 * Object init/exit code.
 */

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
  if (THIS->n) {
    free_string(THIS->n);
  }
  if (THIS->e) {
    free_string(THIS->e);
  }
  if (THIS->d) {
    free_string(THIS->d);
  }
  if (THIS->cipher) {
    B_DestroyAlgorithmObject(&(THIS->cipher));
  }
  if (THIS->key) {
    B_DestroyKeyObject(&(THIS->key));
  }
  MEMSET(THIS, 0, sizeof(struct pike_rsa_data));
}

/*
 * Public functions
 */

/* object set_public_key(bignum modulo, bignum pub) */
static void f_set_public_key(INT32 args)
{
  A_RSA_KEY key_info;
  ONERROR tmp;
  struct object *modulo = NULL;
  struct object *pub = NULL;
  int code;

  get_all_args("set_public_key", args, "%o%o", &modulo, &pub);

  if (THIS->n) {
    free_string(THIS->n);
    THIS->n = NULL;
  }
  if (THIS->e) {
    free_string(THIS->e);
    THIS->e = NULL;
  }

  push_int(256);
  apply(modulo, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from modulo->digits().\n");
  }

  if (sp[-1].u.string->len < 12) {
    error("Too small modulo.\n");
  }

  copy_shared_string(THIS->n, sp[-1].u.string);

  pop_stack();

  push_int(256);
  apply(pub, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from pub->digits().\n");
  }

  copy_shared_string(THIS->e, sp[-1].u.string);

  pop_n_elems(args + 1);
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

  if (THIS->d) {
    free_string(THIS->d);
    THIS->d = NULL;
  }

  push_int(256);
  apply(priv, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from priv->digits().\n");
  }

  copy_shared_string(THIS->d, sp[-1].u.string);

  /* FIXME: extra is currently ignored */

  pop_n_elems(args + 1);
  ref_push_object(fp->current_object);
}

/* int query_blocksize() */
static void f_query_blocksize(INT32 args)
{
  if (!THIS->n) {
    error("Public key has not been set.\n");
  }

  pop_n_elems(args);
  push_int(THIS->n->len - 3);
}

/* bignum rsa_pad(string message, int type, mixed|void random) */
static void f_rsa_pad(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* string rsa_unpad(bignum block, int type) */
static void f_rsa_unpad(INT32 args)
{
  struct object *block = NULL;
  INT32 type;
  int i;
  struct pike_string *res;

  get_all_args("rsa_unpad", args, "%o%i", &block, &type);

  push_int(256);
  apply(block, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Unexpected return value from block->digits().\n");
  }

  if (!THIS->n) {
    error("Public key has not been set.\n");
  }

  if (((i = strlen(sp[-1].u.string->str)) < 9) ||
      (sp[-1].u.string->len != THIS->n->len - 1) ||
      (sp[-1].u.string->str[0] != type)) {
    pop_n_elems(args + 1);
    push_int(0);
    return;
  }

  res = make_shared_binary_string(sp[-1].u.string->str + i + 1,
				  sp[-1].u.string->len - i - 1);

  pop_n_elems(args + 1);
  push_string(res);
}

/* object raw_sign(string digest) */
static void f_raw_sign(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* int raw_verify(string digest, object s) */
static void f_raw_verify(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* object sign(string message, program h, mixed|void r) */
static void f_sign(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* int verify(string msg, program h, object sign) */
static void f_verify(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* string sha_sign(string message, mixed|void r) */
static void f_sha_sign(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* int sha_verify(string message, string signature) */
static void f_sha_verify(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* object generate_key(int bits, function|void r) */
static void f_generate_key(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* string encrypt(string s, mixed|void r) */
static void f_encrypt(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* string decrypt(string s) */
static void f_decrypt(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* object set_encrypt_key(array(bignum) key) */
static void f_set_encrypt_key(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* object set_decrypt_key(array(bignum) key) */
static void f_set_decrypt_key(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* string crypt_block(string s) */
static void f_crypt_block(INT32 args)
{
  error("Not yet implemented.\n");
  pop_n_elems(args);
  push_int(0);
}

/* int rsa_size() */
static void f_rsa_size(INT32 args)
{
  if (!THIS->n) {
    error("Public key has not been set.\n");
  }

  pop_n_elems(args);
  push_int(THIS->n->len);
}

/* int public_key_equal (object rsa) */
static void f_public_key_equal(INT32 args)
{
  error("Not yet implemented.\n");
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
	       tFunc(tString tPrg tOr(tMix, tVoid), tObj), 0);

  ADD_FUNCTION("verify", f_verify,
	       tFunc(tString tPrg tObj, tInt), 0);

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
