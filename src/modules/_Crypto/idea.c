/*
 * $Id: idea.c,v 1.3 1996/11/08 22:44:50 grubba Exp $
 *
 * IDEA crypto module for Pike
 *
 * /precompiled/crypto/idea
 *
 * Henrik Grubbström 1996-11-03
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
#include <idea.h>

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

struct program *pike_idea_program;

/*
 * Functions
 */

void init_pike_idea(struct object *o)
{
  MEMSET(PIKE_IDEA, 0, sizeof(struct pike_idea));
}

void exit_pike_idea(struct object *o)
{
  MEMSET(PIKE_IDEA, 0, sizeof(struct pike_idea));
}

/*
 * efuns and the like
 */

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  if (args) {
    error("Too many arguments to idea->query_block_size()\n");
  }
  push_int(8);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  if (args) {
    error("Too many arguments to idea->query_key_length()\n");
  }
  push_int(16);
}

/* void set_key(string) */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of args to idea->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->set_key()\n");
  }
  if (sp[-1].u.string->len < 16) {
    error("idea->set_key(): Too short key\n");
  }
  idea_set_encrypt_key((unsigned char *)sp[-1].u.string->str,
		       &PIKE_IDEA->e_key);
  idea_set_decrypt_key(&PIKE_IDEA->e_key, &PIKE_IDEA->d_key);

  MEMCPY(&(PIKE_IDEA->key), sp[-1].u.string->str, 8);

  pop_n_elems(args);
}

/* string encrypt(string) */
static void f_encrypt(INT32 args)
{
  unsigned char buffer[8];

  if (args != 1) {
    error("Wrong number of arguemnts to idea->encrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->encrypt()\n");
  }
  if (sp[-1].u.string->len != 8) {
    error("Bad length of argument 1 to idea->encrypt()\n");
  }

  idea_ecb_encrypt((unsigned char *)sp[-1].u.string->str, buffer,
		   &(PIKE_IDEA->e_key));

  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)buffer, 8));

  MEMSET(buffer, 0, 8);
}

/* string decrypt(string) */
static void f_decrypt(INT32 args)
{
  unsigned char buffer[8];

  if (args != 1) {
    error("Wrong number of arguemnts to idea->decrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->decrypt()\n");
  }
  if (sp[-1].u.string->len != 8) {
    error("Bad length of argument 1 to idea->decrypt()\n");
  }

  idea_ecb_encrypt((unsigned char *)sp[-1].u.string->str, buffer,
		   &(PIKE_IDEA->d_key));

  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)buffer, 8));

  MEMSET(buffer, 0, 8);
}

/* string cbc_encrypt(string) */
static void f_cbc_encrypt(INT32 args)
{
  unsigned char *buffer;
  unsigned char iv[8];
  int len;

  if (args != 1) {
    error("Wrong number of arguments to idea->cbc_encrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->cbc_encrypt()\n");
  }
  
  len = (sp[-1].u.string->len + 7) & ~7;

  if (!(buffer = alloca(len))) {
    error("Out of memory\n");
  }

  MEMCPY(iv, &(PIKE_IDEA->key), 8);

  idea_cbc_encrypt((unsigned char *)sp[-1].u.string->str, buffer,
		   sp[-1].u.string->len,
		   &(PIKE_IDEA->e_key), iv, IDEA_ENCRYPT);

  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)buffer, len));

  MEMSET(buffer, 0, len);
}

/* string cbc_decrypt(string) */
static void f_cbc_decrypt(INT32 args)
{
  unsigned char *buffer;
  unsigned char iv[8];
  int len;

  if (args != 1) {
    error("Wrong number of arguments to idea->cbc_decrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->cbc_decrypt()\n");
  }
  
  len = (sp[-1].u.string->len + 7) & ~7;

  if (!(buffer = alloca(len))) {
    error("Out of memory\n");
  }

  MEMCPY(iv, &(PIKE_IDEA->key), 8);

  idea_cbc_encrypt((unsigned char *)sp[-1].u.string->str, buffer,
		   sp[-1].u.string->len,
		   &(PIKE_IDEA->d_key), iv, IDEA_DECRYPT);

  pop_n_elems(args);

  push_string(make_shared_binary_string((const char *)buffer, len));

  MEMSET(buffer, 0, len);
}

/*
 * Module linkage
 */

void init_idea_efuns(void)
{
  /* add_efun()s */
}

void init_idea_programs(void)
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

  /* /precompiled/crypto/idea */
  start_new_program();
  add_storage(sizeof(struct pike_idea));

  add_function("query_block_size", f_query_block_size, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("set_key", f_set_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("encrypt", f_encrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("decrypt", f_decrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("cbc_encrypt", f_cbc_encrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("cbc_decrypt", f_cbc_decrypt, "function(string:string)", OPT_SIDE_EFFECT);

  set_init_callback(init_pike_idea);
  set_exit_callback(exit_pike_idea);

  pike_idea_program = end_c_program("/precompiled/crypto/idea");
  pike_idea_program->refs++;
}

void exit_idea(void)
{
  /* free_program()s */
  free_program(pike_idea_program);
}
