#define READ_BUFFER 10000

/*
   string file->read(int len);
   string file->write(string str);
   int file->open(string filename,string read_write_append);
   int file->close(void| string read_write)
   int file->seek(int pos);
   int file->tell();
   void file->set_callback(function(int fd) callback,string read_write_close);
 */

#include "global.h"
#include "types.h"
#include "interpret.h"
#include "svalue.h"
#include "stralloc.h"
#include "array.h"
#include "object.h"
#include "macros.h"
#include "backend.h"
#include "fd_control.h"

#include "file_machine.h"
#include "file.h"
#include "error.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_SYS_SOCKETVAR_H
#include <sys/socketvar.h>
#endif

#define THIS ((struct file *)(fp->current_storage))

static int fd_references[MAX_OPEN_FILEDESCRIPTORS];
static char open_mode[MAX_OPEN_FILEDESCRIPTORS];
static struct program *file_program;

static void reference_fd(int fd)
{
  fd_references[fd]++;
}

static int close_fd(int fd)
{
  if(--fd_references[fd])
  {
    return 0;
  }else{
    return close(fd);
  }
}

static int parse(char *a)
{
  int ret;
  ret=0;
  while(1)
  {
    switch(*(a++))
    {
    case 0: return ret;

    case 'r':
    case 'R':
      ret|=FILE_READ;
      break;

    case 'w':
    case 'W':
      ret|=FILE_WRITE;
      break; 

    case 'a':
    case 'A':
      ret|=FILE_APPEND;
      break;

    case 'c':
    case 'C':
      ret|=FILE_CREATE;
      break;

    case 't':
    case 'T':
      ret|=FILE_TRUNC;
      break;

    case 'x':
    case 'X':
      ret|=FILE_EXCLUSIVE;
      break;

   }
  }
}

static int map(int flags)
{
  int ret;
  ret=0;
  switch(flags & (FILE_READ|FILE_WRITE))
  {
  case FILE_READ: ret=O_RDONLY; break;
  case FILE_WRITE: ret=O_WRONLY; break;
  case FILE_READ | FILE_WRITE: ret=O_RDWR; break;
  }
  if(flags & FILE_APPEND) ret|=O_APPEND;
  if(flags & FILE_CREATE) ret|=O_CREAT;
  if(flags & FILE_TRUNC) ret|=O_TRUNC;
  if(flags & FILE_EXCLUSIVE) ret|=O_EXCL;
  return ret;
}

static void file_read(INT32 args)
{
  INT32 i, r, bytes_read;

  if(THIS->fd < 0)
    error("File not open.\n");

  if(args<1 || sp[-args].type != T_INT)
    error("Bad argument 1 to file->read().\n");

  r=sp[-args].u.integer;

  pop_n_elems(args);
  bytes_read=0;
  THIS->errno=0;

  if(r < 65536)
  {
    struct lpc_string *str;

    str=begin_shared_string(r);

    do{
      i=read(THIS->fd, str->str+bytes_read, r);

      if(i>0)
      {
	r-=i;
	bytes_read+=i;
      }
      else if(i==0)
      {
	break;
      }
      else if(errno != EINTR)
      {
	THIS->errno=errno;
	if(!bytes_read)
	{
	  free((char *)str);
	  push_int(0);
	  return;
	}
	break;
      }
    }while(r);

    if(bytes_read == str->len)
    {
      push_string(end_shared_string(str));
    }else{
      push_string(make_shared_binary_string(str->str,bytes_read));
      free((char *)str);
    }
    
  }else{
#define CHUNK 16384
    INT32 try_read;

    init_buf();
    do{
      try_read=MINIMUM(CHUNK,r);

      i=read(THIS->fd, make_buf_space(try_read), try_read);

      if(i==try_read)
      {
	r-=i;
	bytes_read+=i;
      }
      else if(i>0)
      {
	bytes_read+=i;
	r-=i;
	make_buf_space(i - try_read);
      }
      else if(i==0)
      {
	make_buf_space(-try_read);
	break;
      }
      else
      {
	make_buf_space(-try_read);
	if(errno != EINTR)
	{
	  THIS->errno=errno;
	  if(!bytes_read)
	  {
	    free(simple_free_buf());
	    push_int(0);
	    return;
	  }
	  break;
	}
      }
    }while(r);
    push_string(free_buf());
  }
}

