/*\
||| This file a part of Pike, and is copyright by Fredrik Hubinette
||| Pike is distributed as GPL (General Public License)
||| See the files COPYING and DISCLAIMER for more information.
\*/
/**/
#define NO_PIKE_SHORTHAND

#include "global.h"
#include "fdlib.h"
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

RCSID("$Id: socket.c,v 1.54 2003/09/30 20:40:40 mast Exp $");

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

#ifdef HAVE_WINSOCK_H
#include <winsock.h>
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

struct port
{
  int fd;
  int my_errno;
  struct svalue accept_callback;
  struct svalue id;
};

#undef THIS
#define THIS ((struct port *)(Pike_fp->current_storage))
static void port_accept_callback(int fd,void *data);

static void do_close(struct port *p, struct object *o)
{
 retry:
  if(p->fd >= 0)
  {
    if(fd_close(p->fd) < 0)
      if(errno == EINTR)
	goto retry;

    if(query_read_callback(p->fd)==port_accept_callback)
      o->refs--;
    set_read_callback(p->fd,0,0);
  }

  p->fd=-1;
}

static void port_set_id(INT32 args)
{
  if(args < 1)
    Pike_error("Too few arguments to port->set_id()\n");

  assign_svalue(& THIS->id, Pike_sp-args);
  pop_n_elems(args-1);
}

static void port_query_id(INT32 args)
{
  pop_n_elems(args);
  assign_svalue_no_free(Pike_sp++,& THIS->id);
}

static void port_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->my_errno);
}

static void port_accept_callback(int fd,void *data)
{
  struct port *f;
  f=(struct port *)data;
#ifndef __NT__
#ifdef PIKE_DEBUG
  if(!query_nonblocking(f->fd))
    fatal("Port is in blocking mode in port accept callback!!!\n");
#endif
#endif

  assign_svalue_no_free(Pike_sp++, &f->id);
  apply_svalue(& f->accept_callback, 1);
  pop_stack();
  return;
}

