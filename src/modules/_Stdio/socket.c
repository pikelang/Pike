/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

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
#include "module_support.h"
#include "time_stuff.h"

#include "file_machine.h"
#include "file.h"

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
#endif

#ifdef HAVE_SYS_PROTOSW_H
#include <sys/protosw.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#if !defined(SOL_TCP) && defined(IPPROTO_TCP)
    /* SOL_TCP isn't defined in Solaris. */
#define SOL_TCP	IPPROTO_TCP
#endif

/*! @module _Stdio
 */

/*! @class _port
 */

struct port
{
  struct fd_callback_box box;	/* Must be first. */
  int my_errno;
  unsigned int immediate_cnt;
  struct svalue accept_callback; /* Mapped. */
  struct svalue id;		/* Mapped. */
};

#undef THIS
#define THIS ((struct port *)(Pike_fp->current_storage))

static int got_port_event (struct fd_callback_box *box, int DEBUGUSED(event))
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
  p->immediate_cnt++;
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
      set_fd_callback_events (&p->box, 0, 0);
    set_nonblocking(p->box.fd,0);
  }
  else {
    if (!p->box.backend)
      INIT_FD_CALLBACK_BOX (&p->box, default_backend, p->box.ref_obj,
			    p->box.fd, PIKE_BIT_FD_READ, got_port_event, 0);
    else
      set_fd_callback_events (&p->box, PIKE_BIT_FD_READ, 0);
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
 *! @note
 *!   In Pike 8.0 and earlier the default value was @expr{0@} (zero)
 *!   (even though it was documented as above).
 *!
 *! @seealso
 *!   @[query_id], @[set_accept_callback()]
 */
static void port_set_id(INT32 args)
{
  check_all_args(NULL, args, BIT_MIXED, 0);

  /* NB: Inlined code for PIKE_T_NO_REF_MIXED from object_lower_set_index(). */
  if (((TYPEOF(THIS->id) == T_OBJECT) || (TYPEOF(THIS->id) == T_FUNCTION)) &&
      (THIS->id.u.object == Pike_fp->current_object)) {
    /* Restore the reference so that assign_svalue() below can remove it. */
    add_ref(Pike_fp->current_object);
  }

  assign_svalue(& THIS->id, Pike_sp-args);

  if (((TYPEOF(THIS->id) == T_OBJECT) || (TYPEOF(THIS->id) == T_FUNCTION)) &&
      (THIS->id.u.object == Pike_fp->current_object)) {
    /* Remove the reference that assign_svalue() added above. */
    sub_ref(Pike_fp->current_object);
  }
  pop_n_elems(args-1);
}

/*! @decl void set_accept_callback(function|void accept_callback)
 *!
 *! Change or remove the accept callback.
 *!
 *! @param accept_callback
 *!   New accept callback.
 *!
 *! @seealso
 *!   @[bind()], @[listen()], @[set_id()]
 */
static void port_set_accept_callback(INT32 args)
{
  push_undefined();	/* Return value and argument fallback if void. */
  args++;
  assign_accept_cb(THIS, Pike_sp-args);
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
 *! If the last call done on this port failed, this function will
 *! return an integer describing what went wrong. Refer to your unix
 *! manual for further information.
 */
static void port_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->my_errno);
}

/*! @decl int listen_fd(int fd, void|function accept_callback)
 *!
 *! This function does the same as @[bind], except that instead of
 *! creating a new socket and bind it to a port, it expects the file
 *! descriptor @[fd] to be an already open port.
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
  struct svalue *cb = NULL;
  int fd;
  do_close(p);

  get_all_args(NULL, args, "%d.%*", &fd, &cb);

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
  if(cb) assign_accept_cb (p, cb);
  p->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}

/*! @decl int bind(int|string port, void|function accept_callback, @
 *!                void|string ip, void|string reuse_port)
 *!
 *! Opens a socket and binds it to port number on the local machine.
 *! If the second argument is present, the socket is set to
 *! nonblocking and the callback funcition is called whenever
 *! something connects to it. The callback will receive the id for
 *! this port as argument and should typically call @[accept] to
 *! establish a connection.
 *!
 *! If the optional argument @[ip] is given, @[bind] will try to bind
 *! to an interface with that host name or IP number. Omitting this
 *! will bind to all available IPv4 addresses; specifying "::" will
 *! bind to all IPv4 and IPv6 addresses.
 *!
 *! If the OS supports TCP_FASTOPEN it is enabled automatically.
 *!
 *! If the OS supports SO_REUSEPORT it is enabled if the fourth argument is true.
 *!
 *! @returns
 *!   1 is returned on success, zero on failure. @[errno] provides
 *!   further details about the error in the latter case.
 *!
 *! @seealso
 *!   @[accept], @[set_id]
 */