static void file_write_callback(int fd, void *data)
{
  struct file *f;
  f=(struct file *)data;

  set_write_callback(fd, 0, 0);

  assign_svalue_no_free(sp++, &f->id);
  apply_svalue(& f->write_callback, 1);
  pop_stack();
}

static void file_write(INT32 args)
{
  INT32 i;
  if(THIS->fd < 0)
    error("File not open for write.\n");

  if(args<1 || sp[-args].type != T_STRING)
    error("Bad argument 1 to file->write().\n");

  i=write(THIS->fd, sp[-args].u.string->str, sp[-args].u.string->len);

  if(i<0)
  {
    switch(errno)
    {
    default:
      THIS->errno=errno;
      pop_n_elems(args);
      push_int(-1);
      return;

    case EINTR:
    case EWOULDBLOCK:
      i=0;
    }
  }

  if(!IS_ZERO(& THIS->write_callback))
    set_write_callback(THIS->fd, file_write_callback, (void *)THIS);
  THIS->errno=0;

  pop_n_elems(args);
  push_int(i);
}

static void do_close(struct file *f, int flags)
{
  f->errno=0;

  if(f->fd == -1) return; /* already closed */

  flags &= open_mode[f->fd];
  
  switch(flags & (FILE_READ | FILE_WRITE))
  {
  case 0:
    return;

  case FILE_READ:
    open_mode[f->fd] &=~ FILE_READ;

    set_read_callback(f->fd, 0, 0);
    if(open_mode[f->fd] & FILE_WRITE)
    {
      shutdown(f->fd, 0);
    }else{
      close_fd(f->fd);
      f->fd=-1;
    }
    break;

  case FILE_WRITE:
    open_mode[f->fd] &=~ FILE_WRITE;

    set_write_callback(f->fd, 0, 0);
    
    if(open_mode[f->fd] & FILE_READ)
    {
      shutdown(f->fd, 1);
    }else{
      close_fd(f->fd);
      f->fd=-1;
    }
    break;

  case FILE_READ | FILE_WRITE:
    open_mode[f->fd] &=~ (FILE_WRITE | FILE_WRITE);

    set_read_callback(f->fd, 0, 0);
    set_write_callback(f->fd, 0, 0);
    
    close_fd(f->fd);
    f->fd=-1;
    break;
  }
}

static void file_close(INT32 args)
{
  int flags;
  if(args)
  {
    if(sp[-args].type != T_STRING)
      error("Bad argument 1 to file->close()\n");
    flags=parse(sp[-args].u.string->str);
  }else{
    flags=FILE_READ | FILE_WRITE;
  }

  do_close(THIS,flags);
  pop_n_elems(args);
  push_int(1);
}

static void file_open(INT32 args)
{
  int flags,fd;
  do_close(THIS,FILE_READ | FILE_WRITE);

  if(args < 2)
    error("Too few arguments to file->open()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file->open()\n");

  if(sp[1-args].type != T_STRING)
    error("Bad argument 2 to file->open()\n");
  
  flags=parse(sp[1-args].u.string->str);

  if(!( flags &  (FILE_READ | FILE_WRITE)))
    error("Must open file for at least one of read and write.\n");

  THIS->errno = 0;

  fd=open(sp[-args].u.string->str,map(flags), 00666);

  if(fd >= MAX_OPEN_FILEDESCRIPTORS)
  {
    THIS->errno=EBADF;
    close(fd);
    fd=-1;
  }
  else if(fd < 0)
  {
    THIS->errno=errno;
  }
  else
  {
    set_close_on_exec(fd,1);
    reference_fd(fd);
    open_mode[fd]=flags;
    THIS->fd=fd;
  }

  pop_n_elems(args);
  push_int(fd>=0);
}


static void file_seek(INT32 args)
{
  INT32 to;

  if(args<1 || sp[-args].type != T_INT)
    error("Bad argument 1 to file->seek().\n");

  if(THIS->fd < 0)
    error("File not open.\n");
  
  to=sp[-args].u.integer;

  THIS->errno=0;

  to=lseek(THIS->fd,to,to<0 ? SEEK_END : SEEK_SET);

  if(to<0) THIS->errno=errno;

  pop_n_elems(args);
  push_int(to);
}

static void file_tell(INT32 args)
{
  INT32 to;

  if(THIS->fd < 0)
    error("File not open.\n");
  
  THIS->errno=0;
  to=lseek(THIS->fd, 0L, SEEK_CUR);

  if(to<0) THIS->errno=errno;

  pop_n_elems(args);
  push_int(to);
}

