/*
 * $Id: crypto.c,v 1.19 1997/03/23 18:18:55 nisse Exp $
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
#include "pike_macros.h"
#include "threads.h"
#include "object.h"
#include "interpret.h"
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

/* Module specific includes */
#include "precompiled_crypto.h"

struct pike_crypto {
  struct object *object;
  INT32 block_size;
  INT32 backlog_len;
  unsigned char *backlog;
};

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

#define THIS	((struct pike_crypto *)(fp->current_storage))

static struct program *pike_crypto_program;

/*
 * Functions
 */

static void init_pike_crypto(struct object *o)
{
  memset(THIS, 0, sizeof(struct pike_crypto));
}

static void exit_pike_crypto(struct object *o)
{
  if (THIS->object) {
    free_object(THIS->object);
  }
  if (THIS->backlog) {
    MEMSET(THIS->backlog, 0, THIS->block_size);
    free(THIS->backlog);
  }
  memset(THIS, 0, sizeof(struct pike_crypto));
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
    if (find_identifier( (char *) *requiered, p) < 0) {
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
  struct pike_string *s;
  INT32 i;

  if (args != 1) {
    error("Wrong number of arguments to string_to_hex()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to string_to_hex()\n");
  }

  s = begin_shared_string(2 * sp[-1].u.string->len);
  
  for (i=0; i<sp[-1].u.string->len; i++) {
    sprintf(s->str + i*2, "%02x", sp[-1].u.string->str[i] & 0xff);
  }
  
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

/* string hex_to_string(string) */
static void f_hex_to_string(INT32 args)
{
  struct pike_string *s;
  INT32 i;

  if (args != 1) {
    error("Wrong number of arguments to hex_to_string()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to hex_to_string()\n");
  }
  if (sp[-1].u.string->len & 1) {
    error("Bad string length to hex_to_string()\n");
  }

  s = begin_shared_string(sp[-1].u.string->len/2);
  for (i=0; i*2<sp[-1].u.string->len; i++)
  {
    switch (sp[-1].u.string->str[i*2])
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      s->str[i] = (sp[-1].u.string->str[i*2] - '0')<<4;
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      s->str[i] = (sp[-1].u.string->str[i*2] + 10 - 'A')<<4;
      break;
    default:
      free_string(end_shared_string(s));
      error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2] & 0xff);
    }
    switch (sp[-1].u.string->str[i*2+1])
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      s->str[i] |= sp[-1].u.string->str[i*2+1] - '0';
      break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      s->str[i] |= (sp[-1].u.string->str[i*2+1] + 10 - 'A') & 0x0f;
      break;
    default:
      free_string(end_shared_string(s));
      error("hex_to_string(): Illegal character (0x%02x) in string\n",
	    sp[-1].u.string->str[i*2+1] & 0xff);
    }
  }
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

static INLINE unsigned INT8 parity(unsigned INT8 c)
{
  c ^= (c >> 4);
  c ^= (c >> 2);
  c ^= (c >> 1);
  return c & 1;
}

static void f_des_parity(INT32 args)
{
  struct pike_string *s;
  int i;
  if (args != 1) {
    error("Wrong number of arguments to des_parity()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to des_parity()\n");
  }

  s = begin_shared_string(sp[-1].u.string->len);
  MEMCPY(s->str, sp[-1].u.string->str, s->len);
  
  for (i=0; i< s->len; i++)
    s->str[i] ^= ! parity(s->str[i]);
  pop_n_elems(args);
  push_string(end_shared_string(s));
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
    THIS->object = clone_object(sp[-args].u.program, args-1);
  } else {
    if (args != 1) {
      error("Too many arguments to crypto->create()\n");
    }
    THIS->object = sp[-args].u.object;
    THIS->object->refs++;
  }
  pop_stack(); /* Just one element left on the stack in both cases */

  check_functions(THIS->object, crypto_functions);

  safe_apply(THIS->object, "query_block_size", 0);

  if (sp[-1].type != T_INT) {
    error("crypto->create(): query_block_size() didn't return an int\n");
  }
  THIS->block_size = sp[-1].u.integer;

  pop_stack();

  if ((!THIS->block_size) ||
      (THIS->block_size > 4096)) {
    error("crypto->create(): Bad block size %d\n", THIS->block_size);
  }

  THIS->backlog = (unsigned char *)xalloc(THIS->block_size);
  THIS->backlog_len = 0;
  MEMSET(THIS->backlog, 0, THIS->block_size);
}

