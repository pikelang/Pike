/* arcfour.c
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

#include "arcfour.h"

RCSID("$Id: arcfour.c,v 1.10 2000/03/28 12:16:08 grubba Exp $");

#undef THIS
#define THIS ((struct arcfour_ctx *)(fp->current_storage))

struct program *pike_arcfour_program;

void init_pike_arcfour(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct arcfour_ctx));
}

void exit_pike_arcfour(struct object *o)
{
  MEMSET(THIS, 0, sizeof(struct arcfour_ctx));
}

/* string name(void) */
static void f_name(INT32 args)
{
  if (args) {
    error("Too many arguments to arcfour->name()\n");
  }
  push_string(make_shared_string("ARCFOUR"));
}

/* int query_key_length(void) */
static void f_query_key_length(INT32 args)
{
  if (args) {
    error("Too many arguments to arcfour->query_key_length()\n");
  }
  push_int(0);
}

/* void set_key(string) */
static void f_set_key(INT32 args)
{
  if (args != 1) {
    error("Wrong number of args to arcfour->set_key()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to arcfour->set_key()\n");
  }
  if (!sp[-1].u.string->len)
    error("Empty key to arcfour_set_key()\n");
  arcfour_set_key(THIS, (unsigned INT8 *) sp[-1].u.string->str, sp[-1].u.string->len);

  pop_n_elems(args);
  push_object(this_object());
}

/* string crypt(string) */
static void f_crypt(INT32 args)
{
  int len;
  struct pike_string *s;
  
  if (args != 1) {
    error("Wrong number of arguments to arcfour->crypt()\n");
  }
  if (sp[-1].type != T_STRING) {
    error("Bad argument 1 to arcfour->crypt()\n");
  }

  len = sp[-1].u.string->len;

  s = begin_shared_string(len);
  arcfour_crypt(THIS,
	    (unsigned INT8 *) s->str,
	    (unsigned INT8 *) sp[-1].u.string->str,
	    len);
  
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

void pike_arcfour_init(void)
{
  start_new_program();
  ADD_STORAGE(struct arcfour_ctx);

  /* function(void:string) */
  ADD_FUNCTION("name", f_name,tFunc(tVoid,tStr), 0);
  /* function(void:int) */
  ADD_FUNCTION("query_key_length", f_query_key_length,tFunc(tVoid,tInt), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_encrypt_key", f_set_key,tFunc(tStr,tObj), 0);
  /* function(string:object) */
  ADD_FUNCTION("set_decrypt_key", f_set_key,tFunc(tStr,tObj), 0);
  /* function(string:string) */
  ADD_FUNCTION("crypt", f_crypt,tFunc(tStr,tStr), 0);

  set_init_callback(init_pike_arcfour);
  set_exit_callback(exit_pike_arcfour);

  end_class("arcfour", 0);
}

void pike_arcfour_exit(void)
{
}