static void port_bind(INT32 args)
{
  struct port *p = THIS;
  PIKE_SOCKADDR addr;
  int addr_len,fd,tmp;

  do_close(p);

  if(args < 1)
    SIMPLE_WRONG_NUM_ARGS_ERROR("bind", 1);

  if(TYPEOF(Pike_sp[-args]) != PIKE_T_INT &&
     (TYPEOF(Pike_sp[-args]) != PIKE_T_STRING ||
      Pike_sp[-args].u.string->size_shift))
    SIMPLE_ARG_TYPE_ERROR("bind", 1, "int|string(8bit)");

  addr_len = get_inet_addr(&addr,
                           (args > 2 && TYPEOF(Pike_sp[2-args])==PIKE_T_STRING?
                            Pike_sp[2-args].u.string->str : NULL),
                           (TYPEOF(Pike_sp[-args]) == PIKE_T_STRING?
                            Pike_sp[-args].u.string->str : NULL),
			   (TYPEOF(Pike_sp[-args]) == PIKE_T_INT?
			    Pike_sp[-args].u.integer : -1), 0);
  INVALIDATE_CURRENT_TIME();

  fd=fd_socket(SOCKADDR_FAMILY(addr), SOCK_STREAM, 0);

  if(fd < 0)
  {
    p->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }
#ifdef SO_REUSEPORT
  if( args > 3 && Pike_sp[3-args].u.integer )
  {
    /* FreeBSD 7.x wants this to reuse portnumbers.
     * Linux 2.6.x seems to have reserved a slot for the option, but not
     * enabled it. Survive libc's with the option on kernels without.
     *
     * The emulated Linux runtime on MS Windows 10 fails this with EINVAL.
     */
    int o=1;
    if((fd_setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&o, sizeof(int)) < 0)
#ifdef ENOPROTOOPT
       && (errno != ENOPROTOOPT)
#endif
#ifdef EINVAL
       && (errno != EINVAL)
#endif
#ifdef WSAENOPROTOOPT
       && (errno != WSAENOPROTOOPT)
#endif
       ){
      p->my_errno=errno;
      while (fd_close(fd) && errno == EINTR) {}
      errno = p->my_errno;
      pop_n_elems(args);
      push_int(0);
      return;
    }
  }
#endif
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

#if defined(IPV6_V6ONLY) && defined(IPPROTO_IPV6)
  if (SOCKADDR_FAMILY(addr) == AF_INET6) {
    /* Attempt to enable dual-stack (ie mapped IPv4 adresses).
     * Needed on WIN32.
     * cf http://msdn.microsoft.com/en-us/library/windows/desktop/bb513665(v=vs.85).aspx
     */
    int o = 0;
    fd_setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&o, sizeof(int));
  }
#endif

  my_set_close_on_exec(fd,1);

  THREADS_ALLOW_UID();
  if( !(tmp=fd_bind(fd, (struct sockaddr *)&addr, addr_len) < 0) ) {
#ifdef TCP_FASTOPEN
    tmp = 256;
    fd_setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN, (char *)&tmp, sizeof(tmp));
#endif
    tmp = (fd_listen(fd, 16384) < 0);
  }
  THREADS_DISALLOW_UID();

  if(!Pike_fp->current_object->prog)
  {
    if (fd >= 0)
      while (fd_close(fd) && errno == EINTR) {}
    Pike_error("Object destructed in Stdio.Port->bind()\n");
  }

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
 *! Opens a Unix domain socket at the given path in the file system.
 *! If the second argument is present, the socket is set to
 *! nonblocking and the callback funcition is called whenever
 *! something connects to it. The callback will receive the id for
 *! this port as argument and should typically call @[accept] to
 *! establish a connection.
 *!
 *! @returns
 *!   1 is returned on success, zero on failure. @[errno] provides
 *!   further details about the error in the latter case.
 *!
 *! @note
 *!   This function is only available on systems that support Unix domain
 *!   sockets.
 *!
 *! @note
 *!   @[path] had a quite restrictive length limit (~100 characters)
 *!   prior to Pike 7.8.334.
 *!
 *! @seealso
 *!   @[accept], @[set_id]
 */
