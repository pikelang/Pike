/*
 * $Id: des.c,v 1.4 1997/01/14 18:28:00 nisse Exp $
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
#include "macros.h"
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

#if 0
static void print_hex(const char *str, int len)
{
  static char buffer[4096];
  int i;

  for (i=0; i<len; i++) {
    sprintf(buffer+i*2, "%02x", str[i] & 0xff);
  }
  write(2, buffer, strlen(buffer));
}
#endif

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
  switch (DesMethod(&(THIS->method), sp[-1].u.string->str))
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
  this_object()->refs++;
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

#if 0
/* void set_key(string) */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of arguments to des->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->set_key()\n");
  }
  if ((sp[-1].u.string->len != 8) &&
      (sp[-1].u.string->len != 16)) {
    error("des->set_key(): Unsupported key length %d\n",
	  sp[-1].u.string->len);
  }
  if ((THIS->flags3 = (sp[-1].u.string->len == 16))) {
    des_set_key((C_Block *)(sp[-1].u.string->str + 8),
		THIS->key_schedules[1]);
  }
  des_set_key((C_Block *)(sp[-1].u.string->str),
	      THIS->key_schedules[0]);

  pop_n_elems(args);
}

/* string get_schedule(void) */
static void f_get_schedule(INT32 args)
{
  if (args) {
    error("Too many arguments to des->get_schedule()\n");
  }
  push_string(make_shared_binary_string((const char *)&(THIS->key_schedules[0]),
					sizeof(THIS->key_schedules)));
  sp[-1].u.string->refs++;
}

/* string make_key(string) */
static void f_make_key(INT32 args)
{
  des_cblock buffer[2];

  if (args != 1) {
    error("Wrong number of arguments to des->make_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->make_key()\n");
  }
  write(2, "rawkey:", 7);
  write(2, sp[-1].u.string->str, sp[-1].u.string->len);
  write(2, "\n", 1);
  des_string_to_2keys(sp[-1].u.string->str, &buffer[0], &buffer[1]);
  des_set_key(&buffer[1], &THIS->key_schedules[1]);

  write(2, "key:", 4);
  print_hex(buffer, sizeof(buffer));
  write(2, "\n", 1);

  if (strlen(sp[-1].u.string->str) != sp[-1].u.string->len) {
    write(2, "FOOBAR!\n", 8);
  }

  if (sp[-1].u.string->len > 8) {
    pop_n_elems(args);

    push_string(make_shared_binary_string((const char *)&(buffer[0][0]), 16));
  } else {
    pop_n_elems(args);

    push_string(make_shared_binary_string((const char *)&(buffer[0][0]), 8));
  }
  sp[-1].u.string->refs++;

  memset(buffer, 0, sizeof(buffer));
}

/* string make_sun_key(string) */
static void f_make_sun_key(INT32 args)
{
  des_cblock key;
  int i;

  if (args != 1) {
    error("Wrong number of arguments to des->make_sun_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->make_sun_key()\n");
  }
  if (sp[-1].u.string->len < 8) {
    error("Too short key to des->make_sun_key()\n");
  }
  for (i=0; i<8; i++) {
    int parity;
    unsigned byte;
    /* Calculate parity
     *
     * SSLeay bug:
     *	SSLeay has bit #7 is in the parity, but for a key
     *	to be legal it must have odd parity, so...
     */
    for ((byte = sp[-1].u.string->str[i] & 0x7f), parity=1; byte; byte >>= 1) {
      parity ^= byte & 1;
    }
    key[i] = (sp[-1].u.string->str[i] & 0x7f) | (parity<<7);
  }
  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)key, 8));
  sp[-1].u.string->refs++;

  memset(key, 0, 8);
}

/* void sum(string) */
static void f_sum(INT32 args)
{
  unsigned char *buffer;
  unsigned newlen;

  if (args != 1) {
    error("Wrong number of arguments to des->sum()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->sum()\n");
  }
  if (!(buffer = alloca(sp[-1].u.string->len + 8))) {
    error("des->sum(): Out of memory\n");
  }
  if ((newlen = THIS->overflow_len)) {
    memcpy(buffer, THIS->overflow, newlen);
  }
  memcpy(buffer + newlen, sp[-1].u.string->str, sp[-1].u.string->len);
  newlen += sp[-1].u.string->len;

  if ((THIS->overflow_len = newlen & 0x07)) {
    memcpy(THIS->overflow, buffer + (newlen & ~0x07), newlen & 0x07);
  }

  /* NOTE: Need to sum the last part at the end */
  des_cbc_cksum((C_Block *)buffer, &THIS->checksum,
		newlen & ~0x07, THIS->key_schedules[0],
		&THIS->checksum);
  
  memset(buffer, 0, newlen);

  pop_n_elems(args);
}

/* DDDDDDDDDDxxxxx(actual%8) */
#endif

/* string encrypt(string) */
static void f_crypt_block(INT32 args)
{
  unsigned len;
  struct pike_string *s;
  INT32 i;
  
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


#if 0
/* string dump(void) */
static void f_dump(INT32 args)
{
  if (args) {
    error("Too many arguemnts to des->dump()\n");
  }
  push_string(make_shared_binary_string(THIS, sizeof(struct pike_crypto_des)));
  sp[-1].u.string->refs++;
}
#endif

/*
 * Module linkage
 */

void MOD_INIT2(des)(void)
{
  /* add_efun()s */
}

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

  add_function("query_block_size", f_query_block_size,
	       "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length,
	       "function(void:int)", OPT_TRY_OPTIMIZE);

  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:object)", OPT_SIDE_EFFECT);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:object)", OPT_SIDE_EFFECT);
  add_function("crypt_block", f_crypt_block, "function(string:string)", OPT_SIDE_EFFECT);
#if 0
  add_function("get_schedule", f_get_schedule, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("make_key", f_make_key, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function("make_sun_key", f_make_sun_key, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function("encrypt", f_encrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("decrypt", f_decrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("sum", f_sum, "function(string:void)", OPT_SIDE_EFFECT);

  add_function("dump", f_dump, "function(void:string)", OPT_EXTERNAL_DEPEND);
#endif
  set_init_callback(init_pike_crypto_des);
  set_exit_callback(exit_pike_crypto_des);

  pike_crypto_des_program = end_c_program(MODULE_PREFIX "des");
  pike_crypto_des_program->refs++;
}

void MOD_EXIT(des)(void)
{
  /* free_program()s */
  free_program(pike_crypto_des_program);
}
