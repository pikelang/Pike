/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: socket.c,v 1.88 2004/05/13 15:35:32 bill Exp $
*/

#define NO_PIKE_SHORTHAND

#include "global.h"
#include "fdlib.h"
#include "pike_netlib.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "pike_macros.h"
#include "backend.h"
#include "fd_control.h"
#include "threads.h"
#include "program_id.h"

#include "file_machine.h"
#include "file.h"

RCSID("$Id: socket.c,v 1.88 2004/05/13 15:35:32 bill Exp $");

#ifdef HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_STREAM_H
#include <sys/stream.h>

/* Ugly patch for AIX 3.2 */
#ifdef u
#undef u
#endif

#endif

#ifdef HAVE_SYS_PROTOSW_H
#include <sys/protosw.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#include "dmalloc.h"

/*! @module Stdio
 */

/*! @class Port
 */

struct port
{
  struct fd_callback_box box;	/* Must be first. */
  int my_errno;
  struct svalue accept_callback; /* Mapped. */
  struct svalue id;		/* Mapped. */
};

#undef THIS
#define THIS ((struct port *)(Pike_fp->current_storage))

static int got_port_event (struct fd_callback_box *box, int event)
{
  struct port *p = (struct port *) box;
#ifdef PIKE_DEBUG
#ifndef __NT__
  if(!query_nonblocking(p->box.fd))
    Pike_fatal("Port is in blocking mode in port accept callback!!!\n");
#endif
  if (event != PIKE_FD_READ)
    Pike_fatal ("Got unexpected event %d.\n", event);
#endif

  p->my_errno = errno;		/* Propagate backend setting. */
  push_svalue (&p->id);
  apply_svalue(& p->accept_callback, 1);
  pop_stack();
  return 0;
}

static void assign_accept_cb (struct port *p, struct svalue *cb)
{
  assign_svalue(& p->accept_callback, cb);
  if (UNSAFE_IS_ZERO (cb)) {
    if (p->box.backend)
      set_fd_callback_events (&p->box, 0);
  }
  else {
    if (!p->box.backend)
      INIT_FD_CALLBACK_BOX (&p->box, default_backend, p->box.ref_obj,
			    p->box.fd, PIKE_BIT_FD_READ, got_port_event);
    else
      set_fd_callback_events (&p->box, PIKE_BIT_FD_READ);
    set_nonblocking(p->box.fd,1);
  }
}

static void do_close(struct port *p)
{
  retry:
  if(p->box.fd >= 0)
  {
    if(fd_close(p->box.fd) < 0)
      if(errno == EINTR) {
	check_threads_etc();
	goto retry;
      }
    change_fd_for_box (&p->box, -1);
  }
}

/*! @decl mixed set_id(mixed id)
 *!
 *! This function sets the id used for accept_callback by this port.
 *! The default id is @[this_object()].
 *!
 *! @seealso
 *!   @[query_id]
 */
static void port_set_id(INT32 args)
{
  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Port->set_id", 1);

  assign_svalue(& THIS->id, Pike_sp-args);
  pop_n_elems(args-1);
}

/*! @decl mixed query_id()
 *!
 *! This function returns the id for this port. The id is normally the
 *! first argument to accept_callback.
 *!
 *! @seealso
 *!   @[set_id]
 */
static void port_query_id(INT32 args)
{
  pop_n_elems(args);
  assign_svalue_no_free(Pike_sp++,& THIS->id);
}

/*! @decl int errno()
 *!
 *! If the last call done on this port failed, errno will return an
 *! integer describing what went wrong. Refer to your unix manual for
 *! further information.
 */
static void port_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->my_errno);
}

/*! @decl int listen_fd(int fd, void|function accept_callback)
 *!
 *! This function does the same as port->bind, except that instead
 *! of creating a new socket and bind it to a port, it expects that
 *! the filedescriptor 'fd' is an already open port.
 *!
 *! @note
 *!  This function is only for the advanced user, and is generally used
 *!  when sockets are passed to Pike at exec time.
 *!
 *! @seealso
 *!   @[bind], @[accept]
 */