static void port_listen_fd(INT32 args)
{
  int fd;
  do_close(THIS,Pike_fp->current_object);

  if(args < 1)
    Pike_error("Too few arguments to port->bind_fd()\n");

  if(Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 to port->bind_fd()\n");

  fd=Pike_sp[-args].u.integer;

  if(fd<0)
  {
    THIS->my_errno=EBADF;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if(fd_listen(fd, 16384) < 0)
  {
    THIS->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if(args > 1)
  {
    assign_svalue(& THIS->accept_callback, Pike_sp+1-args);
    set_nonblocking(fd,1);
    if(!IS_ZERO(& THIS->accept_callback))
    {
      add_ref(Pike_fp->current_object);
      set_read_callback(fd, port_accept_callback, (void *)THIS);
    }
  }

  THIS->fd=fd;
  THIS->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}

static void port_bind(INT32 args)
{
  struct sockaddr_in addr;
  int o;
  int fd,tmp;

  do_close(THIS,Pike_fp->current_object);

  if(args < 1)
    Pike_error("Too few arguments to port->bind()\n");

  if(Pike_sp[-args].type != PIKE_T_INT)
    Pike_error("Bad argument 1 to port->bind()\n");

  fd=fd_socket(AF_INET, SOCK_STREAM, 0);

  if(fd < 0)
  {
    THIS->my_errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

#ifndef __NT__
  o=1;
  if(fd_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&o, sizeof(int)) < 0)
  {
    THIS->my_errno=errno;
    while (close(fd) && errno == EINTR) {}
    errno = THIS->my_errno;
    push_int(0);
    return;
  }
#endif

  my_set_close_on_exec(fd,1);

  MEMSET((char *)&addr,0,sizeof(struct sockaddr_in));

  if(args > 2 && Pike_sp[2-args].type==PIKE_T_STRING)
  {
    get_inet_addr(&addr, Pike_sp[2-args].u.string->str);
  }else{
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  addr.sin_port = htons( ((u_short)Pike_sp[-args].u.integer) );
  addr.sin_family = AF_INET;

  THREADS_ALLOW_UID();
  tmp = fd_bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 ||
    fd_listen(fd, 16384) < 0;
  THREADS_DISALLOW_UID();

  if(tmp)
  {
    THIS->my_errno=errno;
    while (fd_close(fd) && errno == EINTR) {}
    errno = THIS->my_errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }


  if(args > 1)
  {
    assign_svalue(& THIS->accept_callback, Pike_sp+1-args);
    if(!IS_ZERO(& THIS->accept_callback))
    {
      add_ref(Pike_fp->current_object);
      set_read_callback(fd, port_accept_callback, (void *)THIS);
      set_nonblocking(fd,1);
    }
  }

  THIS->fd=fd;
  THIS->my_errno=0;
  pop_n_elems(args);
  push_int(1);
}

static void port_create(INT32 args)
{
  if(args)
  {
    if(Pike_sp[-args].type == PIKE_T_INT)
    {
      port_bind(args);
      return;
    }else{
      if(Pike_sp[-args].type != PIKE_T_STRING)
	Pike_error("Bad argument 1 to port->create()\n");

      if(strcmp("stdin",Pike_sp[-args].u.string->str))
	Pike_error("port->create() called with string other than 'stdin'\n");

      do_close(THIS,Pike_fp->current_object);
      THIS->fd=0;

      if(fd_listen(THIS->fd, 16384) < 0)
      {
	THIS->my_errno=errno;
      }else{
	if(args > 1)
	{
	  assign_svalue(& THIS->accept_callback, Pike_sp+1-args);
	  if(!IS_ZERO(& THIS->accept_callback))
	  {
	    add_ref(Pike_fp->current_object);
	    set_read_callback(THIS->fd, port_accept_callback, (void *)THIS);
	    set_nonblocking(THIS->fd,1);
	  }
	}
      }
    }
  }
  pop_n_elems(args);
}

extern struct program *file_program;

static void port_accept(INT32 args)
{
  struct sockaddr_in addr;
  struct port *this=THIS;
  int fd,tmp, err;
  struct object *o;
  ACCEPT_SIZE_T len=0;

  if(THIS->fd < 0)
    Pike_error("port->accept(): Port not open.\n");

  THREADS_ALLOW();
  len=sizeof(addr);
  do {
    fd=fd_accept(this->fd, (struct sockaddr *)&addr, &len);
    err = errno;
  } while (fd < 0 && err == EINTR);
  THREADS_DISALLOW();

  if(fd < 0)
  {
    THIS->my_errno=errno = err;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  my_set_close_on_exec(fd,1);
  o=file_make_object_from_fd(fd,FILE_READ | FILE_WRITE, SOCKET_CAPABILITIES);

  pop_n_elems(args);
  push_object(o);
}

static void socket_query_address(INT32 args)
{
  struct sockaddr_in addr;
  int i;
  char buffer[496],*q;
  ACCEPT_SIZE_T len;

  if(THIS->fd <0)
    Pike_error("socket->query_address(): Socket not bound yet.\n");

  len=sizeof(addr);
  i=fd_getsockname(THIS->fd,(struct sockaddr *)&addr,&len);
  pop_n_elems(args);
  if(i < 0 || len < (int)sizeof(addr))
  {
    THIS->my_errno=errno;
    push_int(0);
    return;
  }

  q=inet_ntoa(addr.sin_addr);
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.sin_port)));

  push_string(make_shared_string(buffer));
}


static void init_port_struct(struct object *o)
{
  THIS->fd=-1;
  THIS->id.type=PIKE_T_INT;
#ifdef __CHECKER__
  THIS->id.subtype=0;
#endif
  THIS->id.u.integer=0;
  THIS->accept_callback.type=PIKE_T_INT;
  THIS->my_errno=0;
}

static void exit_port_struct(struct object *o)
{
  do_close(THIS,o);
  free_svalue(& THIS->id);
  free_svalue(& THIS->accept_callback);
  THIS->id.type=PIKE_T_INT;
  THIS->accept_callback.type=PIKE_T_INT;
}

struct program *port_program;

void port_exit_program(void)
{
  free_program( port_program );
}

void port_setup_program(void)
{
  ptrdiff_t offset;
  start_new_program();
  offset=ADD_STORAGE(struct port);
  map_variable("_accept_callback","mixed",0,offset+OFFSETOF(port,accept_callback),PIKE_T_MIXED);
  map_variable("_id","mixed",0,offset+OFFSETOF(port,id),PIKE_T_MIXED);
  /* function(int,void|mixed,void|string:int) */
  ADD_FUNCTION("bind",port_bind,tFunc(tInt tOr(tVoid,tMix) tOr(tVoid,tStr),tInt),0);
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
  ADD_FUNCTION("create",port_create,tFunc(tOr3(tVoid,tStr,tInt) tOr(tVoid,tMix) tOr(tVoid,tStr),tVoid),0);

  set_init_callback(init_port_struct);
  set_exit_callback(exit_port_struct);

  port_program = end_program();
  add_program_constant( "_port", port_program, 0 );
/*   end_class("_port",0); */
}

int fd_from_portobject( struct object *p )
{
  struct port *po = (struct port *)get_storage( p, port_program );
  if(!po) return -1;
  return po->fd;
}
