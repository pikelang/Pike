/*
 * $Id: rsa.c,v 1.13 2000/05/03 20:19:25 grubba Exp $
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
#include "port.h"

#include "module_support.h"

#include "crypto.h"

#ifdef HAVE_LIBCRYPTOCI
#ifndef HAVE_RSA_LIB
#define HAVE_RSA_LIB
#endif /* !HAVE_RSA_LIB */
#endif /* HAVE_LIBCRYPTOCI */

#define PIKE_RSA_DEBUG

#ifdef HAVE_RSA_LIB

#include <bsafe.h>

RCSID("$Id: rsa.c,v 1.13 2000/05/03 20:19:25 grubba Exp $");

struct pike_rsa_data
{
  B_ALGORITHM_OBJ cipher;
  B_KEY_OBJ public_key;
  B_KEY_OBJ private_key;
  struct pike_string *n;	/* modulo */
  struct pike_string *e;	/* exponent. Needed for comparison. */
};

/* Flags */
#define P_RSA_PUBLIC_KEY_NOT_SET	1
#define P_RSA_PRIVATE_KEY_NOT_SET	2
#define P_RSA_KEY_NOT_SET	3

#define THIS ((struct pike_rsa_data *)(fp->current_storage))

static struct program *pike_rsa_program = NULL;

static B_ALGORITHM_METHOD *rsa_chooser[] =
{
  &AM_RSA_ENCRYPT, &AM_RSA_DECRYPT, NULL
};

#ifdef PIKE_RSA_DEBUG

/*
 * Debug code.
 */

static void low_dump_string(char *name, unsigned char *buffer, int len)
{
  int i;
  fprintf(stderr, "%s:\n", name);
  for(i=0; i < len; i+=16) {
    int j;
    fprintf(stderr, "0x%04x: ", i);
    for(j=0; j < 16; j++) {
      if (i+j < len) {
	fprintf(stderr, "\\x%02x", buffer[i+j]);
      }
    }
    fprintf(stderr, "\n");
  }
}

#endif /* PIKE_RSA_DEBUG */

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

  if ((code = B_CreateAlgorithmObject(&(THIS->cipher)))) {
    error("Crypto.rsa(): Failed to create cipher object: %04x\n", code);
  }
  if ((code = B_SetAlgorithmInfo(THIS->cipher, AI_RSAPublic, NULL_PTR))) {
    error("Crypto.rsa(): Failed to initialize RSA algorithm: %04x\n", code);
  }
}

static void exit_pike_rsa(struct object *o)
{
  if (THIS->e) {
    free_string(THIS->e);
  }
  if (THIS->n) {
    free_string(THIS->n);
  }
  if (THIS->cipher) {
    B_DestroyAlgorithmObject(&(THIS->cipher));
  }
  if (THIS->private_key) {
    B_DestroyKeyObject(&(THIS->private_key));
  }
  if (THIS->public_key) {
    B_DestroyKeyObject(&(THIS->public_key));
  }
  MEMSET(THIS, 0, sizeof(struct pike_rsa_data));
}

/*
 * Public functions
 */

/* object set_public_key(bignum modulo, bignum pub) */
static void f_set_public_key(INT32 args)
{
  A_RSA_KEY rsa_public_key;
  struct object *modulo = NULL;
  struct object *pub = NULL;
  int code;

  get_all_args("set_public_key", args, "%o%o", &modulo, &pub);

  if (THIS->n) {
    free_string(THIS->n);
    THIS->n = NULL;
  }

  if (THIS->private_key) {
    B_DestroyKeyObject(&(THIS->private_key));
    THIS->private_key = NULL;
  }

  push_int(256);
  apply(modulo, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Crypto.rsa.set_public_key(): "
	  "Unexpected return value from modulo->digits().\n");
  }

  if (sp[-1].u.string->len < 12) {
    error("Crypto.rsa.set_public_key(): Too small modulo.\n");
  }

  /* We need to remember the modulus for the private key. */
  copy_shared_string(THIS->n, sp[-1].u.string);

  pop_stack();

  push_int(256);
  apply(pub, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Crypto.rsa.set_public_key(): "
	  "Unexpected return value from pub->digits().\n");
  }

  if ((code = B_CreateKeyObject(&(THIS->public_key)))) {
    error("Crypto.rsa.set_public_key(): "
	  "Failed to create public key object: %04x\n", code);
  }

  rsa_public_key.modulus.data = (POINTER)THIS->n->str;
  rsa_public_key.modulus.len = THIS->n->len;
  rsa_public_key.exponent.data = (POINTER)sp[-1].u.string->str;
  rsa_public_key.exponent.len = sp[-1].u.string->len;

  if ((code = B_SetKeyInfo(THIS->public_key, KI_RSAPublic,
			   (POINTER)&rsa_public_key))) {
    error("Crypto.rsa.set_public_key(): "
	  "Failed to set public key: %04x\n", code);
  }

  copy_shared_string(THIS->e, sp[-1].u.string);

  pop_n_elems(args + 1);
  ref_push_object(fp->current_object);
}