static void bind_unix(INT32 args)
{
  struct port *p = THIS;
  struct sockaddr_un *addr;
  struct pike_string *path;
  struct svalue *cb = NULL;
  int addr_len,fd,tmp;

  do_close(p);

  get_all_args(NULL, args, "%n.%*", &path, &cb);

  /* NOTE: Some operating systems (eg Linux 2.6) do not support
   *       paths longer than what fits into a plain struct sockaddr_un.
   */
  addr_len = sizeof(struct sockaddr_un) + path->len + 1 -
    sizeof(addr->sun_path);
  addr = xalloc(addr_len);

  strcpy(addr->sun_path, path->str);
  addr->sun_family = AF_UNIX;
#ifdef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN
  /* Length including NUL. */
  addr->sun_len = path->len + 1;
#endif

  fd=fd_socket(AF_UNIX, SOCK_STREAM, 0);

  if(fd < 0)
  {
    free(addr);
    p->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

#ifndef __NT__
  {
    int o=1;
    do {
      tmp = fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			  (char *)&o, sizeof(int));
    } while ((tmp < 0) && (errno == EINTR));
  }
#endif

  my_set_close_on_exec(fd,1);

  THREADS_ALLOW_UID();
  do {
    tmp = fd_bind(fd, (struct sockaddr *)addr, addr_len);
  } while ((tmp < 0) && (errno == EINTR));
  if (tmp >= 0) {
    do {
      tmp = fd_listen(fd, 16384);
    } while ((tmp < 0) && (errno == EINTR));
  }
  THREADS_DISALLOW_UID();

  free(addr);

  if(!Pike_fp->current_object->prog)
  {
    if (fd >= 0)
      while (fd_close(fd) && errno == EINTR) {}
    Pike_error("Object destructed in Stdio.Port->bind_unix()\n");
  }

  if(tmp < 0)
  {
    p->my_errno=errno;
    while (fd_close(fd) && errno == EINTR) {}
    errno = p->my_errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  change_fd_for_box (&p->box, fd);
  if (cb) assign_accept_cb (p, cb);
  p->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}

#endif /* HAVE_SYS_UN_H */

/*! @decl void close()
 *!
 *! Closes the socket.
 */
static void port_close (INT32 UNUSED(args))
{
  do_close (THIS);
}

/*! @decl void create(int|string port, void|function accept_callback, @
 *!                   void|string ip)
 *! @decl void create("stdin", void|function accept_callback)
 *!
 *! When called with an int or any string except @expr{"stdin"@} as
 *! first argument, this function does the same as @[bind()] would do
 *! with the same arguments.
 *!
 *! When called with @expr{"stdin"@} as argument, a socket is created
 *! out of the file descriptor 0. This is only useful if that actually
 *! IS a socket to begin with, and is equivalent to creating a port and
 *! initializing it with @[listen_fd](0).
 *!
 *! @seealso
 *!   @[bind], @[listen_fd]
 */
static void port_create(INT32 args)
{
  if(args)
  {
    if(TYPEOF(Pike_sp[-args]) == PIKE_T_INT ||
       (TYPEOF(Pike_sp[-args]) == PIKE_T_STRING &&
	(Pike_sp[-args].u.string->len != 5 ||
	 strcmp("stdin",Pike_sp[-args].u.string->str))))
    {
      port_bind(args);
      return;
    }else{
      struct port *p = THIS;

      if(TYPEOF(Pike_sp[-args]) != PIKE_T_STRING)
	SIMPLE_WRONG_NUM_ARGS_ERROR("create", 1);

      /* FIXME: Check that the argument is "stdin". */

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
}

extern struct program *file_program;

static int port_fd_factory_fun_num = -1;
static void port_fd_factory(INT32 args)
{
  pop_n_elems(args);
  push_object(clone_object(file_program, 0));
}

/*! @decl Stdio.File accept()
 *!
 *! Get the first connection request waiting for this port and return
 *! it as a connected socket.
 *!
 *! If no connection request is waiting and the port is in nonblocking
 *! mode (i.e. an accept callback is installed) then zero is returned.
 *! Otherwise this function waits until a connection has arrived.
 *!
 *! In Pike 7.8 and later the returned object is created via @[fd_factory()].
 *!
 *! @note
 *!   In Pike 7.7 and later the resulting file object will be assigned
 *!   to the same backend as the port object.
 */

static void port_accept(INT32 args)
{
  PIKE_SOCKADDR addr;
  struct port *this=THIS;
  int fd, err;
  ACCEPT_SIZE_T len=0;
  int one = 1;

  if(this->box.fd < 0)
    Pike_error("port->accept(): Port not open.\n");

  /* FIXME: Race. */
  THIS->box.revents = 0;

  THREADS_ALLOW();
  len=sizeof(addr);
  do {
    fd=fd_accept(this->box.fd, (struct sockaddr *)&addr, &len);
    err = errno;
  } while (fd < 0 && err == EINTR);
  THREADS_DISALLOW();
  INVALIDATE_CURRENT_TIME();

  if(fd < 0)
  {
    this->my_errno=errno = err;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  /* We don't really care if setsockopt fails, since it's just a hint. */
  while ((fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			(char *)&one, sizeof(int)) < 0) &&
	 (errno == EINTR))
    one = 1;

  my_set_close_on_exec(fd,1);
  push_new_fd_object(port_fd_factory_fun_num,
		     fd, FILE_READ | FILE_WRITE, SOCKET_CAPABILITIES);

  if (this->box.backend) {
    struct object *o = Pike_sp[-1].u.object;
    struct my_file *f = (struct my_file *)
      (o->storage + o->prog->inherits[SUBTYPEOF(Pike_sp[-1])].storage_offset);
    change_backend_for_box(&f->box, this->box.backend);
  }

  stack_pop_n_elems_keep_top(args);
}

/*! @decl string query_address()
 *!
 *! Get the address and port of the local socket end-point.
 *!
 *! @returns
 *!   This function returns the address and port of a socket end-point
 *!   on the form @expr{"x.x.x.x port"@} (IPv4) or
 *!   @expr{"x:x:x:x:x:x:x:x port"@} (IPv6).
 *!
 *!   If there is some error querying or formatting the address,
 *!   @expr{0@} (zero) is returned and @[errno()] will return the
 *!   error code.
 *!
 *! @throws
 *!   An error is thrown if the socket isn't bound.
 */
static void socket_query_address(INT32 args)
{
  PIKE_SOCKADDR addr;
  int i;
  char buffer[496];
  ACCEPT_SIZE_T len;

  if(THIS->box.fd <0)
    Pike_error("Stdio.Port->query_address(): Socket not bound yet.\n");

  len=sizeof(addr);
  i=fd_getsockname(THIS->box.fd,(struct sockaddr *)&addr,&len);
  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr.ipv4))
  {
    THIS->my_errno=errno;
    push_int(0);
    return;
  }

#ifdef fd_inet_ntop
  if(!fd_inet_ntop(SOCKADDR_FAMILY(addr), SOCKADDR_IN_ADDR(addr),
		   buffer, sizeof(buffer)-20))
  {
    THIS->my_errno = errno;
    push_int(0);
    return;
  }
#else
  if(SOCKADDR_FAMILY(addr) == AF_INET)
  {
    char *q = inet_ntoa(*SOCKADDR_IN_ADDR(addr));
    strncpy(buffer,q,sizeof(buffer)-20);
    buffer[sizeof(buffer)-20]=0;
  }else{
#ifdef EAFNOSUPPORT
    THIS->my_errno = EAFNOSUPPORT;
#else
    THIS->my_errno = EINVAL;
#endif
    push_int(0);
    return;
  }
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

  if (args!=1)
    SIMPLE_WRONG_NUM_ARGS_ERROR ("set_backend", 1);
  if (TYPEOF(Pike_sp[-args]) != PIKE_T_OBJECT)
    SIMPLE_ARG_TYPE_ERROR ("set_backend", 1, "object(Pike.Backend)");
  backend = get_storage (Pike_sp[-args].u.object, Backend_program);
  if (!backend)
    SIMPLE_ARG_TYPE_ERROR ("set_backend", 1, "object(Pike.Backend)");

  if (p->box.backend)
    change_backend_for_box (&p->box, backend);
  else
    INIT_FD_CALLBACK_BOX (&p->box, backend, p->box.ref_obj,
			  p->box.fd, 0, got_port_event, 0);
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

int fd_from_portobject( struct object *p )
{
  struct port *po = get_storage( p, port_program );
  if(!po) return -1;
  return po->box.fd;
}

/*! @decl int query_fd()
 *!
 *! Returns the file descriptor number associated with this object.
 */
static void port_query_fd(INT32 UNUSED(args))
{
  push_int(fd_from_portobject(Pike_fp->current_object));
}


static void init_port_struct(struct object *o)
{
  INIT_FD_CALLBACK_BOX(&THIS->box, NULL, o, -1, 0, got_port_event, 0);
  THIS->my_errno=0;
  /* NB: NOT reference-counted! */
  SET_SVAL(THIS->id, T_OBJECT, 0, object, Pike_fp->current_object);
}

static void exit_port_struct(struct object *UNUSED(o))
{
  do_close(THIS);
  unhook_fd_callback_box (&THIS->box);
  /* map_variable takes care of id and accept_callback. */
}
/*! @endclass
 */

/*! @endmodule
 */

PMOD_EXPORT struct program *port_program = NULL;

void exit_stdio_port(void)
{
  free_program( port_program );
}

void init_stdio_port(void)
{
  ptrdiff_t offset;
  START_NEW_PROGRAM_ID (STDIO_PORT);
  offset = ADD_STORAGE(struct port);
  PIKE_MAP_VARIABLE("_accept_callback",
                    offset + OFFSETOF(port, accept_callback),
                    tMix, PIKE_T_MIXED, 0);
  PIKE_MAP_VARIABLE("_id",
		    offset + OFFSETOF(port, id), tMix, PIKE_T_NO_REF_MIXED, 0);
  /* function(int|string,void|mixed,void|string:int) */
  ADD_FUNCTION("bind", port_bind,
	       tFunc(tOr(tInt,tStr) tOr(tVoid,tMix) tOr(tVoid,tStr) tOr(tVoid,tInt),tInt), 0);
#ifdef HAVE_SYS_UN_H
  /* function(int|string,void|mixed,void|string:int) */
  ADD_FUNCTION("bind_unix", bind_unix,
               tFunc(tStr tOr(tVoid,tMix),tInt), ID_OPTIONAL);
#endif /* HAVE_SYS_UN_H */
  ADD_FUNCTION("close",port_close,tFunc(tNone,tVoid),0);
  /* function(int,void|mixed:int) */
  ADD_FUNCTION("listen_fd",port_listen_fd,tFunc(tInt tOr(tVoid,tMix),tInt),0);
  /* function(mixed:mixed) */
  ADD_FUNCTION("set_id",port_set_id,tFunc(tMix,tMix),0);
  /* function(mixed:void) */
  ADD_FUNCTION("set_accept_callback", port_set_accept_callback,
               tFunc(tMix, tVoid), 0);
  /* function(:mixed) */
  ADD_FUNCTION("query_id",port_query_id,tFunc(tNone,tMix),0);
  /* function(:string) */
  ADD_FUNCTION("query_address",socket_query_address,tFunc(tNone,tStr),0);
  /* function(:int) */
  ADD_FUNCTION("errno",port_errno,tFunc(tNone,tInt),0);
  /* function(:object) */
  port_fd_factory_fun_num =
    ADD_FUNCTION("fd_factory", port_fd_factory, tFunc(tNone,tObjIs_STDIO_FD),
                 ID_PROTECTED);
  ADD_FUNCTION("accept",port_accept,tFunc(tNone,tObjIs_STDIO_FD),0);
  /* function(void|string|int,void|mixed,void|string:void) */
  ADD_FUNCTION("create", port_create,
	       tFunc(tOr3(tVoid,tStr,tInt) tOr(tVoid,tMix) tOr(tVoid,tStr),
		     tVoid), 0);
  ADD_FUNCTION ("set_backend", port_set_backend, tFunc(tObj,tVoid), 0);
  ADD_FUNCTION ("query_backend", port_query_backend, tFunc(tVoid,tObj), 0);
  ADD_FUNCTION ("query_fd", port_query_fd, tFunc(tVoid,tInt), 0);

#ifdef SO_REUSEPORT
  ADD_INT_CONSTANT( "SO_REUSEPORT_SUPPORT", SO_REUSEPORT, ID_OPTIONAL );
#endif
#ifdef TCP_FASTOPEN
  ADD_INT_CONSTANT( "TCP_FASTOPEN_SUPPORT", TCP_FASTOPEN, ID_OPTIONAL );
#endif
  set_init_callback(init_port_struct);
  set_exit_callback(exit_port_struct);

  port_program = end_program();
  add_program_constant( "_port", port_program, 0 );

}
