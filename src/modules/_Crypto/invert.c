/*
 * $Id: invert.c,v 1.1 1996/11/08 22:28:42 grubba Exp $
 *
 * INVERT crypto module for Pike
 *
 * /precompiled/crypto/invert
 *
 * Henrik Grubbström 1996-11-08
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

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

struct program *pike_invert_program;

/*
 * Functions
 */

void init_pike_invert(struct object *o)
{
}

void exit_pike_invert(struct object *o)
{
}

/*
 * efuns and the like
 */

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  if (args) {
    error("Too many arguments to invert->query_block_size()\n");
  }
  push_int(8);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  if (args) {
    error("Too many arguments to invert->query_key_length()\n");
  }
  push_int(8);
}

/* void set_key(string) */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of args to invert->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to invert->set_key()\n");
  }

  pop_n_elems(args);
}

/* string encrypt(string) */
/* string decrypt(string) */
/* string dencrypt(string) */
static void f_dencrypt(INT32 args)
{
  char buffer[8];
  int i;

  if (args != 1) {
    error("Wrong number of arguments to invert->dencrypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to invert->dencrypt()\n");
  }
  if (sp[-1].u.string->len != 8) {
    error("Bad length of argument 1 to invert->dencrypt()\n");
  }

  for (i=0; i<8; i++) {
    buffer[i] = ~sp[-1].u.string->str[i];
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string(buffer, 8));

  MEMSET(buffer, 0, 8);
}

/*
 * Module linkage
 */

void init_invert_efuns(void)
{
  /* add_efun()s */
}

void init_invert_programs(void)
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

  /* /precompiled/crypto/invert */
  start_new_program();

  add_function("query_block_size", f_query_block_size, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("set_key", f_set_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("encrypt", f_dencrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("decrypt", f_dencrypt, "function(string:string)", OPT_SIDE_EFFECT);
  add_function("dencrypt", f_dencrypt, "function(string:string)", OPT_SIDE_EFFECT);

  set_init_callback(init_pike_invert);
  set_exit_callback(exit_pike_invert);

  pike_invert_program = end_c_program("/precompiled/crypto/invert");
  pike_invert_program->refs++;
}

void exit_invert(void)
{
  /* free_program()s */
  free_program(pike_invert_program);
}