/* object set_private_key(bignum priv, array(bignum)|void extra) */
static void f_set_private_key(INT32 args)
{
  A_RSA_KEY rsa_private_key;
  struct object *priv = NULL;
  struct array *extra = NULL;
  int code;

  get_all_args("set_private_key", args, "%o%a", &priv, &extra);

  if (THIS->private_key) {
    B_DestroyKeyObject(&(THIS->private_key));
    THIS->private_key = NULL;
  }

  if (!THIS->n) {
    error("Crypto.rsa.set_private_key(): Public key hasn't been set.\n");
  }

  push_int(256);
  apply(priv, "digits", 1);

  if ((sp[-1].type != T_STRING) || (!sp[-1].u.string) ||
      (sp[-1].u.string->size_shift)) {
    error("Crypto.rsa.set_private_key(): "
	  "Unexpected return value from priv->digits().\n");
  }

  if ((code = B_CreateKeyObject(&(THIS->private_key)))) {
    error("Crypto.rsa.set_private_key(): "
	  "Failed to create private key object: %04x\n", code);
  }

  rsa_private_key.modulus.data = (POINTER)THIS->n->str;
  rsa_private_key.modulus.len = THIS->n->len;
  rsa_private_key.exponent.data = (POINTER)sp[-1].u.string->str;
  rsa_private_key.exponent.len = sp[-1].u.string->len;

  if ((code = B_SetKeyInfo(THIS->private_key, KI_RSAPublic,
			   (POINTER)&rsa_private_key))) {
    error("Crypto.rsa.set_private_key(): "
	  "Failed to set private key: %04x\n", code);
  }

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
  struct pike_string *s = NULL;
  int err;
  unsigned char *buffer;
  unsigned int len;
  unsigned int flen;
  unsigned int i;

  if (!THIS->private_key) {
    error("Private key has not been set.\n");
  }

  get_all_args("encrypt", args, "%S", &s);

  if ((err = B_EncryptInit(THIS->cipher, THIS->public_key, rsa_chooser,
			   (A_SURRENDER_CTX *)NULL_PTR))) {
    error("Failed to initialize encrypter: %04x\n", err);
  }

  /* rsa_pad(s, 2, r) inlined. */
  len = THIS->n->len - 3 - s->len;
  if (len < 8) {
    error("Too large block.\n");
  }
  
  buffer = (unsigned char *)xalloc(THIS->n->len+1);
  buffer[THIS->n->len] = 0;	/* Ensure NUL-termination. */

  buffer[0] = 0;	/* Ensure that it is less than n. */
  buffer[1] = 2;	/* Padding type 2. */

  if ((args > 1) && (sp[1-args].type != T_INT)) {
    struct svalue *r = sp + 1 - args;
    ONERROR tmp;

    SET_ONERROR(tmp, free, buffer);
    push_int(len);
    apply_svalue(r, 1);
    if ((sp[-1].type != T_STRING) || (sp[-1].u.string->size_shift) ||
	((unsigned int)sp[-1].u.string->len != len)) {
      error("Unexpected return value from the random function\n");
    }      
    MEMCPY(buffer + 2, sp[-1].u.string->str, len);
    pop_stack();
    UNSET_ONERROR(tmp);

    /* Ensure no NUL's. */
    for(i=0; i < len; i++) {
      if (!buffer[i+2]) {
	buffer[i+2] = 1;
      }
    }
  } else {
    for (i=0; i < len; i++) {
      buffer[i+2] = 1 + (my_rand() % 255);
    }
  }

  buffer[len+2] = 0;	/* End of pad marker. */

  MEMCPY(buffer + len + 3, s->str, s->len);

  /* End of rsa_pad */

  s = begin_shared_string(THIS->n->len);

  len = 0;
  if ((err = B_EncryptUpdate(THIS->cipher, (POINTER)s->str, &len, THIS->n->len,
			     buffer, THIS->n->len,
			     (B_ALGORITHM_OBJ)NULL_PTR,
			     (A_SURRENDER_CTX *)NULL_PTR))) {
    free_string(s);
    free(buffer);
    error("Encrypt failed: %04x\n", err);
  }

#ifdef PIKE_RSA_DEBUG
  fprintf(stderr, "RSA: Encrypt len: %d\n", len);
#endif /* PIKE_RSA_DEBUG */

  free(buffer);

  flen = 0;
  if ((err = B_EncryptFinal(THIS->cipher, (POINTER)s->str + len, &flen,
			    THIS->n->len - len,
			    (B_ALGORITHM_OBJ)NULL_PTR,
			    (A_SURRENDER_CTX *)NULL_PTR))) {
    free_string(s);
    error("Encrypt failed: %04x\n", err);
  }

#ifdef PIKE_RSA_DEBUG
  fprintf(stderr, "RSA: Encrypt flen: %d\n", flen);
#endif /* PIKE_RSA_DEBUG */

  len += flen;

#if defined(PIKE_RSA_DEBUG) || defined(PIKE_DEBUG)
  if (len != (unsigned int)THIS->n->len) {
    free_string(s);
    error("Encrypted string has bad length. Expected %d. Got %d\n",
	  THIS->n->len, len);
  }
#endif /* PIKE_RSA_DEBUG || PIKE_DEBUG */

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/* string decrypt(string s) */
static void f_decrypt(INT32 args)
{
  struct pike_string *s = NULL;
  int err;
  unsigned char *buffer;
  unsigned int len;
  unsigned int flen;
  unsigned int i;

  if (!THIS->private_key) {
    error("Private key has not been set.\n");
  }

  get_all_args("decrypt", args, "%S", &s);
  
  if ((err = B_DecryptInit(THIS->cipher, THIS->private_key, rsa_chooser,
			   (A_SURRENDER_CTX *)NULL_PTR))) {
    error("Failed to initialize decrypter: %04x\n", err);
  }

  buffer = (unsigned char *)xalloc(s->len+1);
  buffer[s->len] = 0;	/* Ensure NUL-termination. */

  len = 0;
  if ((err = B_DecryptUpdate(THIS->cipher, buffer, &len, s->len,
			     (POINTER)s->str, s->len,
			     (B_ALGORITHM_OBJ)NULL_PTR,
			     (A_SURRENDER_CTX *)NULL_PTR))) {
    free(buffer);
    error("decrypt failed: %04x\n", err);
  }

#ifdef PIKE_RSA_DEBUG
  fprintf(stderr, "RSA: Decrypt len: %d\n", len);
#endif /* PIKE_RSA_DEBUG */

  flen = 0;
  if ((err = B_DecryptFinal(THIS->cipher, buffer + len, &flen, s->len - len,
			    (B_ALGORITHM_OBJ)NULL_PTR,
			    (A_SURRENDER_CTX *)NULL_PTR))) {
    free(buffer);
    error("Decrypt failed: %04x\n", err);
  }

#ifdef PIKE_RSA_DEBUG
  fprintf(stderr, "RSA: Decrypt flen: %d\n", flen);
#endif /* PIKE_RSA_DEBUG */

  len += flen;

  /* Inlined rsa_unpad(s, 2). */

  /* Skip any initial zeros. Note that the buffer is aligned to the right. */
  i = 0;
  while (!buffer[i] && (i < len)) {
    i++;
  }

  /* FIXME: Enforce i being 1? */
  if ((buffer[i] != 2) ||
      ((i += strlen((char *)buffer + i)) < 9) ||
      (len != (unsigned int)THIS->n->len)) {
#ifdef PIKE_RSA_DEBUG
    fprintf(stderr, "Decrypt failed: i:%d, len:%d, n->len:%d, buffer[0]:%d\n",
	    i, len, THIS->n->len, buffer[0]);
    low_dump_string("s", s->str, s->len);
    low_dump_string("buffer", buffer, s->len+1);
    low_dump_string("n", THIS->n->str, THIS->n->len);
    low_dump_string("e", THIS->e->str, THIS->e->len);
#endif /* PIKE_RSA_DEBUG */
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);
  push_string(make_shared_binary_string((char *)buffer + i + 1, len - i - 1));

  free(buffer);
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
  push_int(THIS->n->len*8);
}

/* int public_key_equal (object rsa) */
static void f_public_key_equal(INT32 args)
{
  struct object *o = NULL;
  struct pike_rsa_data *other = NULL;

  get_all_args("public_key_equal", args, "%o", &o);

  if ((other = (struct pike_rsa_data *)get_storage(o, pike_rsa_program))) {
    if ((other->n == THIS->n) && (other->e == THIS->e)) {
      pop_n_elems(args);
      push_int(1);
      return;
    }
  }

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
  
  pike_rsa_program = end_program();
  add_program_constant("rsa", pike_rsa_program, 0);
#endif /* HAVE_RSA_LIB */
}

void pike_rsa_exit(void)
{
#ifdef HAVE_RSA_LIB
  free_program(pike_rsa_program);
#endif /* HAVE_RSA_LIB */
}
