/*
 * $Id: pipe.c,v 1.14 1999/02/10 21:51:39 hubbe Exp $
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
#include "pike_macros.h"
#include "threads.h"
#include "object.h"
#include "stralloc.h"
#include "builtin_functions.h"
#include "operators.h"

/* Prototypes */
#include "crypto.h"

struct pike_crypto_pipe {
  struct object **objects;
  INT32 num_objs;
  INT32 block_size;
  INT32 mode;
};

#undef THIS
#define THIS	((struct pike_crypto_pipe *)(fp->current_storage))
/*
 * Globals
 */

struct program *pike_crypto_pipe_program;

/*
 * Functions
 */

void init_pike_crypto_pipe(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct pike_crypto_pipe));
}

void exit_pike_crypto_pipe(struct object *o)
{
  int i;

  if (THIS->objects) {
    for (i=0; i<THIS->num_objs; i++) {
      if (THIS->objects[i]) {
	free_object(THIS->objects[i]);
      }
    }
    MEMSET(THIS->objects, 0,
	   THIS->num_objs * sizeof(struct object *));
    free(THIS->objects);
  }
  MEMSET(THIS, 0, sizeof(struct pike_crypto_pipe));
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
    error("_Crypto.pipe->create(): Too few arguments\n");
  }
  THIS->objects = (struct object **)xalloc(args * sizeof(struct object *));
  MEMSET(THIS->objects, 0, args * sizeof(struct object *));
  for (i=-args; i; i++) {
    if (sp[i].type == T_OBJECT) {
      add_ref(THIS->objects[args + i] = sp[i].u.object);
    } else if (sp[i].type == T_PROGRAM) {
      THIS->objects[args + i] = clone_object(sp[i].u.program, 0);
    } else if (sp[i].type == T_ARRAY) {
      struct program *prog;
      INT32 n_args;

      if (!sp[i].u.array->size) {
	error("_Crypto.pipe->create(): Argument %d: Empty array\n",
	      1 + args + i);
      }
      if (sp[i].u.array->item[0].type != T_PROGRAM) {
	error("_Crypto.pipe->create(): Argument %d: "
	      "First element of array must be a program\n",
	      1 + args + i);
      }
      prog = sp[i].u.array->item[0].u.program;
      n_args = sp[i].u.array->size - 1;

      push_array_items(sp[i].u.array);	/* Pushes one arg too many */
      THIS->objects[args + i] = clone_object(prog, n_args);

      pop_stack();	/* Pop the program */

      assert_is_crypto_module(THIS->objects[args + i]);
    } else {
      error("_Crypto.pipe->create(): Bad argument %d\n", i + args);
    }
  }
  THIS->num_objs = args;

  for (i=0; i<THIS->num_objs; i++) {
    int j;
    int sub_size;
    int factor = 1;

    safe_apply(THIS->objects[i], "query_block_size", 0);
    if (sp[-1].type != T_INT) {
      error("_Crypto.pipe->create(): query_block_size() returned other than int\n"
	    "for object #%d\n", i+1);
    }
    if (!(sub_size = sp[-1].u.integer)) {
      error("_Crypto.pipe->create(): query_block_size() returned zero\n"
	    "for object #%d\n", i+1);
    }
    pop_stack();

    /* Calculate the least common factor, and use that as the block size */
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

  THIS->block_size = block_size;

  pop_n_elems(args);
}

/* string name(void) */
static void f_name(INT32 args)
{
  int i;

  pop_n_elems(args);

  push_text("PIPE(");
  
  for (i=0; i<THIS->num_objs; i++) {
    if (i) {
      push_text(", ");
    }
    safe_apply(THIS->objects[i], "name", 0);
  }
  push_text(")");

  f_add(2*THIS->num_objs + 1);
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);

  push_int(THIS->block_size);
}

/* array(int|array) query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  int i;

  pop_n_elems(args);

  for (i=0; i<THIS->num_objs; i++) {
    safe_apply(THIS->objects[i], "query_key_length", 0);
  }
  f_aggregate(THIS->num_objs);
}

/* void set_encrypt_key(array|string ..) */
static void f_set_encrypt_key(INT32 args)
{
  int i;

  if (args != THIS->num_objs) {
    error("_Crypto.pipe->set_encrypt_key(): Wrong number of arguments\n");
  }
  THIS->mode = 0;
  for (i=-args; i; i++) {
    int n_args;

    if (sp[i].type == T_STRING) {
      ref_push_string(sp[i].u.string);
      n_args = 1;
    } else if (sp[i].type == T_ARRAY) {
      n_args = sp[i].u.array->size;
      push_array_items(sp[i].u.array);
    } else {
      error("_Crypto.pipe->set_encrypt_key(): Bad argument %d\n",
	    1 + args + i);
    }
    safe_apply(THIS->objects[args + i], "set_encrypt_key", n_args);
    pop_stack(); /* Get rid of the void value */
  }
  pop_n_elems(args);
  push_object(this_object());
}

/* void set_decrypt_key(array|string ..) */
static void f_set_decrypt_key(INT32 args)
{
  int i;

  if (args != THIS->num_objs) {
    error("_Crypto.pipe->set_decrypt_key(): Wrong number of arguments\n");
  }
  THIS->mode = 1;
  for (i=-args; i; i++) {
    int n_args;

    if (sp[i].type == T_STRING) {
      ref_push_string(sp[i].u.string);
      n_args = 1;
    } else if (sp[i].type == T_ARRAY) {
      n_args = sp[i].u.array->size;
      push_array_items(sp[i].u.array);
    } else {
      error("_Crypto.pipe->set_decrypt_key(): Bad argument %d\n",
	    1 + args + i);
    }
    safe_apply(THIS->objects[args + i], "set_decrypt_key", n_args);
    pop_stack(); /* Get rid of the void value */
  }
  push_object(this_object());
  pop_n_elems(args);
}

/* string crypt_block(string) */
static void f_crypt_block(INT32 args)
{
  int i;

  if (args < 1) {
    error("_Crypto.pipe->crypt_block(): Too few arguments\n");
  }
  if (args > 1) {
    pop_n_elems(args-1);
  }
  if (sp[-1].type != T_STRING) {
    error("_Crypto.pipe->crypt_block(): Bad argument 1\n");
  }
  if (sp[-1].u.string->len % THIS->block_size) {
    error("_Crypto.pipe->crypt_block(): Bad length of argument 1\n");
  }
  if (THIS->mode) {
    /* Decryption -- Reverse the order */
    for (i=THIS->num_objs; i--;) {
      safe_apply(THIS->objects[i], "crypt_block", 1);
    }
  } else {
    for (i=0; i<THIS->num_objs; i++) {
      safe_apply(THIS->objects[i], "crypt_block", 1);
    }
  }
}

/*
 * Module linkage
 */

void pike_pipe_init(void)
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
  ADD_STORAGE(struct pike_crypto_pipe);

  /* function(program|object|array(program|mixed) ...:void) */
  ADD_FUNCTION("create", f_create,tFuncV(,tOr3(tPrg,tObj,tArr(tOr(tPrg,tMix))),tVoid), 0);

  /* function(void:string) */
  ADD_FUNCTION("name", f_name,tFunc(tVoid,tStr), 0);
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

  set_init_callback(init_pike_crypto_pipe);
  set_exit_callback(exit_pike_crypto_pipe);

  end_class("pipe", 0);
}

void pike_pipe_exit(void)
{
}
