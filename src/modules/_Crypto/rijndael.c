/*
 * $Id: rijndael.c,v 1.2 2000/10/02 20:06:45 grubba Exp $
 *
 * A pike module for getting access to some common cryptos.
 *
 * Henrik Grubbström 2000-10-02
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
#include "module_support.h"
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

#include "rijndael.h"

/* THIS MUST BE INCLUDED LAST */
#include "module_magic.h"

struct pike_crypto_rijndael {
  int rounds;
  word8 keySchedule[MAXROUNDS+1][4][4];
  int (*crypt_fun)(word8 a[16], word8 b[16],
		   word8 rk[MAXROUNDS+1][4][4], int rounds);
};

#undef THIS
#define THIS ((struct pike_crypto_rijndael *) Pike_fp->current_storage)

/*
 * Globals
 */

struct program *pike_crypto_rijndael_program;

/*
 * Functions
 */

static void init_pike_crypto_rijndael(struct object *o)
{
  memset(THIS, 0, sizeof(struct pike_crypto_rijndael));
}

static void exit_pike_crypto_rijndael(struct object *o)
{
  memset(THIS, 0, sizeof(struct pike_crypto_rijndael));
}

/*
 * efuns and the like
 */

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(RIJNDAEL_BLOCK_SIZE);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  pop_n_elems(args);
  push_int(32);
}

/* void set_encrypt_key */
static void f_set_encrypt_key(INT32 args)
{
  struct pike_string *key = NULL;
  word8 k[MAXKC][4];

  get_all_args("rijndael->set_encrypt_key()", args, "%S", &key);
  if (((key->len - 8) & ~0x18) || (!key->len)) {
    error("rijndael->set_encrypt_key(): Bad key length "
	  "(must be 16, 24 or 32).\n");
  }
  MEMCPY(k, key->str, key->len);
  THIS->rounds = 6 + key->len/32;
  rijndaelKeySched(k, THIS->keySchedule, THIS->rounds);
  THIS->crypt_fun = rijndaelEncrypt;
}

/* void set_decrypt_key */
static void f_set_decrypt_key(INT32 args)
{
  struct pike_string *key = NULL;
  word8 k[MAXKC][4];

  get_all_args("rijndael->set_encrypt_key()", args, "%S", &key);
  if (((key->len - 8) & ~0x18) || (key->len != 8)) {
    error("rijndael->set_encrypt_key(): Bad key length "
	  "(must be 16, 24 or 32).\n");
  }
  MEMCPY(k, key->str, key->len);
  THIS->rounds = 6 + key->len/32;
  rijndaelKeySched(k, THIS->keySchedule, THIS->rounds);
  rijndaelKeyEncToDec(THIS->keySchedule, THIS->rounds);
  THIS->crypt_fun = rijndaelDecrypt;
}

/* string encrypt(string) */
static void f_crypt_block(INT32 args)
{
  size_t len;
  struct pike_string *s;
  size_t i;
  
  if (args != 1) {
    error("Wrong number of arguments to rijndael->crypt_block()\n");
  }
  if (!THIS->crypt_fun)
    error("rijndael->crypt_block: must set key first\n");
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to rijndael->crypt_block()\n");
  }
  if ((len = sp[-1].u.string->len) % RIJNDAEL_BLOCK_SIZE) {
    error("Bad string length in rijndael->crypt_block()\n");
  }
  s = begin_shared_string(len);
  for(i = 0; i < len; i += RIJNDAEL_BLOCK_SIZE)
    THIS->crypt_fun((unsigned INT8 *) sp[-1].u.string->str + i,
		    (unsigned INT8 *) s->str + i,
		    THIS->keySchedule, THIS->rounds);

  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/*
 * Module linkage
 */

void pike_rijndael_init(void)
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

  /* /precompiled/crypto/rijndael */
  start_new_program();
  ADD_STORAGE(struct pike_crypto_rijndael);

  /* function(void:int) */
  ADD_FUNCTION("query_block_size", f_query_block_size,tFunc(tVoid,tInt), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length,tFunc(tVoid,tInt), 0);

  /* function(string:object) */
  ADD_FUNCTION("set_encrypt_key", f_set_encrypt_key,tFunc(tStr,tObj), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_decrypt_key", f_set_decrypt_key,tFunc(tStr,tObj), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt_block", f_crypt_block,tFunc(tStr,tStr), 0);
  set_init_callback(init_pike_crypto_rijndael);
  set_exit_callback(exit_pike_crypto_rijndael);

  end_class("rijndael", 0);
}

void pike_rijndael_exit(void)
{
}
