/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: crypto.c,v 1.56 2003/08/06 23:56:50 nilsson Exp $
*/

/*
 * A pike module for getting access to some common cryptos.
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
#include "interpret.h"
#include "module.h"

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

/* Prototypes */
#include "crypto.h"


#define sp Pike_sp

struct pike_crypto {
  struct object *object;
  ptrdiff_t block_size;
  ptrdiff_t backlog_len;
  unsigned char *backlog;
};

/*
 * Globals
 */

static const char *crypto_functions[] = {
  "query_block_size",
  "query_key_length",
  "set_encrypt_key",
  "set_decrypt_key",
  "crypt_block",
  NULL
};

#undef THIS
#define THIS	((struct pike_crypto *)(Pike_fp->current_storage))

static struct program *pike_crypto_program;

/*
 * Functions
 */

static void init_pike_crypto(struct object *o)
{
  memset(THIS, 0, sizeof(struct pike_crypto));
}

static void exit_pike_crypto(struct object *o)
{
  if (THIS->object) {
    free_object(THIS->object);
  }
  if (THIS->backlog) {
    MEMSET(THIS->backlog, 0, THIS->block_size);
    free(THIS->backlog);
  }
  memset(THIS, 0, sizeof(struct pike_crypto));
}

static void check_functions(struct object *o, const char **requiered)
{
  struct program *p;

  if (!o) {
    Pike_error("Crypto: internal error -- no object\n");
  }
  if (!requiered) {
    return;
  }

  p = o->prog;

  while (*requiered) {
    if (find_identifier( (char *) *requiered, p) < 0) {
      Pike_error("Crypto: Object is missing identifier \"%s\"\n",
	    *requiered);
    }
    requiered++;
  }
}

void assert_is_crypto_module(struct object *o)
{
  check_functions(o, crypto_functions);
}

/*
 * efuns and the like
 */

/*! @module Crypto
 */

/*! @decl string string_to_hex(string data)
 *!
 *! Convert a string of binary data to a hexadecimal string.
 *!
 *! @seealso
 *!   @[hex_to_string()]
 */
static void f_string_to_hex(INT32 args)
{
  struct pike_string *s;
  INT32 i;

  if (args != 1) {
    Pike_error("Wrong number of arguments to string_to_hex()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to string_to_hex()\n");
  }
  if (sp[-1].u.string->size_shift) {
    Pike_error("Bad argument 1 to string_to_hex(), expected 8-bit string.\n");
  }

  s = begin_shared_string(2 * sp[-1].u.string->len);
  
  for (i=0; i<sp[-1].u.string->len; i++) {
    sprintf(s->str + i*2, "%02x", sp[-1].u.string->str[i] & 0xff);
  }
  
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*! @decl string hex_to_string(string hex)
 *!
 *! Convert a string of hexadecimal digits to binary data.
 *!
 *! @seealso
 *!   @[string_to_hex()]
 */
static void f_hex_to_string(INT32 args)
{
  struct pike_string *s;
  INT32 i;

  if (args != 1) {
    Pike_error("Wrong number of arguments to hex_to_string()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to hex_to_string()\n");
  }
  if (sp[-1].u.string->len & 1) {
    Pike_error("Bad string length to hex_to_string()\n");
  }

  s = begin_shared_string(sp[-1].u.string->len/2);
  for (i=0; i*2<sp[-1].u.string->len; i++)
  {
    switch (sp[-1].u.string->str[i*2])
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      s->str[i] = (sp[-1].u.string->str[i*2] - '0')<<4;
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      s->str[i] = (sp[-1].u.string->str[i*2] + 10 - 'A')<<4;
      break;
    default:
      free_string(end_shared_string(s));
      Pike_error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2] & 0xff);
    }
    switch (sp[-1].u.string->str[i*2+1])
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      s->str[i] |= sp[-1].u.string->str[i*2+1] - '0';
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      s->str[i] |= (sp[-1].u.string->str[i*2+1] + 10 - 'A') & 0x0f;
      break;
    default:
      free_string(end_shared_string(s));
      Pike_error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2+1] & 0xff);
    }
  }
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

static INLINE unsigned INT8 parity(unsigned INT8 c)
{
  c ^= (c >> 4);
  c ^= (c >> 2);
  c ^= (c >> 1);
  return c & 1;
}

/*! @decl string des_parity(string raw)
 *!
 *! Adjust the parity for a string so that it is acceptable
 *! according to DES.
 */
