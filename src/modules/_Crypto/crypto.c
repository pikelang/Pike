/*
 * $Id: crypto.c,v 1.8 1996/11/11 19:25:11 grubba Exp $
 *
 * A pike module for getting access to some common cryptos.
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
#include "interpret.h"
#include "builtin_functions.h"

/* System includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* Module specific includes */
#include "precompiled_crypto.h"

/*
 * Globals
 */

static const char *crypto_functions[] = {
  "query_block_size",
  "query_key_length",
  "set_encrypt_key",
  "set_decrypt_key",
  "crypt_block",
  NULL
};

static struct program *pike_crypto_program;

/*
 * Functions
 */

static void init_pike_crypto(struct object *o)
{
  memset(PIKE_CRYPTO, 0, sizeof(struct pike_crypto));
}

static void exit_pike_crypto(struct object *o)
{
  if (PIKE_CRYPTO->object) {
    free_object(PIKE_CRYPTO->object);
  }
  if (PIKE_CRYPTO->backlog) {
    MEMSET(PIKE_CRYPTO->backlog, 0, PIKE_CRYPTO->block_size);
    free(PIKE_CRYPTO->backlog);
  }
  memset(PIKE_CRYPTO, 0, sizeof(struct pike_crypto));
}

static void check_functions(struct object *o, const char **requiered)
{
  struct program *p;

  if (!o) {
    error("/precompiled/crypto: internal error -- no object\n");
  }
  if (!requiered) {
    return;
  }

  p = o->prog;

  while (*requiered) {
    if (find_identifier(*requiered, p) < 0) {
      error("/precompiled/crypto: Object is missing identifier \"%s\"\n",
	    *requiered);
    }
    requiered++;
  }
}

void assert_is_crypto_module(struct object *o)
{
  check_functions(o, crypto_functions);
}

/*
 * efuns and the like
 */

/* string string_to_hex(string) */
static void f_string_to_hex(INT32 args)
{
  char *buffer;
  int i;

  if (args != 1) {
    error("Wrong number of arguments to string_to_hex()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to string_to_hex()\n");
  }

  if (!(buffer=alloca(sp[-1].u.string->len*2))) {
    error("string_to_hex(): Out of memory\n");
  }
  
  for (i=0; i<sp[-1].u.string->len; i++) {
    sprintf(buffer + i*2, "%02x", sp[-1].u.string->str[i] & 0xff);
  }
  
  pop_n_elems(args);
  
  push_string(make_shared_binary_string(buffer, i*2));
  sp[-1].u.string->refs++;
}

/* string hex_to_string(string) */
static void f_hex_to_string(INT32 args)
{
  char *buffer;
  int i;

  if (args != 1) {
    error("Wrong number of arguments to hex_to_string()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to hex_to_string()\n");
  }
  if (sp[-1].u.string->len & 1) {
    error("Bad string length to hex_to_string()\n");
  }
  if (!(buffer = alloca(sp[-1].u.string->len/2))) {
    error("hex_to_string(): Out of memory\n");
  }
  for (i=0; i*2<sp[-1].u.string->len; i++) {
    switch (sp[-1].u.string->str[i*2]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      buffer[i] = (sp[-1].u.string->str[i*2] - '0')<<4;
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      buffer[i] = (sp[-1].u.string->str[i*2] + 10 - 'A')<<4;
      break;
    default:
      error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2] & 0xff);
    }
    switch (sp[-1].u.string->str[i*2+1]) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      buffer[i] |= sp[-1].u.string->str[i*2+1] - '0';
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      buffer[i] |= (sp[-1].u.string->str[i*2+1] + 10 - 'A') & 0x0f;
      break;
    default:
      error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2+1] & 0xff);
    }
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string(buffer, i));
  sp[-1].u.string->refs++;
}

/*
 * /precompiled/crypto
 */