struct array *encode_stat(struct stat *);

static void file_stat(INT32 args)
{
  struct stat s;

  if(THIS->fd < 0)
    error("File not open.\n");
  
  pop_n_elems(args);

  if(fstat(THIS->fd, &s) < 0)
  {
    THIS->errno=errno;
    push_int(0);
  }else{
    THIS->errno=0;
    push_array(encode_stat(&s));
  }
}

static void file_errno(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->errno);
}

/* Trick compiler to keep 'buffer' in memory for
 * as short a time as possible.
 */
static struct lpc_string *do_read(INT32 *amount,int fd)
{
  char buffer[READ_BUFFER];
  
  *amount = read(fd, buffer, READ_BUFFER);

  if(*amount>0) return make_shared_binary_string(buffer,*amount);
  return 0;
}

static void file_read_callback(int fd, void *data)
{
  struct lpc_string *s;
  INT32 i;
  struct file *f;
  f=(struct file *)data;
  f->errno=0;

#ifdef DEBUG
  if(f->fd == -1)
    fatal("Error in file::read_callback()\n");

  if(fd != f->fd)
    fatal("Error in file::read_callback() (2)\n");
#endif

  s=do_read(&i,f->fd);

  if(i>0)
  {
    assign_svalue_no_free(sp++, &f->id);
    push_string(s);
    apply_svalue(& f->read_callback, 2);
    pop_stack();
    return;
  }
  
  if(i < 0)
  {
    f->errno=errno;
    switch(errno)
    {
    case EINTR:
    case EWOULDBLOCK:
      return;
    }
  }

  set_read_callback(fd, 0, 0);

  if(f->close_callback.type == T_INT)
  {
    do_close(f, FILE_READ | FILE_WRITE);
  }else{
    assign_svalue_no_free(sp++, &f->id);
    apply_svalue(& f->close_callback, 1);
  }
}

static void file_set_nonblocking(INT32 args)
{
  if(args < 3)
    error("Too few arguments to file->set_nonblocking()\n");

  assign_svalue(& THIS->read_callback, sp-args);
  assign_svalue(& THIS->write_callback, sp+1-args);
  assign_svalue(& THIS->close_callback, sp+2-args);

  if(THIS->fd >= 0)
  {
    if(IS_ZERO(& THIS->read_callback))
      set_read_callback(THIS->fd, 0,0);
    else
      set_read_callback(THIS->fd, file_read_callback, (void *)THIS);

    if(IS_ZERO(& THIS->write_callback))
      set_write_callback(THIS->fd, 0,0);
    else
      set_write_callback(THIS->fd, file_write_callback, (void *)THIS);
    set_nonblocking(THIS->fd,1);
  }

  pop_n_elems(args);
}

static void file_set_blocking(INT32 args)
{
  free_svalue(& THIS->read_callback);
  THIS->read_callback.type=T_INT;
  free_svalue(& THIS->write_callback);
  THIS->write_callback.type=T_INT;
  free_svalue(& THIS->close_callback);
  THIS->close_callback.type=T_INT;

  if(THIS->fd >= 0)
  {
    set_read_callback(THIS->fd, 0, 0);
    set_write_callback(THIS->fd, 0, 0);
    set_nonblocking(THIS->fd,0);
  }
  pop_n_elems(args);
}

static void file_set_id(INT32 args)
{
  if(args < 1)
    error("Too few arguments to file->set_id()\n");

  assign_svalue(& THIS->id, sp-args);
  pop_n_elems(args-1);
}

static void file_query_id(INT32 args)
{
  pop_n_elems(args);
  assign_svalue_no_free(sp++,& THIS->id);
}

static void file_query_fd(INT32 args)
{
  pop_n_elems(args);
  push_int(THIS->fd);
}

static void file_query_read_callback(INT32 args)
{
  pop_n_elems(args);
  assign_svalue_no_free(sp++,& THIS->read_callback);
}

static void file_query_write_callback(INT32 args)
{
  pop_n_elems(args);
  assign_svalue_no_free(sp++,& THIS->write_callback);
}

static void file_query_close_callback(INT32 args)
{
  pop_n_elems(args);
  assign_svalue_no_free(sp++,& THIS->close_callback);
}

struct object *file_make_object_from_fd(int fd, int mode)
{
  struct object *o;
  struct file *f;

