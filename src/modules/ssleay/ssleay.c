/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/

#include "global.h"
RCSID("$Id: ssleay.c,v 1.9 1997/02/07 00:45:45 hubbe Exp $");
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "macros.h"
#include "backend.h"
#include "program.h"
#include "threads.h"

#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#ifdef HAVE_SSLEAY

#include <ssl.h>
#include <crypto.h>
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
  if (args < 1)
    error("ssleay_connection->create: no context given\n");
  if ((sp[-args].type != T_OBJECT)
      || (sp[-args].u.object->prog != ssleay_program))
    error("ssleay_connection->create: invalid argument\n");
  if (CON)
    SSL_free(CON);
  CON = SSL_new( ( (struct ssleay_context *) sp[-args].u.object->storage)
		 -> shared);
  if (!CON)
    {
      ERR_print_errors_fp(stderr);
      error("ssleay_connection->create: Could not allocate new connection\n");
    }
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
  int res;
  
  pop_n_elems(args);
  THREADS_ALLOW();
  res = SSL_accept(CON);
  THREADS_DISALLOW();
  push_int(res);
}

void ssleay_connection_read(INT32 args)
{
  struct pike_string *s;
  INT32 len;
  INT32 count;
  
  if ((args < 1) || (sp[-args].type != T_INT))
    error("ssleay_connection->read: wrong type\n");
  len = sp[-args].u.integer;

  if (len < 0)
    error("ssleay_connection->read: invalid argument\n");
  pop_n_elems(args);

  s = begin_shared_string(len);
  if (len)
    {
      THREADS_ALLOW();
      count = SSL_read(CON, s->str, len);
      THREADS_DISALLOW();
      if (count < 0)
	{
	  free_string(end_shared_string(s));
	  push_int(count);
	}
      else
	{
	  s->len = count;
	  push_string(end_shared_string(s));
	}
    }
}

void ssleay_connection_write(INT32 args)
{
  INT32 res;

  if ((args < 1) || (sp[-args].type != T_STRING))
    error("ssleay_connection->write: wrong argument\n");
  THREADS_ALLOW();
  res = SSL_write(CON, sp[-args].u.string->str, sp[-args].u.string->len);
  THREADS_DISALLOW();
  pop_n_elems(args);
  push_int(res);
}

void ssleay_connection_werror(INT32 args)
{
  ERR_print_errors_fp(stderr);
  pop_n_elems(args);
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
  if (SSL_CTX_use_certificate_file(CTX, sp[-args].u.string->str, SSL_FILETYPE_PEM) <= 0)
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
  if (SSL_CTX_use_PrivateKey_file(CTX, sp[-args].u.string->str, SSL_FILETYPE_PEM) <= 0)
    {
      ERR_print_errors_fp(stderr);
      error("ssleay->use_private_key_file: unable to use private_key\n");
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
  pop_n_elems(args);
  THISOBJ->refs++;
  push_object(THISOBJ);
  push_object(clone(ssleay_connection_program, 1));
}


/* Thread stuff */
#ifdef _REENTRANT

static MUTEX_T ssleay_locks[CRYPTO_NUM_LOCKS];

static void ssleay_locking_callback(int mode, int type, char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    mt_lock(ssleay_locks + type);
  else
    mt_unlock(ssleay_locks + type);
}

static unsigned long ssleay_thread_id(void)
{
  return th_self();
}
      
static void ssleay_init_threads()
{
  int i;
  for (i = 0; i<CRYPTO_NUM_LOCKS; i++)
    mt_init(ssleay_locks + i);
  CRYPTO_set_id_callback(ssleay_thread_id);
  CRYPTO_set_locking_callback(ssleay_locking_callback);
}
#endif /* _REENTRANT */

/* Initializing etc */
void init_context(struct object *o)
{
  CTX = NULL;
}

void exit_context(struct object *o)
{
  if (CTX)
    SSL_CTX_free(CTX);
}

void init_connection(struct object *o)
{
  CON = NULL;
}

void exit_connection(struct object *o)
{
  if (CON)
    SSL_free(CON);
}

#endif /* HAVE_SSLEAY */

void pike_module_exit()
{
#ifdef HAVE_SSLEAY
  free_program(ssleay_connection_program);
  free_program(ssleay_program);
  ssleay_connection_program=0;
  ssleay_program=0;
#endif
}

void pike_module_init(void)
{
#ifdef HAVE_SSLEAY
  ERR_load_ERR_strings();
  ERR_load_SSL_strings();
  ERR_load_crypto_strings();
#ifdef _REENTRANT
  ssleay_init_threads();
#endif /* _REENTRANT */
  start_new_program();
  add_storage(sizeof(struct ssleay_context));

  add_function("create", ssleay_create, "function(void:void)",0);
  add_function("use_certificate_file", ssleay_use_certificate_file, "function(string:void)", 0);
  add_function("use_private_key_file", ssleay_use_private_key_file, "function(string:void)", 0);
  add_function("new", ssleay_new, "function(void:object)", 0);

  set_init_callback(init_context);
  set_exit_callback(exit_context);

  ssleay_program=end_program();
  add_program_constant("ssleay",ssleay_program);
  
  start_new_program();
  add_storage(sizeof(struct ssleay_connection));

  add_function("create", ssleay_connection_create, "function(object:void)",0);
  add_function("accept", ssleay_connection_accept, "function(void:int)", 0);
  add_function("read",ssleay_connection_read,"function(int,int|void:int|string)",0);
  add_function("write",ssleay_connection_write,"function(string:int)",0);
  add_function("set_fd",ssleay_connection_set_fd,"function(int:void)",0);
  add_function("ssleay_werror", ssleay_connection_werror, "function(void:void)", 0);
  set_init_callback(init_connection);
  set_exit_callback(exit_connection);

  ssleay_connection_program=end_program();
  add_program_constant("connection",ssleay_program);
#endif /* HAVE_SSLEAY */
}


