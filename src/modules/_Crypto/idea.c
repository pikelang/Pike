/*
 * $Id: idea.c,v 1.4 1996/11/11 14:23:25 grubba Exp $
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

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) {
    error("Too many arguments to idea->name()\n");
  }
  push_string(make_shared_string("IDEA"));
}

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

/* void set_encrypt_key(string) */
static void f_set_encrypt_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of args to idea->set_encrypt_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->set_encrypt_key()\n");
  }
  if (sp[-1].u.string->len < 16) {
    error("idea->set_encrypt_key(): Too short key\n");
  }
  idea_set_encrypt_key((unsigned char *)sp[-1].u.string->str,
		       &PIKE_IDEA->key);

  pop_n_elems(args);
}

/* void set_decrypt_key(string) */
static void f_set_decrypt_key(INT32 args)
{
  IDEA_KEY_SCHEDULE key_tmp;

  if (args != 1) {
    error("Wrong number of args to idea->set_decrypt_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->set_decrypt_key()\n");
  }
  if (sp[-1].u.string->len < 16) {
    error("idea->set_decrypt_key(): Too short key\n");
  }
  idea_set_encrypt_key((unsigned char *)sp[-1].u.string->str, &key_tmp);
  idea_set_decrypt_key(&key_tmp, &PIKE_IDEA->key);

  pop_n_elems(args);

  MEMSET(&key_tmp, 0, sizeof(key_tmp));
}

/* string crypt_block(string) */
static void f_crypt_block(INT32 args)
{
  unsigned char *buffer;
  int len;

  if (args != 1) {
    error("Wrong number of arguemnts to idea->crypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to idea->crypt()\n");
  }
  if (sp[-1].u.string->len % 8) {
    error("Bad length of argument 1 to idea->crypt()\n");
  }

  if (!(buffer = alloca(len = sp[-1].u.string->len))) {
    error("idea->crypt(): Out of memory\n");
  }

  idea_ecb_encrypt((unsigned char *)sp[-1].u.string->str, buffer,
		   &(PIKE_IDEA->key));

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

  add_function("name", f_name, "function(void:string)", OPT_TRY_OPTIMIZE);
  add_function("query_block_size", f_query_block_size, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("crypt_block", f_crypt_block, "function(string:string)", OPT_EXTERNAL_DEPEND);

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