  reference_fd(fd);
  open_mode[fd]=mode;

  o=clone(file_program,0);
  f=(struct file *)o->storage;
  f->fd=fd;
  return o;
}

static void file_set_buffer(INT32 args)
{
  INT32 bufsize;
  int flags;

  if(THIS->fd==-1)
    error("file->set_buffer() on closed file.\n");
  if(!args)
    error("Too few arguments to file->set_buffer()\n");
  if(sp[-args].type!=T_INT)
    error("Bad argument 1 to file->set_buffer()\n");

  bufsize=sp[-args].u.integer;
  if(bufsize < 0)
    error("Bufsize must be larger than zero.\n");

  if(args>1)
  {
    if(sp[1-args].type != T_STRING)
      error("Bad argument 2 to file->set_buffer()\n");
    flags=parse(sp[1-args].u.string->str);
  }else{
    flags=FILE_READ | FILE_WRITE;
  }

#ifdef SOCKET_BUFFER_MAX
#if SOCKET_BUFFER_MAX
  if(bufsize>SOCKET_BUFFER_MAX) bufsize=SOCKET_BUFFER_MAX;
#endif
  flags &= open_mode[THIS->fd];
  if(flags & FILE_READ)
  {
    int tmp=bufsize;
    setsockopt(THIS->fd,SOL_SOCKET, SO_RCVBUF, (char *)&tmp, sizeof(tmp));
  }

  if(flags & FILE_WRITE)
  {
    int tmp=bufsize;
    setsockopt(THIS->fd,SOL_SOCKET, SO_SNDBUF, (char *)&tmp, sizeof(tmp));
  }
#endif
}

static void file_pipe(INT32 args)
{
  int inout[2],i;
  do_close(THIS,FILE_READ | FILE_WRITE);
  pop_n_elems(args);
  THIS->errno=0;

  i=socketpair(AF_UNIX, SOCK_STREAM, 0, &inout[0]);
  if(i<0)
  {
    THIS->errno=errno;
    push_int(0);
  }
  else if(i >= MAX_OPEN_FILEDESCRIPTORS)
  {
    THIS->errno=EBADF;
    close(i);
    push_int(0);
  }
  else
  {
    reference_fd(inout[0]);
    open_mode[inout[0]]=FILE_READ | FILE_WRITE;

    set_close_on_exec(inout[0],1);
    set_close_on_exec(inout[1],1);
    THIS->fd=inout[0];

    push_object(file_make_object_from_fd(inout[1],FILE_READ | FILE_WRITE));
  }
}

static void init_file_struct(char *foo, struct object *o)
{
  struct file *f;
  f=(struct file *) foo;
  f->fd=-1;
  f->id.type=T_OBJECT;
  f->id.u.object=o;
  o->refs++;
  f->read_callback.type=T_INT;
  f->write_callback.type=T_INT;
  f->close_callback.type=T_INT;
  f->errno=0;
}

static void exit_file_struct(char *foo, struct object *o)
{
  struct file *f;
  f=(struct file *) foo;
  if(f->fd != -1 &&
     query_read_callback(f->fd) == file_read_callback &&
     query_read_callback_data(f->fd) == (void *) f)
  {
    set_read_callback(f->fd,0,0);
  }

  if(f->fd != -1 &&
     query_write_callback(f->fd) == file_write_callback &&
     query_write_callback_data(f->fd) == (void *) f)
  {
    set_write_callback(f->fd,0,0);
  }
  do_close(f,FILE_READ | FILE_WRITE);
  free_svalue(& f->id);
  free_svalue(& f->read_callback);
  free_svalue(& f->write_callback);
  free_svalue(& f->close_callback);
  f->id.type=T_INT;
  f->read_callback.type=T_INT;
  f->write_callback.type=T_INT;
  f->close_callback.type=T_INT;

}

static void file_dup(INT32 args)
{
  struct object *o;
  struct file *f;

  if(THIS->fd < 0)
    error("File not open.\n");

  pop_n_elems(args);
  reference_fd(THIS->fd);

  o=clone(file_program,0);
  f=(struct file *)o->storage;
  f->fd=THIS->fd;
  assign_svalue(& f->read_callback, & THIS->read_callback);
  assign_svalue(& f->write_callback, & THIS->write_callback);
  assign_svalue(& f->close_callback, & THIS->close_callback);
  assign_svalue(& f->id, & THIS->id);
  pop_n_elems(args);
  push_object(o);
}

