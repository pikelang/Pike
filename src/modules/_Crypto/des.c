/*
 * $Id: des.c,v 1.1.1.1 1996/11/05 15:10:09 grubba Exp $
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
#include "builtin_functions.h"

/* System includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* SSL includes */
#include <des.h>

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

struct program *pike_des_program;

/*
 * Functions
 */

static void print_hex(const char *str, int len)
{
  static char buffer[4096];
  int i;

  for (i=0; i<len; i++) {
    sprintf(buffer+i*2, "%02x", str[i] & 0xff);
  }
  write(2, buffer, strlen(buffer));
}


static void init_pike_des(struct object *o)
{
  memset(PIKE_DES, 0, sizeof(struct pike_des));
}

static void exit_pike_des(struct object *o)
{
  memset(PIKE_DES, 0, sizeof(struct pike_des));
}

/*
 * efuns and the like
 */

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
  if ((PIKE_DES->flags3 = (sp[-1].u.string->len == 16))) {
    des_set_key((C_Block *)(sp[-1].u.string->str + 8),
		PIKE_DES->key_schedules[1]);
  }
  des_set_key((C_Block *)(sp[-1].u.string->str),
	      PIKE_DES->key_schedules[0]);

  pop_n_elems(args);
}

/* string get_schedule(void) */
static void f_get_schedule(INT32 args)
{
  if (args) {
    error("Too many arguments to des->get_schedule()\n");
  }
  push_string(make_shared_binary_string((const char *)&(PIKE_DES->key_schedules[0]),
					sizeof(PIKE_DES->key_schedules)));
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
  des_set_key(&buffer[1], &PIKE_DES->key_schedules[1]);

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
  if ((newlen = PIKE_DES->overflow_len)) {
    memcpy(buffer, PIKE_DES->overflow, newlen);
  }
  memcpy(buffer + newlen, sp[-1].u.string->str, sp[-1].u.string->len);
  newlen += sp[-1].u.string->len;

  if ((PIKE_DES->overflow_len = newlen & 0x07)) {
    memcpy(PIKE_DES->overflow, buffer + (newlen & ~0x07), newlen & 0x07);
  }

  /* NOTE: Need to sum the last part at the end */
  des_cbc_cksum((C_Block *)buffer, &PIKE_DES->checksum,
		newlen & ~0x07, PIKE_DES->key_schedules[0],
		&PIKE_DES->checksum);
  
  memset(buffer, 0, newlen);

  pop_n_elems(args);
}

/* DDDDDDDDDDxxxxx(actual%8) */

/* string encrypt(string) */
static void f_encrypt(INT32 args)
{
  unsigned char *buffer;
  unsigned len;

  if (args != 1) {
    error("Wrong number of arguments to des->encrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->encrypt()\n");
  }
  if ((len = sp[-1].u.string->len) & 0x07) {
    error("Bad string length in des->encrypt()\n");
  }
  if (!(buffer = alloca(len))) {
    error("des->encrypt(): Out of memory\n");
  }

  if (PIKE_DES->flags3) {
    write(2, "Mode:3\n", 7);

    des_3cbc_encrypt((C_Block *)sp[-1].u.string->str, (C_Block *)buffer, len,
		     &(PIKE_DES->key_schedules[0][0]),
		     &(PIKE_DES->key_schedules[1][0]),
		     &PIKE_DES->ivs[0], &PIKE_DES->ivs[1], DES_ENCRYPT);
  } else {
    write(2, "Mode:0\n", 7);

    des_cbc_encrypt((C_Block *)sp[-1].u.string->str, (C_Block *)buffer, len,
		    &(PIKE_DES->key_schedules[0][0]),
		    &PIKE_DES->ivs[0], DES_ENCRYPT);
    if (len > 8) {
      /* This seems unmotivated */
      memcpy(&PIKE_DES->ivs[0], buffer + len - 8, 8);
    }
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)buffer, len));
  sp[-1].u.string->refs++;
  
  memset(buffer, 0, len);
}

/* string decrypt(string) */
static void f_decrypt(INT32 args)
{
  unsigned char *buffer;
  unsigned len;

  if (args != 1) {
    error("Wrong number of arguments to des->decrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des->decrypt()\n");
  }
  if ((len = sp[-1].u.string->len) & 0x07) {
    error("Bad string length in des->decrypt()\n");
  }
  if (!(buffer = alloca(len))) {
    error("des->decrypt(): Out of memory\n");
  }

  if (PIKE_DES->flags3) {
    write(2, "Mode:3\n", 7);

    des_3cbc_encrypt((C_Block *)sp[-1].u.string->str, (C_Block *)buffer, len,
		     &(PIKE_DES->key_schedules[0][0]),
		     &(PIKE_DES->key_schedules[1][0]),
		     &(PIKE_DES->ivs[0]), &(PIKE_DES->ivs[1]), DES_DECRYPT);
  } else {
    write(2, "Mode:0\n", 7);

    des_cbc_encrypt((C_Block *)sp[-1].u.string->str, (C_Block *)buffer, len,
		    &(PIKE_DES->key_schedules[0][0]),
		    &(PIKE_DES->ivs[0]), DES_DECRYPT);
    if (len > 8) {
      /* This seems unmotivated */
      memcpy(buffer + len - 8, &PIKE_DES->ivs[0], 8);
    }
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)buffer, len));
  sp[-1].u.string->refs++;
  
  memset(buffer, 0, len);
}

/* string dump(void) */
static void f_dump(INT32 args)
{
  if (args) {
    error("Too many arguemnts to des->dump()\n");
  }
  push_string(make_shared_binary_string(PIKE_DES, sizeof(struct pike_des)));
  sp[-1].u.string->refs++;
}


/*
 * Module linkage
 */

void init_des_efuns(void)
{
  /* add_efun()s */
}

void init_des_programs(void)
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
  add_storage(sizeof(struct pike_des));

  add_function("set_key", f_set_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("get_schedule", f_get_schedule, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("make_key", f_make_key, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function("make_sun_key", f_make_sun_key, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_function("encrypt", f_encrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("decrypt", f_decrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("sum", f_sum, "function(string:void)", OPT_SIDE_EFFECT);

  add_function("dump", f_dump, "function(void:string)", OPT_EXTERNAL_DEPEND);

  set_init_callback(init_pike_des);
  set_exit_callback(exit_pike_des);

  pike_des_program = end_c_program("/precompiled/crypto/des");
  pike_des_program->refs++;
}

void exit_des(void)
{
  /* free_program()s */
  free_program(pike_des_program);
}
