/*
 * $Id: cbc.c,v 1.1 1996/11/11 14:22:08 grubba Exp $
 *
 * CBC (Cipher Block Chaining Mode) crypto module for Pike.
 *
 * /precompiled/crypto/cbc
 *
 * Henrik Grubbström 1996-11-10
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
#include "interpret.h"
#include "builtin_functions.h"

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

static struct program *pike_cbc_program;

/*
 * Functions
 */

static void init_pike_cbc(struct object *o)
{
  memset(PIKE_CBC, 0, sizeof(struct pike_cbc));
}

static void exit_pike_cbc(struct object *o)
{
  if (PIKE_CBC->object) {
    free_object(PIKE_CBC->object);
  }
  if (PIKE_CBC->iv) {
    MEMSET(PIKE_CBC->iv, 0, PIKE_CBC->block_size);
    free(PIKE_CBC->iv);
  }
  memset(PIKE_CBC, 0, sizeof(struct pike_cbc));
}

INLINE static void cbc_encrypt_step(const unsigned char *source,
				    unsigned char *dest)
{
  INT32 block_size = PIKE_CBC->block_size;
  INT32 i;
  
  for (i=0; i < block_size; i++) {
    PIKE_CBC->iv[i] ^= source[i];
  }

  push_string(make_shared_binary_string((char *)PIKE_CBC->iv, block_size));
  safe_apply(PIKE_CBC->object, "crypt_block", 1);

  if (sp[-1].type != T_STRING) {
    error("cbc->encrypt(): Expected string from crypt_block()\n");
  }
  if (sp[-1].u.string->len != block_size) {
    error("cbc->encrypt(): Bad string length %d returned from crypt_block()\n",
	  sp[-1].u.string->len);
  }
  MEMCPY(PIKE_CBC->iv, sp[-1].u.string->str, block_size);
  MEMCPY(dest, sp[-1].u.string->str, block_size);
  pop_stack();
}

INLINE static void cbc_decrypt_step(const unsigned char *source,
				    unsigned char *dest)
{
  INT32 block_size = PIKE_CBC->block_size;
  INT32 i;
  
  push_string(make_shared_binary_string((const char *)source, block_size));
  safe_apply(PIKE_CBC->object, "crypt_block", 1);

  if (sp[-1].type != T_STRING) {
    error("cbc->decrypt(): Expected string from crypt_block()\n");
  }
  if (sp[-1].u.string->len != block_size) {
    error("cbc->decrypt(): Bad string length %d returned from crypt_block()\n",
	  sp[-1].u.string->len);
  }

  for (i=0; i < block_size; i++) {
    dest[i] = PIKE_CBC->iv[i] ^ sp[-1].u.string->str[i];
  }

  pop_stack();
  MEMCPY(PIKE_CBC->iv, source, block_size);
}

/*
 * efuns and the like
 */

/* void create(program|object, ...) */
static void f_create(INT32 args)
{
  if (args < 1) {
    error("Too few arguments to cbc->create()\n");
  }
  if ((sp[-args].type != T_PROGRAM) &&
      (sp[-args].type != T_OBJECT)) {
    error("Bad argument 1 to cbc->create()\n");
  }
  if (sp[-args].type == T_PROGRAM) {
    PIKE_CBC->object = clone(sp[-args].u.program, args-1);
  } else {
    if (args != 1) {
      error("Too many arguments to cbc->create()\n");
    }
    PIKE_CBC->object = sp[-args].u.object;
    PIKE_CBC->object->refs++;
  }
  pop_stack(); /* Just one element left on the stack in both cases */

  assert_is_crypto_module(PIKE_CBC->object);

  safe_apply(PIKE_CBC->object, "query_block_size", 0);

  if (sp[-1].type != T_INT) {
    error("cbc->create(): query_block_size() didn't return an int\n");
  }
  PIKE_CBC->block_size = sp[-1].u.integer;

  pop_stack();

  if ((!PIKE_CBC->block_size) ||
      (PIKE_CBC->block_size > 4096)) {
    error("cbc->create(): Bad block size %d\n", PIKE_CBC->block_size);
  }

  PIKE_CBC->iv = (unsigned char *)xalloc(PIKE_CBC->block_size);
  MEMSET(PIKE_CBC->iv, 0, PIKE_CBC->block_size);
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_CBC->block_size);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  safe_apply(PIKE_CBC->object, "query_key_length", args);
}