static void file_assign(INT32 args)
{
  struct object *o;
  struct file *f;

  if(args < 1)
    error("Too few arguments to file->assign()\n");

  if(sp[-args].type != T_OBJECT)
    error("Bad argument 1 to file->assign()\n");

  o=sp[-args].u.object;

  /* Actually, we allow any object which first inherit is 
   * /precompiled/file
   */
  if(!o->prog || o->prog->inherits[0].prog != file_program)
    error("Argument 1 to file->assign() must be a clone of /precompiled/file\n");
  do_close(THIS, FILE_READ | FILE_WRITE);

  f=(struct file *)o->storage;

  if(f->fd < 0)
    error("File given to assign is not open.\n");

  THIS->fd=f->fd;
  reference_fd(f->fd);
  assign_svalue(& THIS->read_callback, & f->read_callback);
  assign_svalue(& THIS->write_callback, & f->write_callback);
  assign_svalue(& THIS->close_callback, & f->close_callback);
  assign_svalue(& THIS->id, & f->id);

  push_int(1);
}

static void file_dup2(INT32 args)
{
  int fd;
  struct object *o;
  struct file *f;

  if(args < 1)
    error("Too few arguments to file->dup2()\n");

  if(THIS->fd < 0)
    error("File not open.\n");

  if(sp[-args].type != T_OBJECT)
    error("Bad argument 1 to file->dup2()\n");

  o=sp[-args].u.object;

  /* Actually, we allow any object which first inherit is 
   * /precompiled/file
   */
  if(!o->prog || o->prog->inherits[0].prog != file_program)
    error("Argument 1 to file->assign() must be a clone of /precompiled/file\n");

  f=(struct file *)o->storage;

  if(f->fd < 0)
    error("File given to dup2 not open.\n");

  fd=f->fd;
  do_close(f, FILE_READ | FILE_WRITE);

  
  if(dup2(THIS->fd,fd) < 0)
  {
    THIS->errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  f->fd=fd;
  open_mode[fd]=open_mode[THIS->fd];
  reference_fd(fd);
  assign_svalue(& THIS->read_callback, & f->read_callback);
  assign_svalue(& THIS->write_callback, & f->write_callback);
  assign_svalue(& THIS->close_callback, & f->close_callback);
  assign_svalue(& THIS->id, & f->id);

  push_int(1);
}

static void file_open_socket(INT32 args)
{
  int fd, tmp;

  do_close(THIS, FILE_READ | FILE_WRITE);
  fd=socket(AF_INET, SOCK_STREAM, 0);
  if(fd >= MAX_OPEN_FILEDESCRIPTORS)
  {
    THIS->errno = EBADF;
    pop_n_elems(args);
    push_int(0);
    return;
  }
  if(fd < 0)
  {
    THIS->errno=errno;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  tmp=1;
  setsockopt(fd,SOL_SOCKET, SO_KEEPALIVE, (char *)&tmp, sizeof(tmp));
  reference_fd(fd);
  set_close_on_exec(fd,1);
  open_mode[fd]=FILE_READ | FILE_WRITE;
  THIS->fd = fd;
  THIS->errno=0;

  pop_n_elems(args);
  push_int(1);
}

static void file_connect(INT32 args)
{
  struct sockaddr_in addr;
  if(args < 2)
    error("Too few arguments to file->connect()\n");

  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file->connect()\n");
      
  if(sp[1-args].type != T_INT)
    error("Bad argument 2 to file->connect()\n");

  if(THIS->fd < 0)
    error("file->connect(): File not open for connect()\n");

  addr.sin_family = AF_INET;
  addr.sin_port = htons(((u_short)sp[1-args].u.integer));

  if (inet_addr(sp[-args].u.string->str) == -1)
     error("Malformed ip number.\n");
  addr.sin_addr.s_addr = inet_addr(sp[-args].u.string->str);

  if((long)addr.sin_addr.s_addr == -1)
  {
    THIS->errno=EINVAL;
    pop_n_elems(args);
    push_int(0);
    return;
  }

  if(connect(THIS->fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    /* something went wrong */
    THIS->errno=errno;
    pop_n_elems(args);
    push_int(0);
  }else{

    THIS->errno=0;
    pop_n_elems(args);
    push_int(1);
  }
}

static void file_query_address(INT32 args)
{
  struct sockaddr_in addr;
  int i,len;
  char buffer[496],*q;

  len=sizeof(addr);
  if(args > 0 && !IS_ZERO(sp-args))
  {
    if(THIS->fd <0)
      error("file->query_address(): Connection not open.\n");
    
    i=getsockname(THIS->fd,(struct sockaddr *)&addr,&len);
  }else{
    if(THIS->fd <0)
      error("file->query_address(): Connection not open.\n");

    i=getpeername(THIS->fd,(struct sockaddr *)&addr,&len);
  }
  pop_n_elems(args);
  if(i < 0 || len < sizeof(addr))
  {
    THIS->errno=errno;
    push_int(0);
  }

  q=inet_ntoa(addr.sin_addr);
  strncpy(buffer,q,sizeof(buffer)-20);
  buffer[sizeof(buffer)-20]=0;
  sprintf(buffer+strlen(buffer)," %d",(int)(ntohs(addr.sin_port)));

  push_string(make_shared_string(buffer));
}

static void file_create(INT32 args)
{
  char *s;
  if(!args || sp[-args].type == T_INT) return;
  if(sp[-args].type != T_STRING)
    error("Bad argument 1 to file->create()\n");

  do_close(THIS, FILE_READ | FILE_WRITE);
  s=sp[-args].u.string->str;
  if(!strcmp(s,"stdin"))
  {
    THIS->fd=0;
    reference_fd(0);
    open_mode[0]=FILE_READ;
  }
  else if(!strcmp(s,"stdout"))
  {
    THIS->fd=1;
    reference_fd(1);
    open_mode[1]=FILE_WRITE;
  }
  else if(!strcmp(s,"stderr"))
  {
    THIS->fd=2;
    reference_fd(2);
    open_mode[2]=FILE_WRITE;
  }
  else
  {
    error("file->create() not called with stdin, stdout or stderr as argument.\n");
  }
}

void exit_files()
{
  free_program(file_program);
}

static RETSIGTYPE sig_child(int arg)
{
  waitpid(-1,0,WNOHANG);
  signal(SIGCHLD,sig_child);
}

void init_files_programs()
{
  int i;
  for(i=0; i<MAX_OPEN_FILEDESCRIPTORS; i++)
  {
    open_mode[i] = 0;
    fd_references[i] = 0;
  }

  reference_fd(0);
  reference_fd(1);
  reference_fd(2);

  signal(SIGCHLD,sig_child);
  signal(SIGPIPE,SIG_IGN);

  start_new_program();
  add_storage(sizeof(struct file));


  add_function("open",file_open,"function(string,string:int)",0);
  add_function("close",file_close,"function(string|void:void)",0);
  add_function("read",file_read,"function(int:string|int)",0);
  add_function("write",file_write,"function(string:int)",0);

  add_function("seek",file_seek,"function(int:int)",0);
  add_function("tell",file_tell,"function(:int)",0);
  add_function("stat",file_stat,"function(:int *)",0);
  add_function("errno",file_errno,"function(:int)",0);

  add_function("set_nonblocking",file_set_nonblocking,"function(mixed,mixed,mixed:void)",0);
  add_function("set_blocking",file_set_blocking,"function(:void)",0);
  add_function("set_id",file_set_id,"function(mixed:void)",0);

  add_function("query_fd",file_query_fd,"function(:int)",0);
  add_function("query_id",file_query_id,"function(:mixed)",0);
  add_function("query_read_callback",file_query_read_callback,"function(:mixed)",0);
  add_function("query_write_callback",file_query_write_callback,"function(:mixed)",0);
  add_function("query_close_callback",file_query_close_callback,"function(:mixed)",0);

  add_function("dup",file_dup,"function(:object)",0);
  add_function("dup2",file_dup2,"function(object:object)",0);
  add_function("assign",file_assign,"function(object:int)",0);
  add_function("pipe",file_pipe,"function(:object)",0);

  add_function("set_buffer",file_set_buffer,"function(int,string|void:void)",0);
  add_function("open_socket",file_open_socket,"function(:int)",0);
  add_function("connect",file_connect,"function(string,int:int)",0);
  add_function("query_address",file_query_address,"function(int|void:int)",0);
  add_function("create",file_create,"function(void|string:void)",0);

  set_init_callback(init_file_struct);
  set_exit_callback(exit_file_struct);

  file_program=end_c_program("/precompiled/file");

  file_program->refs++;

  port_setup_program();

}

