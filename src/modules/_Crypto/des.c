/*
 * $Id: des.c,v 1.8 1997/05/30 02:40:14 grubba Exp $
 *
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <des.h>

/* Module specific includes */
#include "precompiled_crypto.h"

struct pike_crypto_des {
  unsigned INT32 method[DES_EXPANDED_KEYLEN];
  void (*crypt_fun)(unsigned INT8 *dest,
		    unsigned INT32 *method, unsigned INT8 *src);
};

#define THIS ((struct pike_crypto_des *) fp->current_storage)

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

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(DES_BLOCKSIZE);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  pop_n_elems(args);
  push_int(DES_KEYSIZE);
}

static void set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of arguments to des->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->set_key()\n");
  }
  if (sp[-1].u.string->len != 8)
    error("Invalid key length to des->set_key()\n");
  switch (DesMethod(THIS->method, (unsigned INT8 *)sp[-1].u.string->str))
    {
    case -1:
      error("des->set_key: parity error\n");
      break;
    case -2:
      error("des->set_key: key is weak!\n");
      break;
    case 0:
      break;
    default:
      error("des->set_key: invalid return value from desMethod, can't happen\n");
    }
  pop_n_elems(args);
  push_object(this_object());
}

/* void set_encrypt_key */
static void f_set_encrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = DesSmallFipsEncrypt;
}

/* void set_decrypt_key */
static void f_set_decrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = DesSmallFipsDecrypt;
}

/* string encrypt(string) */
static void f_crypt_block(INT32 args)
{
  unsigned len;
  struct pike_string *s;
  unsigned INT32 i;
  
  if (args != 1) {
    error("Wrong number of arguments to des->crypt_block()\n");
  }
  if (!THIS->crypt_fun)
    error("des->crypt_block: must set key first\n");
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->crypt_block()\n");
  }
  if ((len = sp[-1].u.string->len) % DES_BLOCKSIZE) {
    error("Bad string length in des->crypt_block()\n");
  }
  s = begin_shared_string(len);
  for(i = 0; i < len; i += DES_BLOCKSIZE)
    THIS->crypt_fun((unsigned INT8 *) s->str + i,
		    THIS->method,
		    (unsigned INT8 *) sp[-1].u.string->str + i);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*
 * Module linkage
 */

#if 0
void MOD_INIT2(des)(void)
{
  /* add_efun()s */
}
#endif

void MOD_INIT(des)(void)
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
  add_storage(sizeof(struct pike_crypto_des));

  add_function("query_block_size", f_query_block_size, "function(void:int)", 0);
  add_function("query_key_length", f_query_key_length, "function(void:int)", 0);

  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:object)", 0);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:object)", 0);
  add_function("crypt_block", f_crypt_block, "function(string:string)", 0);
  set_init_callback(init_pike_crypto_des);
  set_exit_callback(exit_pike_crypto_des);

  end_class(MODULE_PREFIX "des", 0);
}

void MOD_EXIT(des)(void)
{
}