/* int query_block_size(void) */
static void f_query_block_size(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->block_size);
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  safe_apply(THIS->object, "query_key_length", args);
}

/* void set_encrypt_key(INT32 args) */
static void f_set_encrypt_key(INT32 args)
{
  if (THIS->block_size) {
    MEMSET(THIS->backlog, 0, THIS->block_size);
    THIS->backlog_len = 0;
  } else {
    error("crypto->set_encrypt_key(): Object has not been created yet\n");
  }
  safe_apply(THIS->object, "set_encrypt_key", args);
  pop_stack();
  push_object(this_object());
}

/* void set_decrypt_key(INT32 args) */
static void f_set_decrypt_key(INT32 args)
{
  if (THIS->block_size) {
    MEMSET(THIS->backlog, 0, THIS->block_size);
    THIS->backlog_len = 0;
  } else {
    error("crypto->set_decrypt_key(): Object has not been created yet\n");
  }
  safe_apply(THIS->object, "set_decrypt_key", args);
  pop_stack();
  push_object(this_object());
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
  if (!(result = alloca(sp[-1].u.string->len + THIS->block_size))) {
    error("crypto->crypt(): Out of memory\n");
  }
  if (THIS->backlog_len) {
    if (sp[-1].u.string->len >=
	(THIS->block_size - THIS->backlog_len)) {
      MEMCPY(THIS->backlog + THIS->backlog_len,
	     sp[-1].u.string->str,
	     (THIS->block_size - THIS->backlog_len));
      soffset += (THIS->block_size - THIS->backlog_len);
      THIS->backlog_len = 0;
      push_string(make_shared_binary_string((char *)THIS->backlog,
					    THIS->block_size));
      safe_apply(THIS->object, "crypt_block", 1);
      if (sp[-1].type != T_STRING) {
	error("crypto->crypt(): crypt_block() did not return string\n");
      }
      if (sp[-1].u.string->len != THIS->block_size) {
	error("crypto->crypt(): Unexpected string length %d\n",
	      sp[-1].u.string->len);
      }
	
      MEMCPY(result, sp[-1].u.string->str, THIS->block_size);
      roffset = THIS->block_size;
      pop_stack();
      MEMSET(THIS->backlog, 0, THIS->block_size);
    } else {
      MEMCPY(THIS->backlog + THIS->backlog_len,
	     sp[-1].u.string->str, sp[-1].u.string->len);
      THIS->backlog_len += sp[-1].u.string->len;
      pop_n_elems(args);
      push_string(make_shared_binary_string("", 0));
      return;
    }
  }
  
  len = (sp[-1].u.string->len - soffset);
  len -= len % THIS->block_size;

  if (len) {
    push_string(make_shared_binary_string(sp[-1].u.string->str + soffset, len));
    soffset += len;

    safe_apply(THIS->object, "crypt_block", 1);

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
    MEMCPY(THIS->backlog, sp[-1].u.string->str + soffset,
	   sp[-1].u.string->len - soffset);
    THIS->backlog_len = sp[-1].u.string->len - soffset;
  }

  pop_n_elems(args);

  push_string(make_shared_binary_string((char *)result, roffset + len));
  MEMSET(result, 0, roffset + len);
}

