/*
 * $Id: pipe.c,v 1.1 1996/11/11 19:24:04 grubba Exp $
 *
 * PIPE crypto module for Pike.
 *
 * /precompiled/crypto/pipe
 *
 * Henrik Grubbström 1996-11-11
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
#include "operators.h"

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

struct program *pike_pipe_program;

/*
 * Functions
 */

void init_pike_pipe(struct object *o)
{
  MEMSET(PIKE_PIPE, 0, sizeof(struct pike_pipe));
}

void exit_pike_pipe(struct object *o)
{
  int i;

  if (PIKE_PIPE->objects) {
    for (i=0; i<PIKE_PIPE->num_objs; i++) {
      free_object(PIKE_PIPE->objects[i]);
    }
    MEMSET(PIKE_PIPE->objects, 0,
	   PIKE_PIPE->num_objs * sizeof(struct object *));
  }
  MEMSET(PIKE_PIPE, 0, sizeof(struct pike_pipe));
}

/*
 * efuns and the like
 */

/* void create(program|object|array(program|mixed)) */
static void f_create(INT32 args)
{
  int i;
  int block_size=1;

  if (!args) {
    error("Too few arguments to pipe->create()\n");
  }
  PIKE_PIPE->objects = (struct object **)xalloc(args * sizeof(struct object *));
  for (i=-args; i; i++) {
    if (sp[i].type == T_OBJECT) {
      PIKE_PIPE->objects[args + i] = sp[i].u.object;
      PIKE_PIPE->objects[i]->refs++;
    } else if (sp[i].type == T_PROGRAM) {
      PIKE_PIPE->objects[i] = clone(sp[i].u.program, 0);
    } else if (sp[i].type == T_ARRAY) {
      struct program *prog;
      INT32 n_args;

      if (!sp[i].u.array->size) {
	error("pipe->create(): Argument %d: Empty array\n", 1 + args + i);
      }
      if (sp[i].u.array->item[0].type != T_PROGRAM) {
	error("pipe->create(): Argument %d: First element of array must be a program\n",
	      1 + args + i);
      }
      prog = sp[i].u.array->item[0].u.program;
      n_args = sp[i].u.array->size - 1;

      push_array_items(sp[i].u.array);	/* Pushes one arg too many */
      PIKE_PIPE->objects[i] = clone(prog, n_args);

      pop_stack();	/* Pop the program */

      assert_is_crypto_module(PIKE_PIPE->objects[i]);
    } else {
      error("Bad argument %d to pipe->create()\n", i + args);
    }
  }
  PIKE_PIPE->num_objs = args;

  for (i=0; i<PIKE_PIPE->num_objs; i++) {
    int j;
    int sub_size;
    int factor = 1;

    safe_apply(PIKE_PIPE->objects[i], "block_size", 0);
    if (sp[-1].type != T_INT) {
      error("pipe->create(): block_size() returned other than int\n");
    }
    sub_size = sp[-1].u.integer;
    pop_stack();

    for (j=2; j <= sub_size;) {
      if (!(block_size % j)) {
	factor *= j;
	block_size /= j;
	sub_size /= j;
      } else {
	j++;
      }
    }
    block_size *= factor * sub_size;
  }

  PIKE_PIPE->block_size = block_size;

  pop_n_elems(args);
}

/* string name(void) */
static void f_name(INT32 args)
{
  int i;

  if (args) {
    error("Too many arguments to pipe->name()\n");
  }
  push_string(make_shared_string("PIPE("));
  
  for (i=0; i<PIKE_PIPE->num_objs; i++) {
    if (i) {
      push_string(make_shared_binary_string(", ", 2));
    }
    safe_apply(PIKE_PIPE->objects[i], "name", 0);
  }
  push_string(make_shared_binary_string(")", 1));

  f_add(2*PIKE_PIPE->num_objs + 1);
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  if (args) {
    error("Too many arguments to pipe->query_block_size()\n");
  }
  push_int(8);
}

/* array(int|array) query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  int i;

  if (args) {
    error("Too many arguments to pipe->query_key_length()\n");
  }

  for (i=0; i<PIKE_PIPE->num_objs; i++) {
    safe_apply(PIKE_PIPE->objects[i], "query_key_length", 0);
  }
  f_aggregate(PIKE_PIPE->num_objs);
}

/* void set_encrypt_key(array|string ..) */
static void f_set_encrypt_key(INT32 args)
{
  int i;

  if (args != PIKE_PIPE->num_objs) {
    error("Wrong number of arguments to pipe->set_encrypt_key()\n");
  }
  for (i=-args; i; i++) {
    int n_args;

    if (sp[i].type == T_STRING) {
      push_string(sp[i].u.string);
      n_args = 1;
    } else if (sp[i].type == T_ARRAY) {
      n_args = sp[i].u.array->size;
      push_array_items(sp[i].u.array);
    } else {
      error("Bad argument %d to pipe->set_encrypt_key()\n", 1 + args + i);
    }
    safe_apply(PIKE_PIPE->objects[args + i], "set_encrypt_key", n_args);
    pop_stack(); /* Get rid of the void value */
  }
  pop_n_elems(args);
}

/* void set_decrypt_key(array|string ..) */
static void f_set_decrypt_key(INT32 args)
{
  int i;

  if (args != PIKE_PIPE->num_objs) {
    error("Wrong number of arguments to pipe->set_decrypt_key()\n");
  }
  for (i=-args; i; i++) {
    int n_args;

    if (sp[i].type == T_STRING) {
      push_string(sp[i].u.string);
      n_args = 1;
    } else if (sp[i].type == T_ARRAY) {
      n_args = sp[i].u.array->size;
      push_array_items(sp[i].u.array);
    } else {
      error("Bad argument %d to pipe->set_decrypt_key()\n", 1 + args + i);
    }
    safe_apply(PIKE_PIPE->objects[args + i], "set_decrypt_key", n_args);
    pop_stack(); /* Get rid of the void value */
  }
  pop_n_elems(args);
}

/* string crypt_block(string) */
static void f_crypt_block(INT32 args)
{
  int i;

  if (args != 1) {
    error("Wrong number of arguments to pipe->crypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to pipe->crypt_block()\n");
  }
  if (sp[-1].u.string->len % PIKE_PIPE->block_size) {
    error("Bad length of argument 1 to pipe->crypt_block()\n");
  }
  for (i=0; i<PIKE_PIPE->num_objs; i++) {
    safe_apply(PIKE_PIPE->objects[i], "crypt_block", 1);
  }
}

/*
 * Module linkage
 */

void init_pipe_efuns(void)
{
  /* add_efun()s */
}

void init_pipe_programs(void)
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

  /* /precompiled/crypto/pipe */
  start_new_program();
  add_storage(sizeof(struct pike_pipe));

  add_function("create", f_create, "function(program|object|array(program|mixed) ...:void)", OPT_SIDE_EFFECT);

  add_function("name", f_name, "function(void:string)", OPT_TRY_OPTIMIZE);
  add_function("query_block_size", f_query_block_size, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("crypt_block", f_crypt_block, "function(string:string)", OPT_EXTERNAL_DEPEND);

  set_init_callback(init_pike_pipe);
  set_exit_callback(exit_pike_pipe);

  pike_pipe_program = end_c_program("/precompiled/crypto/pipe");
  pike_pipe_program->refs++;
}

void exit_pipe(void)
{
  /* free_program()s */
  free_program(pike_pipe_program);
}