static void port_listen_fd(INT32 args)
{
  struct port *p = THIS;
  int fd;
  do_close(p);

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Port->bind_fd", 1);

  if(Pike_sp[-args].type != PIKE_T_INT)
    SIMPLE_BAD_ARG_ERROR("Port->bind_fd", 1, "int");

  fd=Pike_sp[-args].u.integer;

  if(fd<0)
  {
    errno = p->my_errno=EBADF;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if(fd_listen(fd, 16384) < 0)
  {
    p->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  change_fd_for_box (&p->box, fd);
  if(args > 1) assign_accept_cb (p, Pike_sp+1-args);
  p->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}

/*! @decl int bind(int|string port, void|function accept_callback, void|string ip)
 *!
 *! Bind opens a socket and binds it to port number on the local machine.
 *! If the second argument is present, the socket is set to nonblocking
 *! and the callback funcition is called whenever something connects to
 *! it. The callback will receive the id for this port as argument.
 *! Bind returns 1 on success, and zero on failiure.
 *! 
 *! If the optional argument 'ip' is given, bind will try to bind to
 *! this ip name or number.
 *!
 *! @seealso
 *!   @[accept]
 */
static void port_bind(INT32 args)
{
  struct port *p = THIS;
  PIKE_SOCKADDR addr;
  int addr_len,fd,tmp;

  do_close(p);

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Port->bind", 1);

  if(Pike_sp[-args].type != PIKE_T_INT &&
     (Pike_sp[-args].type != PIKE_T_STRING ||
      Pike_sp[-args].u.string->size_shift))
    SIMPLE_BAD_ARG_ERROR("Port->bind", 1, "int|string (8bit)");

  addr_len = get_inet_addr(&addr, (args > 2 && Pike_sp[2-args].type==PIKE_T_STRING?
				   Pike_sp[2-args].u.string->str : NULL),
			   (Pike_sp[-args].type == PIKE_T_STRING?
			    Pike_sp[-args].u.string->str : NULL),
			   (Pike_sp[-args].type == PIKE_T_INT?
			    Pike_sp[-args].u.integer : -1), 0);

  fd=fd_socket(SOCKADDR_FAMILY(addr), SOCK_STREAM, 0);

  if(fd < 0)
  {
    p->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

#ifndef __NT__
  {
    int o=1;
    if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0)
    {
      p->my_errno=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = p->my_errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }
#endif

  my_set_close_on_exec(fd,1);

  THREADS_ALLOW_UID();
  tmp=fd_bind(fd, (struct sockaddr *)&addr, addr_len) < 0 || fd_listen(fd, 16384) < 0;
  THREADS_DISALLOW_UID();

  if(tmp)
  {
    p->my_errno=errno;
    while (fd_close(fd) && errno == EINTR) {}
    errno = p->my_errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  change_fd_for_box (&p->box, fd);
  if(args > 1) assign_accept_cb (p, Pike_sp+1-args);
  p->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}


#ifdef HAVE_SYS_UN_H

/*! @decl int bind_unix(string path, void|function accept_callback)
 *!
 *! Bind opens a Unix domain socket at the filesystem location path.
 *! If the second argument is present, the socket is set to nonblocking
 *! and the callback funcition is called whenever something connects to
 *! it. The callback will receive the id for this port as argument.
 *!
 *! @returns
 *!    1 on success, and zero on failiure.
 *!
 *! @note
 *!   this function is only available on systems that support Unix domain
 *!   sockets.
 *!
 *! @seealso
 *!   @[accept]
 */
static void unix_bind(INT32 args)
{
  struct port *p = THIS;
  struct sockaddr_un addr;
  int addr_len,fd,tmp;

  do_close(p);

  if(args < 1)
    SIMPLE_TOO_FEW_ARGS_ERROR("Port->bind", 1);

  if((Pike_sp[-args].type != PIKE_T_STRING ||
      Pike_sp[-args].u.string->size_shift))
    SIMPLE_BAD_ARG_ERROR("Port->bind", 1, "string (8bit)");

  tmp = sizeof(addr.sun_path);

  if(Pike_sp[-args].u.string->len > tmp)
    SIMPLE_BAD_ARG_ERROR("Port->Bind", 1, "path too long");

  strcpy(addr.sun_path, Pike_sp[-args].u.string->str);
  addr.sun_family = AF_UNIX;

  addr_len = sizeof(struct sockaddr_un);

  fd=fd_socket(AF_UNIX, SOCK_STREAM, 0);

  if(fd < 0)
  {
    p->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

#ifndef __NT__
  {
    int o=1;
    if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0)
    {
      p->my_errno=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = p->my_errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }
#endif

  my_set_close_on_exec(fd,1);

  THREADS_ALLOW_UID();
  tmp=fd_bind(fd, (struct sockaddr *)&addr, addr_len) < 0 || fd_listen(fd, 16384) < 0;
  THREADS_DISALLOW_UID();

  if(tmp)
  {
    p->my_errno=errno;
    while (fd_close(fd) && errno == EINTR) {}
    errno = p->my_errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  change_fd_for_box (&p->box, fd);
  if(args > 1) assign_accept_cb (p, Pike_sp+1-args);
  p->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}

#endif /* HAVE_SYS_UN_H */

/*! @decl void close()
 *!
 *! Closes the socket.
 */
static void port_close (INT32 args)
{
  pop_n_elems (args);
  do_close (THIS);
}

/* @decl void create("stdin", void|function accept_callback)
 * @decl void create(int|string port, void|function accept_callback, void|string ip)
 *
 * When create is called with 'stdin' as argument, a socket is created
 * out of the file descriptor 0. This is only useful if that actually
 * IS a socket to begin with. When create is called with an int or any
 * other string as first argument, it does the same as bind() would do
 * with the same arguments.
 */
static void port_create(INT32 args)
{
  if(args)
  {
    if(Pike_sp[-args].type == PIKE_T_INT ||
       (Pike_sp[-args].type == PIKE_T_STRING &&
	strcmp("stdin",Pike_sp[-args].u.string->str)))
    {
      port_bind(args); /* pops stack */
      return;
    }else{
      struct port *p = THIS;

      if(Pike_sp[-args].type != PIKE_T_STRING)
	SIMPLE_TOO_FEW_ARGS_ERROR("Port->create", 1);

      if(strcmp("stdin",Pike_sp[-args].u.string->str))
	Pike_error("port->create() called with string other than 'stdin'\n");

      do_close(p);
      change_fd_for_box (&p->box, 0);

      if(fd_listen(p->box.fd, 16384) < 0)
      {
	p->my_errno=errno;
      }else{
	if(args > 1) assign_accept_cb (p, Pike_sp+1-args);
      }
    }
  }
  pop_n_elems(args);
}

extern struct program *file_program;

/*! @decl Stdio.File accept()
 */

static void port_accept(INT32 args)
{
  PIKE_SOCKADDR addr;
  struct port *this=THIS;
  int fd, err;
  struct object *o;
  ACCEPT_SIZE_T len=0;

  if(this->box.fd < 0)
    Pike_error("port->accept(): Port not open.\n");

  THREADS_ALLOW();
  len=sizeof(addr);
  do {
    fd=fd_accept(this->box.fd, (struct sockaddr *)&addr, &len);
    err = errno;
  } while (fd < 0 && err == EINTR);
  THREADS_DISALLOW();

  if(fd < 0)
  {
    this->my_errno=errno = err;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  my_set_close_on_exec(fd,1);
  o=file_make_object_from_fd(fd,FILE_READ | FILE_WRITE, SOCKET_CAPABILITIES);

  pop_n_elems(args);
  push_object(o);
}

/*! @decl string query_address(string arg1)
 *!
 *! @fixme
 *!   Document this function.
 */
static void socket_query_address(INT32 args)
{
  PIKE_SOCKADDR addr;
  int i;
  char buffer[496];
#ifndef HAVE_INET_NTOP
  char *q;
#endif
  ACCEPT_SIZE_T len;

  if(THIS->box.fd <0)
    Pike_error("socket->query_address(): Socket not bound yet.\n");

  len=sizeof(addr);
  i=fd_getsockname(THIS->box.fd,(struct sockaddr *)&addr,&len);
  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr.ipv4))
  {
    THIS->my_errno=errno;
    push_int(0);
    return;
  }

#ifdef HAVE_INET_NTOP
  inet_ntop(SOCKADDR_FAMILY(addr), SOCKADDR_IN_ADDR(addr),
	    buffer, sizeof(buffer)-20);
#else
  q=inet_ntoa(*SOCKADDR_IN_ADDR(addr));
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
#endif
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.ipv4.sin_port)));

  push_text(buffer);
}

/*! @decl void set_backend (Pike.Backend backend)
 *!
 *! Set the backend used for the accept callback.
 *!
 *! @note
 *! The backend keeps a reference to this object as long as the port
 *! is accepting connections, but this object does not keep a
 *! reference to the backend.
 *!
 *! @seealso
 *!   @[query_backend]
 */
static void port_set_backend (INT32 args)
{
  struct port *p = THIS;
  struct Backend_struct *backend;

  if (!args)
    SIMPLE_TOO_FEW_ARGS_ERROR ("Stdio.Port->set_backend", 1);
  if (Pike_sp[-args].type != PIKE_T_OBJECT)
    SIMPLE_BAD_ARG_ERROR ("Stdio.Port->set_backend", 1, "object(Pike.Backend)");
  backend = (struct Backend_struct *)
    get_storage (Pike_sp[-args].u.object, Backend_program);
  if (!backend)
    SIMPLE_BAD_ARG_ERROR ("Stdio.Port->set_backend", 1, "object(Pike.Backend)");

  if (p->box.backend)
    change_backend_for_box (&p->box, backend);
  else
    INIT_FD_CALLBACK_BOX (&p->box, backend, p->box.ref_obj,
			  p->box.fd, 0, got_port_event);

  pop_n_elems (args - 1);
}

/*! @decl Pike.Backend query_backend()
 *!
 *! Return the backend used for the accept callback.
 *!
 *! @seealso
 *!   @[set_backend]
 */
static void port_query_backend (INT32 args)
{
  pop_n_elems (args);
  ref_push_object (get_backend_obj (THIS->box.backend ? THIS->box.backend :
				    default_backend));
}


static void init_port_struct(struct object *o)
{
  THIS->box.backend = NULL;
  THIS->box.ref_obj = o;
  THIS->box.fd = -1;
  THIS->my_errno=0;
  /* map_variable takes care of id and accept_callback. */
}

static void exit_port_struct(struct object *o)
{
  do_close(THIS);
  unhook_fd_callback_box (&THIS->box);
  /* map_variable takes care of id and accept_callback. */
}

/*! @endclass
 */

/*! @endmodule
 */

struct program *port_program = NULL;

void port_exit_program(void)
{
  free_program( port_program );
}

void port_setup_program(void)
{
  ptrdiff_t offset;
  START_NEW_PROGRAM_ID (STDIO_PORT);
  offset = ADD_STORAGE(struct port);
  map_variable("_accept_callback", "mixed", 0,
	       offset + OFFSETOF(port, accept_callback), PIKE_T_MIXED);
  map_variable("_id", "mixed", 0,
	       offset + OFFSETOF(port, id), PIKE_T_MIXED);
  /* function(int|string,void|mixed,void|string:int) */
  ADD_FUNCTION("bind", port_bind,
	       tFunc(tOr(tInt,tStr) tOr(tVoid,tMix) tOr(tVoid,tStr),tInt), 0);
#ifdef HAVE_SYS_UN_H
  /* function(int|string,void|mixed,void|string:int) */
  ADD_FUNCTION("bind_unix", unix_bind,
               tFunc(tStr tOr(tVoid,tMix),tInt), 0);
#endif /* HAVE_SYS_UN_H */
  ADD_FUNCTION("close",port_close,tFunc(tNone,tVoid),0);
  /* function(int,void|mixed:int) */
  ADD_FUNCTION("listen_fd",port_listen_fd,tFunc(tInt tOr(tVoid,tMix),tInt),0);
  /* function(mixed:mixed) */
  ADD_FUNCTION("set_id",port_set_id,tFunc(tMix,tMix),0);
  /* function(:mixed) */
  ADD_FUNCTION("query_id",port_query_id,tFunc(tNone,tMix),0);
  /* function(:string) */
  ADD_FUNCTION("query_address",socket_query_address,tFunc(tNone,tStr),0);
  /* function(:int) */
  ADD_FUNCTION("errno",port_errno,tFunc(tNone,tInt),0);
  /* function(:object) */
  ADD_FUNCTION("accept",port_accept,tFunc(tNone,tObjIs_STDIO_FD),0);
  /* function(void|string|int,void|mixed,void|string:void) */
  ADD_FUNCTION("create", port_create,
	       tFunc(tOr3(tVoid,tStr,tInt) tOr(tVoid,tMix) tOr(tVoid,tStr),
		     tVoid), 0);
  ADD_FUNCTION ("set_backend", port_set_backend, tFunc(tObj,tVoid), 0);
  ADD_FUNCTION ("query_backend", port_query_backend, tFunc(tVoid,tObj), 0);

  set_init_callback(init_port_struct);
  set_exit_callback(exit_port_struct);

  port_program = end_program();
  add_program_constant( "_port", port_program, 0 );
}

int fd_from_portobject( struct object *p )
{
  struct port *po = (struct port *)get_storage( p, port_program );
  if(!po) return -1;
  return po->box.fd;
}