/* string pad(void) */
static void f_pad(INT32 args)
{
  int i;
  int len;
  
  if (args) {
    error("Too many arguments to crypto->pad()\n");
  }

  len = THIS->block_size - 1 - THIS->backlog_len;
  for (i=0; i < len; i++)
    THIS->backlog[THIS->backlog_len + i] = my_rand() & 0xff;
  
  THIS->backlog[i] = len;

  push_string(make_shared_binary_string((const char *)THIS->backlog,
					THIS->block_size));

  MEMSET(THIS->backlog, 0, THIS->block_size);
  THIS->backlog_len = 0;

  safe_apply(THIS->object, "crypt_block", 1);
}

/* string unpad(string) */
static void f_unpad(INT32 args)
{
  int len;
  struct pike_string *str;

  if (args != 1) 
    error("Wrong number of arguments to crypto->unpad()\n");
  
  if (sp[-1].type != T_STRING) 
    error("Bad argument 1 to crypto->unpad()\n");
  
  str = sp[-1].u.string;
  len = str->len;

  if (str->str[len - 1] > (THIS->block_size - 1))
    error("crypto->unpad(): Invalid padding\n");

  len -= (str->str[len - 1] + 1);

  if (len < 0) 
    error("crypto->unpad(): String to short to unpad\n");
  
  str->refs++;
  pop_stack();
  push_string(make_shared_binary_string(str->str, len));
  free_string(str);
}

/*
 * Module linkage
 */

#if 0
void MOD_INIT2(crypto)(void)
{
  /* add_efun()s */

#if 0
  MOD_INIT2(md2)();
  MOD_INIT2(md5)();
#endif
#if 1
  MOD_INIT2(idea)();
  MOD_INIT2(des)();
  MOD_INIT2(rc4)();
  MOD_INIT2(invert)();

  MOD_INIT2(sha)();
  MOD_INIT2(cbc)();
  MOD_INIT2(pipe)();
#endif
}
#endif

void MOD_INIT(crypto)(void)
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

  add_function("create", f_create, "function(program|object:void)", 0);

  add_function("query_block_size", f_query_block_size, "function(void:int)", 0);
  add_function("query_key_length", f_query_key_length, "function(void:int)", 0);

  add_function("set_encrypt_key", f_set_encrypt_key, "function(string:object)", 0);
  add_function("set_decrypt_key", f_set_decrypt_key, "function(string:object)", 0);
  add_function("crypt", f_crypt, "function(string:string)", 0);

  add_function("pad", f_pad, "function(void:string)", 0);
  add_function("unpad", f_unpad, "function(string:string)", 0);

  set_init_callback(init_pike_crypto);
  set_exit_callback(exit_pike_crypto);

  end_class(MODULE_PREFIX "crypto", 0);
}

void MOD_EXIT(crypto)(void) {}

void pike_module_init(void)
{
  add_function("string_to_hex", f_string_to_hex, "function(string:string)", 0);
  add_function("hex_to_string", f_hex_to_string, "function(string:string)", 0);
  add_function("des_parity", f_des_parity, "function(string:string)", 0);
#if 0
  MOD_INIT(md2)();
#endif
#if 1
  MOD_INIT(md5)();
  MOD_INIT(crypto)();
  MOD_INIT(idea)();
  MOD_INIT(des)();
  MOD_INIT(rc4)();
  MOD_INIT(invert)();

  MOD_INIT(sha)();
  MOD_INIT(cbc)();
  MOD_INIT(pipe)();
#endif
}  

void pike_module_exit(void)
{
  /* free_program()s */
#if 0
  MOD_EXIT(md2)();
#endif
#if 1
  MOD_EXIT(md5)();
  MOD_EXIT(crypto)();
  MOD_EXIT(idea)();
  MOD_EXIT(des)();
  MOD_EXIT(rc4)();
  MOD_EXIT(invert)();
  MOD_EXIT(sha)();
  MOD_EXIT(cbc)();
  MOD_EXIT(pipe)();
#endif
}

