/*
 * $Id: cast.c,v 1.2 1997/11/16 22:25:39 nisse Exp $
 *
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
#include "error.h"
#include "las.h"

#include <cast.h>

struct pike_crypto_cast {
  struct cast_key key;
  void (*crypt_fun)(struct cast_key* key, unsigned INT8* inblock,
		    unsigned INT8* outblock);
};

#define THIS ((struct pike_crypto_cast *)(fp->current_storage))
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
 * methods
 */

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) {
    error("Too many arguments to cast->name()\n");
  }
  push_string(make_shared_string("CAST"));
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  if (args) {
    error("Too many arguments to cast->query_block_size()\n");
  }
  push_int(CAST_BLOCKSIZE);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  if (args) {
    error("Too many arguments to cast->query_key_length()\n");
  }
  push_int(CAST_MAX_KEYSIZE);
}

static void set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of arguments to des->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->set_key()\n");
  }
  if ( (sp[-1].u.string->len < CAST_MIN_KEYSIZE)
       || (sp[-1].u.string->len > CAST_MAX_KEYSIZE))
    error("Invalid key length to cast->set_key()\n");

  cast_setkey(&(THIS->key), sp[-1].u.string->str, sp[-1].u.string->len);
  
  pop_n_elems(args);
  push_object(this_object());
}

/* void set_encrypt_key */
static void f_set_encrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = cast_encrypt;
}

/* void set_decrypt_key */
static void f_set_decrypt_key(INT32 args)
{
  set_key(args);
  THIS->crypt_fun = cast_decrypt;
}

/* string crypt_block(string) */
static void f_crypt_block(INT32 args)
{
  int len;
  struct pike_string *s;
  INT32 i;
  
  if (args != 1) {
    error("Wrong number of arguments to cast->crypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to cast->crypt()\n");
  }

  len = sp[-1].u.string->len;
  if (len % CAST_BLOCKSIZE) {
    error("Bad length of argument 1 to cast->crypt()\n");
  }

  if (!THIS->key.rounds)
    error("Crypto.cast->crypt_block: Key not set\n");
  s = begin_shared_string(len);
  for(i = 0; i < len; i += CAST_BLOCKSIZE)
    THIS->crypt_fun(&(THIS->key), 
		    (unsigned INT8 *) sp[-1].u.string->str + i,
		    (unsigned INT8 *) s->str + i);
  
  pop_n_elems(args);

  push_string(end_shared_string(s));
}

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
  add_storage(sizeof(struct pike_crypto_cast));

  add_function("name", f_name, "function(void:string)", 0);
  add_function("query_block_size", f_query_block_size, "function(void:int)", 0);
  add_function("query_key_length", f_query_key_length, "function(void:int)", 0);
  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:object)", 0);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:object)", 0);
  add_function("crypt_block", f_crypt_block, "function(string:string)", 0);

  set_init_callback(init_pike_crypto_cast);
  set_exit_callback(exit_pike_crypto_cast);

  end_class("cast", 0);
}

void pike_cast_exit(void)
{
}