static void f_des_parity(INT32 args)
{
  struct pike_string *s;
  int i;
  if (args != 1) {
    Pike_error("Wrong number of arguments to des_parity()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to des_parity()\n");
  }

  s = begin_shared_string(sp[-1].u.string->len);
  MEMCPY(s->str, sp[-1].u.string->str, s->len);

  /* FIXME: Shouldn't it be the MSB that gets inverted? */
  for (i=0; i< s->len; i++)
    s->str[i] ^= ! parity(s->str[i]);
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*! @decl string crypt_md5(string password)
 *! @decl string crypt_md5(string password, string salt)
 *!
 *! This function crypts a password with an algorithm based on MD5 hashing.
 *! 
 *! If @[salt] is left out, an 8 character long salt (max length) will 
 *! be randomized.
 *!
 *! Verification can be done by supplying the crypted password as @[salt]:
 *! @code
 *! crypt_md5(typed_pw, crypted_pw) == crypted_pw;
 *! @endcode
 *! 
 *! @seealso
 *!   @[crypt()]
 */
static void f_crypt_md5(INT32 args)
{
  char salt[8];
  char *ret, *saltp ="";
  char *choice =
    "cbhisjKlm4k65p7qrJfLMNQOPxwzyAaBDFgnoWXYCZ0123tvdHueEGISRTUV89./";
 
  if (args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("crypt_md5", 1);

  if (Pike_sp[-args].type != T_STRING)
    SIMPLE_BAD_ARG_ERROR("crypt_md5", 1, "string");

  if (args > 1)
  {
    if (Pike_sp[1-args].type != T_STRING)
      SIMPLE_BAD_ARG_ERROR("crypt_md5", 2, "string");

    saltp = Pike_sp[1-args].u.string->str;
  } else {
    unsigned int i, r;
    for (i = 0; i < sizeof(salt); i++) 
    {
      r = my_rand();
      salt[i] = choice[r % (size_t) strlen(choice)];
    }
    saltp = salt;
  }

  ret = (char *)crypt_md5(Pike_sp[-args].u.string->str, saltp);

  pop_n_elems(args);
  push_string(make_shared_string(ret));
}

/*
 * Crypto
 */

/*! @class crypto
 *!
 *! Buffer a block-crypto.
 */

/*! @decl void create(object cipher)
 *! @decl void create(program prog, mixed ... args)
 *!
 *! Initialize a block-crypto buffer.
 */
static void f_create(INT32 args)
{
  if (args < 1) {
    Pike_error("Too few arguments to crypto->create()\n");
  }
  if ((sp[-args].type != T_PROGRAM) &&
      (sp[-args].type != T_OBJECT)) {
    Pike_error("Bad argument 1 to crypto->create()\n");
  }
  if (sp[-args].type == T_PROGRAM) {
    THIS->object = clone_object(sp[-args].u.program, args-1);
  } else {
    if (args != 1) {
      Pike_error("Too many arguments to crypto->create()\n");
    }
    add_ref(THIS->object = sp[-args].u.object);
  }
  pop_stack(); /* Just one element left on the stack in both cases */

  check_functions(THIS->object, crypto_functions);

  safe_apply(THIS->object, "query_block_size", 0);

  if (sp[-1].type != T_INT) {
    Pike_error("crypto->create(): query_block_size() didn't return an int\n");
  }
  THIS->block_size = sp[-1].u.integer;

  pop_stack();

  if ((!THIS->block_size) ||
      (THIS->block_size > 4096)) {
    Pike_error("crypto->create(): Bad block size %ld\n",
	  DO_NOT_WARN((long)THIS->block_size));
  }

  THIS->backlog = (unsigned char *)xalloc(THIS->block_size);
  THIS->backlog_len = 0;
  MEMSET(THIS->backlog, 0, THIS->block_size);
}

/*! @decl int query_block_size()
 *!
 *! Get the block size of the contained block-crypto.
 */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(DO_NOT_WARN(THIS->block_size));
}

/*! @decl int query_key_length()
 */
static void f_query_key_length(INT32 args)
{
  safe_apply(THIS->object, "query_key_length", args);
}

/*! @decl void set_encrypt_key(string key)
 *!
 *! Set the encryption key.
 *!
 *! @note
 *!   As a side-effect any buffered data will be cleared.
 */
static void f_set_encrypt_key(INT32 args)
{
  if (THIS->block_size) {
    MEMSET(THIS->backlog, 0, THIS->block_size);
    THIS->backlog_len = 0;
  } else {
    Pike_error("crypto->set_encrypt_key(): Object has not been created yet\n");
  }
  safe_apply(THIS->object, "set_encrypt_key", args);
  pop_stack();
  push_object(this_object());
}

/*! @decl void set_decrypt_key(string key)
 *!
 *! Set the decryption key.
 *!
 *! @note
 *!   As a side-effect any buffered data will be cleared.
 */
static void f_set_decrypt_key(INT32 args)
{
  if (THIS->block_size) {
    MEMSET(THIS->backlog, 0, THIS->block_size);
    THIS->backlog_len = 0;
  } else {
    Pike_error("crypto->set_decrypt_key(): Object has not been created yet\n");
  }
  safe_apply(THIS->object, "set_decrypt_key", args);
  pop_stack();
  push_object(this_object());
}

/*! @decl string crypt(string data)
 *!
 *! Encrypt some data.
 *!
 *! Adds data to be encrypted to the buffer. If there's enough
 *! data to en/decrypt a block, that will be done, and the result
 *! returned. Any uncrypted data will be left in the buffer.
 */
static void f_crypto_crypt(INT32 args)
{
  unsigned char *result;
  ptrdiff_t roffset = 0;
  ptrdiff_t soffset = 0;
  ptrdiff_t len;

  if (args != 1) {
    Pike_error("Wrong number of arguments to crypto->crypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    Pike_error("Bad argument 1 to crypto->crypt()\n");
  }
  if (!(result = alloca(sp[-1].u.string->len + THIS->block_size))) {
    Pike_error("crypto->crypt(): Out of memory\n");
  }
  if (THIS->backlog_len) {
    if (sp[-1].u.string->len >=
	(THIS->block_size - THIS->backlog_len)) {
      MEMCPY(THIS->backlog + THIS->backlog_len,
	     sp[-1].u.string->str,
	     (THIS->block_size - THIS->backlog_len));
      soffset += (THIS->block_size - THIS->backlog_len);
      THIS->backlog_len = 0;
      push_string(make_shared_binary_string((char *)THIS->backlog,
					    THIS->block_size));
      safe_apply(THIS->object, "crypt_block", 1);
      if (sp[-1].type != T_STRING) {
	Pike_error("crypto->crypt(): crypt_block() did not return string\n");
      }
      if (sp[-1].u.string->len != THIS->block_size) {
	Pike_error("crypto->crypt(): Unexpected string length %ld\n",
	      DO_NOT_WARN((long)sp[-1].u.string->len));
      }
	
      MEMCPY(result, sp[-1].u.string->str, THIS->block_size);
      roffset = THIS->block_size;
      pop_stack();
      MEMSET(THIS->backlog, 0, THIS->block_size);
    } else {
      MEMCPY(THIS->backlog + THIS->backlog_len,
	     sp[-1].u.string->str, sp[-1].u.string->len);
      THIS->backlog_len += sp[-1].u.string->len;
      pop_n_elems(args);
      push_string(make_shared_binary_string("", 0));
      return;
    }
  }
  
  len = (sp[-1].u.string->len - soffset);
  len -= len % THIS->block_size;

  if (len) {
    push_string(make_shared_binary_string(sp[-1].u.string->str + soffset, len));
    soffset += len;

    safe_apply(THIS->object, "crypt_block", 1);

    if (sp[-1].type != T_STRING) {
      Pike_error("crypto->crypt(): crypt_block() did not return string\n");
    }
    if (sp[-1].u.string->len != len) {
      Pike_error("crypto->crypt(): Unexpected string length %ld\n",
	    DO_NOT_WARN((long)sp[-1].u.string->len));
    }
	
    MEMCPY(result + roffset, sp[-1].u.string->str, len);

    pop_stack();
  }

  if (soffset < sp[-1].u.string->len) {
    MEMCPY(THIS->backlog, sp[-1].u.string->str + soffset,
	   sp[-1].u.string->len - soffset);
    THIS->backlog_len = sp[-1].u.string->len - soffset;
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((char *)result, roffset + len));
  MEMSET(result, 0, roffset + len);
}

/*! @decl string pad()
 *!
 *! Pad and de/encrypt any data left in the buffer.
 *!
 *! @seealso
 *!   @[unpad()]
 */
static void f_pad(INT32 args)
{
  ptrdiff_t i;
  
  if (args) {
    Pike_error("Too many arguments to crypto->pad()\n");
  }

  for (i = THIS->backlog_len; i < THIS->block_size - 1; i++) 
    THIS->backlog[i] = DO_NOT_WARN((unsigned char)(my_rand() & 0xff));
  
  THIS->backlog[THIS->block_size - 1] =
    DO_NOT_WARN((unsigned char)(7 - THIS->backlog_len));

  push_string(make_shared_binary_string((const char *)THIS->backlog,
					THIS->block_size));

  MEMSET(THIS->backlog, 0, THIS->block_size);
  THIS->backlog_len = 0;

  safe_apply(THIS->object, "crypt_block", 1);
}

/*! @decl string unpad(string data)
 *!
 *! De/encrypt and unpad a block of data.
 *!
 *! This performs the reverse operation of @[pad()].
 *!
 *! @seealso
 *!   @[pad()]
 */
static void f_unpad(INT32 args)
{
  ptrdiff_t len;
  struct pike_string *str;

  if (args != 1) 
    Pike_error("Wrong number of arguments to crypto->unpad()\n");
  
  if (sp[-1].type != T_STRING) 
    Pike_error("Bad argument 1 to crypto->unpad()\n");
  
  str = sp[-1].u.string;
  len = str->len;

  if (str->str[len - 1] > (THIS->block_size - 1))
    Pike_error("crypto->unpad(): Invalid padding\n");

  len -= (str->str[len - 1] + 1);

  if (len < 0) 
    Pike_error("crypto->unpad(): String to short to unpad\n");
  
  add_ref(str);
  pop_stack();
  push_string(make_shared_binary_string(str->str, len));
  free_string(str);
}

/*! @endclass
 */

/*! @endmodule
 */

/*
 * Module linkage
 */


void pike_crypto_init(void)
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
  ADD_STORAGE(struct pike_crypto);

  /* function(program|object:void) */
  ADD_FUNCTION("create", f_create, tFunc(tOr(tPrg(tObj), tObj), tVoid), 0);

  /* function(void:int) */
  ADD_FUNCTION("query_block_size", f_query_block_size, tFunc(tNone, tInt), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length, tFunc(tNone, tInt), 0);

  /* function(string:object) */
  ADD_FUNCTION("set_encrypt_key", f_set_encrypt_key, tFunc(tStr, tObj), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_decrypt_key", f_set_decrypt_key, tFunc(tStr, tObj), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt", f_crypto_crypt, tFunc(tStr, tStr), 0);

  /* function(void:string) */
  ADD_FUNCTION("pad", f_pad, tFunc(tVoid, tStr), 0);
  /* function(string:string) */
  ADD_FUNCTION("unpad", f_unpad, tFunc(tStr, tStr), 0);

  set_init_callback(init_pike_crypto);
  set_exit_callback(exit_pike_crypto);

  end_class("crypto", 0);
}

void pike_crypto_exit(void) {}

PIKE_MODULE_INIT
{
  /* function(string:string) */
  ADD_FUNCTION("string_to_hex", f_string_to_hex, tFunc(tStr, tStr), 0);
  /* function(string:string) */
  ADD_FUNCTION("hex_to_string", f_hex_to_string, tFunc(tStr, tStr), 0);
  /* function(string:string) */
  ADD_FUNCTION("des_parity", f_des_parity, tFunc(tStr, tStr), 0);
  /* function(string:string)|function(string,string:string) */
  ADD_FUNCTION("crypt_md5", f_crypt_md5,
	       tOr(tFunc(tStr,tStr), tFunc(tStr tStr,tStr)), 0);

  pike_md2_init();
  pike_md4_init();
  pike_md5_init();
  pike_sha_init();
  pike_crypto_init();
  pike_cbc_init();
  pike_pipe_init();
  pike_nt_init();

  pike_idea_init();
  pike_des_init();
  pike_cast_init();
  pike_arcfour_init();
  pike_rijndael_init();
}

PIKE_MODULE_EXIT
{
  pike_md2_exit();
  pike_md4_exit();
  pike_md5_exit();
  pike_sha_exit();
  pike_crypto_exit();
  pike_cbc_exit();
  pike_pipe_exit();

  pike_idea_exit();
  pike_des_exit();
  pike_cast_exit();
  pike_arcfour_exit();
  pike_rijndael_exit();
}