/* void create(program|object, ...) */
static void f_create(INT32 args)
{
  if (args < 1) {
    error("Too few arguments to crypto->create()\n");
  }
  if ((sp[-args].type != T_PROGRAM) &&
      (sp[-args].type != T_OBJECT)) {
    error("Bad argument 1 to crypto->create()\n");
  }
  if (sp[-args].type == T_PROGRAM) {
    PIKE_CRYPTO->object = clone(sp[-args].u.program, args-1);
  } else {
    if (args != 1) {
      error("Too many arguments to crypto->create()\n");
    }
    PIKE_CRYPTO->object = sp[-args].u.object;
    PIKE_CRYPTO->object->refs++;
  }
  pop_stack(); /* Just one element left on the stack in both cases */

  check_functions(PIKE_CRYPTO->object, crypto_functions);

  safe_apply(PIKE_CRYPTO->object, "query_block_size", 0);

  if (sp[-1].type != T_INT) {
    error("crypto->create(): query_block_size() didn't return an int\n");
  }
  PIKE_CRYPTO->block_size = sp[-1].u.integer;

  pop_stack();

  if ((!PIKE_CRYPTO->block_size) ||
      (PIKE_CRYPTO->block_size > 4096)) {
    error("crypto->create(): Bad block size %d\n", PIKE_CRYPTO->block_size);
  }

  PIKE_CRYPTO->backlog = (unsigned char *)xalloc(PIKE_CRYPTO->block_size);
  PIKE_CRYPTO->backlog_len = 0;
  MEMSET(PIKE_CRYPTO->backlog, 0, PIKE_CRYPTO->block_size);
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(PIKE_CRYPTO->block_size);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  safe_apply(PIKE_CRYPTO->object, "query_key_length", args);
}

/* void set_encrypt_key(INT32 args) */
static void f_set_encrypt_key(INT32 args)
{
  if (PIKE_CRYPTO->block_size) {
    MEMSET(PIKE_CRYPTO->backlog, 0, PIKE_CRYPTO->block_size);
    PIKE_CRYPTO->backlog_len = 0;
  } else {
    error("crypto->set_encrypt_key(): Object has not been created yet\n");
  }
  safe_apply(PIKE_CRYPTO->object, "set_encrypt_key", args);
}

/* void set_decrypt_key(INT32 args) */
static void f_set_decrypt_key(INT32 args)
{
  if (PIKE_CRYPTO->block_size) {
    MEMSET(PIKE_CRYPTO->backlog, 0, PIKE_CRYPTO->block_size);
    PIKE_CRYPTO->backlog_len = 0;
  } else {
    error("crypto->set_decrypt_key(): Object has not been created yet\n");
  }
  safe_apply(PIKE_CRYPTO->object, "set_decrypt_key", args);
}

/* string crypt(string) */
static void f_crypt(INT32 args)
{
  unsigned char *result;
  INT32 roffset = 0;
  INT32 soffset = 0;
  INT32 len;

  if (args != 1) {
    error("Wrong number of arguments to crypto->crypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to crypto->crypt()\n");
  }
  if (!(result = alloca(sp[-1].u.string->len + PIKE_CRYPTO->block_size))) {
    error("crypto->crypt(): Out of memory\n");
  }
  if (PIKE_CRYPTO->backlog_len) {
    if (sp[-1].u.string->len >=
	(PIKE_CRYPTO->block_size - PIKE_CRYPTO->backlog_len)) {
      MEMCPY(PIKE_CRYPTO->backlog + PIKE_CRYPTO->backlog_len,
	     sp[-1].u.string->str,
	     (PIKE_CRYPTO->block_size - PIKE_CRYPTO->backlog_len));
      soffset += (PIKE_CRYPTO->block_size - PIKE_CRYPTO->backlog_len);
      PIKE_CRYPTO->backlog_len = 0;
      push_string(make_shared_binary_string((char *)PIKE_CRYPTO->backlog,
					    PIKE_CRYPTO->block_size));
      safe_apply(PIKE_CRYPTO->object, "crypt_block", 1);
      if (sp[-1].type != T_STRING) {
	error("crypto->crypt(): crypt_block() did not return string\n");
      }
      if (sp[-1].u.string->len != PIKE_CRYPTO->block_size) {
	error("crypto->crypt(): Unexpected string length %d\n",
	      sp[-1].u.string->len);
      }
	
      MEMCPY(result, sp[-1].u.string->str, PIKE_CRYPTO->block_size);
      roffset = PIKE_CRYPTO->block_size;
      pop_stack();
      MEMSET(PIKE_CRYPTO->backlog, 0, PIKE_CRYPTO->block_size);
    } else {
      MEMCPY(PIKE_CRYPTO->backlog + PIKE_CRYPTO->backlog_len,
	     sp[-1].u.string->str, sp[-1].u.string->len);
      PIKE_CRYPTO->backlog_len += sp[-1].u.string->len;
      pop_n_elems(args);
      push_string(make_shared_binary_string("", 0));
      return;
    }
  }
  
  len = (sp[-1].u.string->len - soffset);
  len -= len % PIKE_CRYPTO->block_size;

  if (len) {
    push_string(make_shared_binary_string(sp[-1].u.string->str + soffset, len));
    soffset += len;

    safe_apply(PIKE_CRYPTO->object, "crypt_block", 1);

    if (sp[-1].type != T_STRING) {
      error("crypto->crypt(): crypt_block() did not return string\n");
    }
    if (sp[-1].u.string->len != len) {
      error("crypto->crypt(): Unexpected string length %d\n",
	    sp[-1].u.string->len);
    }
	
    MEMCPY(result + roffset, sp[-1].u.string->str, len);

    pop_stack();
  }

  if (soffset < sp[-1].u.string->len) {
    MEMCPY(PIKE_CRYPTO->backlog, sp[-1].u.string->str + soffset,
	   sp[-1].u.string->len - soffset);
    PIKE_CRYPTO->backlog_len = sp[-1].u.string->len - soffset;
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((char *)result, roffset + len));
  MEMSET(result, 0, roffset + len);
}

