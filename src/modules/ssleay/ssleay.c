/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"
RCSID("$Id: ssleay.c,v 1.2 1996/12/01 19:51:12 grubba Exp $");
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "macros.h"
#include "backend.h"
#include "program.h"

#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#ifdef HAVE_SSLEAY

#include <ssl.h>
#include <pem.h>
#include <err.h>
#include <x509.h>

/* SSLeay defines _ as a macro. That's Puckat(TM). */
#undef _

struct ssleay_context
{
  SSL_CTX *shared;
};

struct ssleay_connection
{
  SSL *con;
};


#define THISOBJ (fp->current_object)
#define CON (((struct ssleay_connection *) (fp->current_storage))->con)
#define CTX (((struct ssleay_context *) (fp->current_storage))->shared)

#endif /* HAVE_SSLEAY */

static struct program *ssleay_program;
static struct program *ssleay_connection_program;

#ifdef HAVE_SSLEAY

/* Methods for ssleay_connection objects */

/* Arg is an ssleay object */
void ssleay_connection_create(INT32 args)
{
  if ((args < 1)
      || (sp[-args].type != T_OBJECT)
      || (sp[-args].u.object->prog != ssleay_program))
    error("ssleay_connection->create: invalid argument\n");
  if (CON)
    SSL_free(CON);
  CON = SSL_new( ( (struct ssleay_context *) sp[-args].u.object->storage)
		 -> shared);
  if (!CON)
    error("ssleay_connection->create: Could not allocate new connection\n");
  SSL_clear(CON);
}

void ssleay_connection_set_fd(INT32 args)
{
  if ((args < 1) || (sp[-args].type != T_INT))
    error("ssleay_connection->set_fd: wrong type\n");
  SSL_set_fd(CON, sp[-args].u.integer);
  pop_n_elems(args);
}

void ssleay_connection_accept(INT32 args)
{
  if (SSL_accept(CON) <= 0)
    {
      ERR_print_errors_fp(stderr);
      error("ssleay_connection->accept: failed");
    }
  pop_n_elems(args);
}

void ssleay_connection_read(INT32 args)
{
  struct pike_string *s;
  INT32 len;

  if ((args < 1) || (sp[-args].type != T_INT))
    error("ssleay_connection->read: wrong type\n");
  len = sp[-args].u.integer;

  if (len < 0)
    error("ssleay_connection->read: invalid argument\n");
  s = begin_shared_string(len);
  if (len)
    {
      s->len = SSL_read(CON, s->str, len);
    }
  pop_n_elems(args);
  push_string(end_shared_string(s));
}

void ssleay_connection_write(INT32 args)
{
  INT32 res;

  if ((args < 1) || (sp[-args].type != T_STRING))
    error("ssleay_connection->write: wrong argument\n");
  res = SSL_write(CON, sp[-args].u.string->str, sp[-args].u.string->len);
  pop_n_elems(args);
  push_int(res);
}

/* Methods for ssleay context objects */

static void ssleay_create(INT32 args)
{
  if (CTX)
    SSL_CTX_free(CTX);
  CTX = SSL_CTX_new();
  if (!CTX)
    error("ssleay->create: couldn't allocate new ssl context\n");
  pop_n_elems(args);
}

static void ssleay_use_certificate_file(INT32 args)
{
  if (sp[-args].type != T_STRING)
    error("ssleay->use_certificate_file: wrong type");
  if (SSL_CTX_use_certificate_file(CTX, sp[-args].u.string->str, SSL_FILETYPE_PEM))
{
ERR_print_errors_fp(stderr);
      error("ssleay->use_certificate_file: unable to use certificate");
    }
  pop_n_elems(args);
}

static void ssleay_use_private_key_file(INT32 args)
{
  if (sp[-args].type != T_STRING)
    error("ssleay->use_private_key_file: wrong type");
  if (SSL_CTX_use_PrivateKey_file(CTX, sp[-args].u.string->str, SSL_FILETYPE_PEM))
    {
      ERR_print_errors_fp(stderr);
      error("ssleay->use_private_key_file: unable to use private_key");
    }
  pop_n_elems(args);
}

/* open(int fd, string mode) where mode is "s" for server
 *  or "c" for client */
static void ssleay_new(INT32 args)
{
  struct object *res;
  
#if 0
  if (strcmp(sp[-args+1].u.string->str, "s") == 0)
    is_server = 1;
  else
    {
      if (strcmp(sp[-args+1].u.string->str, "c") == 0)
	error("ssleay->open: client mode not implemented\n")
      else
	error("ssleay->open: invalid mode\n");
    }
#endif
  pop_n_elems(args );
  push_object(THISOBJ);
  push_object(clone(ssleay_connection_program, 1));
}

/* Initializing etc */
void init_context(struct object *o)
{
  CTX = NULL;
}

void exit_context(struct object *o) {}

void init_connection(struct object *o)
{
  CON = NULL;
}

void exit_connection(struct object *o) {}

void init_ssleay_efuns(void)
{
  add_efun("ssleay", ssleay_create, "function(void:object)", 0);
}

void exit_ssleay()
{
  free_program(ssleay_connection_program);
  free_program(ssleay_program);
}

#endif /* HAVE_SSLEAY */

void init_ssleay_programs(void)
{
  start_new_program();
#ifdef HAVE_SSLEAY
  add_storage(sizeof(struct ssleay_context));

  add_function("use_certificate_file", ssleay_use_certificate_file, "function(string:void)", 0);
  add_function("use_private_key_file", ssleay_use_private_key_file, "function(string:void)", 0);
  add_function("new", ssleay_new, "function(void:object)", 0);

  set_init_callback(init_context);
  set_exit_callback(exit_context);
#endif /* HAVE_SSLEAY */

  ssleay_program=end_c_program("/precompiled/ssleay");
  ssleay_program->refs++;
  
  start_new_program();
#ifdef HAVE_SSLEAY
  add_storage(sizeof(struct ssleay_connection));

  add_function("create", ssleay_connection_create, "function(int:void)",0);
  add_function("accept", ssleay_connection_accept, "function(void:void)", 0);
  add_function("read",ssleay_connection_read,"function(int,int|void:int|string)",0);
  add_function("write",ssleay_connection_write,"function(string:int)",0);
  add_function("set_fd",ssleay_connection_set_fd,"function(int:void)",0);
  set_init_callback(init_connection);
  set_exit_callback(exit_connection);
#endif /* HAVE_SSLEAY */
  ssleay_connection_program=end_c_program("/precompiled/ssleay_connection");
  ssleay_connection_program->refs++;
}


