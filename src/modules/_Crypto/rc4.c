/* rc4.c
 *
 * Written by Niels Möller
 */

#include "global.h"
#include "svalue.h"
#include "string.h"
#include "pike_types.h"
#include "stralloc.h"
#include "object.h"
#include "interpret.h"
#include "program.h"
#include "error.h"
#include "las.h"

#include "rc4.h"

RCSID("$Id: rc4.c,v 1.4 1997/03/14 22:17:11 nisse Exp $");

/* Module specific includes */
#include "precompiled_crypto.h"

#define THIS ((struct rc4_ctx *)(fp->current_storage))

struct program *pike_rc4_program;

void init_pike_rc4(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct rc4_ctx));
}

void exit_pike_rc4(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct rc4_ctx));
}

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) {
    error("Too many arguments to rc4->name()\n");
  }
  push_string(make_shared_string("RC4"));
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  if (args) {
    error("Too many arguments to rc4->query_key_length()\n");
  }
  push_int(0);
}

/* void set_key(string) */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of args to rc4->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to rc4->set_key()\n");
  }
  rc4_set_key(THIS, (unsigned INT8 *) sp[-1].u.string->str, sp[-1].u.string->len);

  pop_n_elems(args);
  push_object(this_object());
}

/* string crypt(string) */
static void f_crypt(INT32 args)
{
  int len;
  struct pike_string *s;
  
  if (args != 1) {
    error("Wrong number of arguments to rc4->crypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to rc4->crypt()\n");
  }

  len = sp[-1].u.string->len;

  s = begin_shared_string(len);
  rc4_crypt(THIS,
	    (unsigned INT8 *) s->str,
	    (unsigned INT8 *) sp[-1].u.string->str,
	    len);
  
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

void MOD_INIT2(rc4)(void)
{
  /* add_efun()s */
}

void MOD_INIT(rc4)(void)
{
  start_new_program();
  add_storage(sizeof(struct rc4_ctx));

  add_function("name", f_name, "function(void:string)", 0);
  add_function("query_key_length", f_query_key_length, "function(void:int)", 0);
  add_function("set_encrypt_key", f_set_key, "function(string:object)", 0);
  add_function("set_decrypt_key", f_set_key, "function(string:object)", 0);
  add_function("crypt", f_crypt, "function(string:string)", 0);

  set_init_callback(init_pike_rc4);
  set_exit_callback(exit_pike_rc4);

  end_class(MODULE_PREFIX "rc4", 0);
}

void MOD_EXIT(rc4)(void)
{
}