/* string pad(void) */
static void f_pad(INT32 args)
{
  int i;

  if (args) {
    error("Too many arguments to crypto->pad()\n");
  }

  for (i=PIKE_CRYPTO->backlog_len; i < (PIKE_CRYPTO->block_size - 1); i++) {
    PIKE_CRYPTO->backlog[i] = my_rand() & 0xff;
  }
  PIKE_CRYPTO->backlog[i] = PIKE_CRYPTO->backlog_len;

  push_string(make_shared_binary_string((const char *)PIKE_CRYPTO->backlog,
					PIKE_CRYPTO->block_size));

  MEMSET(PIKE_CRYPTO->backlog, 0, PIKE_CRYPTO->block_size);
  PIKE_CRYPTO->backlog_len = 0;

  safe_apply(PIKE_CRYPTO->object, "crypt_block", 1);
}

/* string unpad(string) */
static void f_unpad(INT32 args)
{
  int len;
  struct pike_string *str;

  if (args != 1) {
    error("Wrong number of arguments to crypto->unpad()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to crypto->unpad()\n");
  }

  str = sp[-1].u.string;

  len = str->len;
  
  len += str->str[len - 1] - PIKE_CRYPTO->block_size;

  if (len < 0) {
    error("crypto->unpad(): String to short to unpad\n");
  }
  
  str->refs++;

  pop_stack();

  push_string(make_shared_binary_string(str->str, len));

  free_string(str);
}

/*
 * Module linkage
 */

void init_module_efuns(void)
{
  /* add_efun()s */

  add_efun("string_to_hex", f_string_to_hex, "function(string:string)", OPT_TRY_OPTIMIZE);
  add_efun("hex_to_string", f_hex_to_string, "function(string:string)", OPT_TRY_OPTIMIZE);

  init_md2_efuns();
  init_md5_efuns();

  init_idea_efuns();
  init_des_efuns();
  init_invert_efuns();

  init_cbc_efuns();
  init_pipe_efuns();
}

void init_module_programs(void)
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
  add_storage(sizeof(struct pike_crypto));

  add_function("create", f_create, "function(program|object:void)", OPT_EXTERNAL_DEPEND);

  add_function("query_block_size", f_query_block_size, "function(void:int)", OPT_TRY_OPTIMIZE);
  add_function("query_key_length", f_query_key_length, "function(void:int)", OPT_TRY_OPTIMIZE);

  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:void)", OPT_SIDE_EFFECT);
  add_function("crypt", f_crypt, "function(string:string)", OPT_EXTERNAL_DEPEND);

  add_function("pad", f_pad, "function(void:string)", OPT_EXTERNAL_DEPEND);
  add_function("unpad", f_unpad, "function(string:string)", OPT_EXTERNAL_DEPEND);

  set_init_callback(init_pike_crypto);
  set_exit_callback(exit_pike_crypto);

  pike_crypto_program = end_c_program("/precompiled/crypto");
  pike_crypto_program->refs++;

  init_md2_programs();
  init_md5_programs();

  init_idea_programs();
  init_des_programs();
  init_invert_programs();

  init_cbc_programs();
  init_pipe_programs();
}

void exit_module(void)
{
  /* free_program()s */
  exit_md2();
  exit_md5();

  exit_idea();
  exit_des();
  exit_invert();

  exit_cbc();
  exit_pipe();
}