/* void set_encrypt_key(INT32 args) */
static void f_set_encrypt_key(INT32 args)
{
  if (PIKE_CBC->block_size) {
    MEMSET(PIKE_CBC->iv, 0, PIKE_CBC->block_size);
  } else {
    error("cbc->set_encrypt_key(): Object has not been created yet\n");
  }
  PIKE_CBC->mode = 0;
  safe_apply(PIKE_CBC->object, "set_encrypt_key", args);
}

/* void set_decrypt_key(INT32 args) */
static void f_set_decrypt_key(INT32 args)
{
  if (PIKE_CBC->block_size) {
    MEMSET(PIKE_CBC->iv, 0, PIKE_CBC->block_size);
  } else {
    error("cbc->set_decrypt_key(): Object has not been created yet\n");
  }
  PIKE_CBC->mode = 1;
  safe_apply(PIKE_CBC->object, "set_decrypt_key", args);
}

/* string encrypt_block(string) */
static void f_encrypt_block(INT32 args)
{
  unsigned char *result;
  INT32 offset = 0;

  if (args != 1) {
    error("Wrong number of arguments to cbc->encrypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to cbc->encrypt_block()\n");
  }
  if (sp[-1].u.string->len % PIKE_CBC->block_size) {
    error("Bad length of argument 1 to cbc->encrypt_block()\n");
  }
  if (!(result = alloca(sp[-1].u.string->len))) {
    error("cbc->encrypt_block(): Out of memory\n");
  }

  while (offset < sp[-1].u.string->len) {

    cbc_encrypt_step((const unsigned char *)sp[-1].u.string->str + offset,
		     result + offset);
    offset += PIKE_CBC->block_size;
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((char *)result, offset));
  MEMSET(result, 0, offset);
}

/* string decrypt_block(string) */
static void f_decrypt_block(INT32 args)
{
  unsigned char *result;
  INT32 offset = 0;

  if (args != 1) {
    error("Wrong number of arguments to cbc->decrypt_block()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to cbc->decrypt_block()\n");
  }
  if (sp[-1].u.string->len & PIKE_CBC->block_size) {
    error("Bad length of argument 1 to cbc->decrypt_block()\n");
  }
  if (!(result = alloca(sp[-1].u.string->len))) {
    error("cbc->cbc_decrypt(): Out of memory\n");
  }

  while (offset < sp[-1].u.string->len) {

    cbc_decrypt_step((const unsigned char *)sp[-1].u.string->str + offset,
		     result + offset);
    offset += PIKE_CBC->block_size;
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((char *)result, offset));
  MEMSET(result, 0, offset);
}

/* string crypt_block(string) */
static void f_crypt_block(INT32 args)
{
  if (PIKE_CBC->mode) {
    f_decrypt_block(args);
  } else {
    f_encrypt_block(args);
  }
}

/*
 * Module linkage
 */

void init_cbc_efuns(void)
{
  /* add_efun()s */
}

void init_cbc_programs(void)
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
  add_storage(sizeof(struct pike_cbc));

  add_function("create", f_create, "function(program|object:void)", OPT_EXTERNAL_DEPEND);

  add_function("query_block_size", f_query_block_size, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length, "function(void:int)", OPT_TRY_OPTIMIZE);

  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("crypt_block", f_crypt_block, "function(string:string)", OPT_EXTERNAL_DEPEND);
  add_function("encrypt_block", f_encrypt_block, "function(string:string)", OPT_EXTERNAL_DEPEND);
  add_function("decrypt_block", f_decrypt_block, "function(string:string)", OPT_EXTERNAL_DEPEND);

  set_init_callback(init_pike_cbc);
  set_exit_callback(exit_pike_cbc);

  pike_cbc_program = end_c_program("/precompiled/crypto/cbc");
  pike_cbc_program->refs++;
}

void exit_cbc(void)
{
  /* free_program()s */
  free_program(pike_cbc_program);
}

