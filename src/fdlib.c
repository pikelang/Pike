#include "global.h"
#include "fdlib.h"
#include "error.h"
#include <math.h>

RCSID("$Id: fdlib.c,v 1.28 1999/08/06 15:23:46 grubba Exp $");

#ifdef HAVE_WINSOCK_H

#ifdef _REENTRANT
#include "threads.h"

static MUTEX_T fd_mutex;
#endif

long da_handle[MAX_OPEN_FILEDESCRIPTORS];
int fd_type[MAX_OPEN_FILEDESCRIPTORS];
int first_free_handle;

#define FDDEBUG(X)

char *debug_fd_info(int fd)
{
  if(fd<0)
    return "BAD";

  if(fd > MAX_OPEN_FILEDESCRIPTORS)
    return "OUT OF RANGE";

  switch(fd_type[fd])
  {
    case FD_SOCKET: return "IS SOCKET";
    case FD_CONSOLE: return "IS CONSOLE";
    case FD_FILE: return "IS FILE";
    case FD_PIPE: return "IS PIPE";
    default: return "NOT OPEN";
  }
}

int debug_fd_query_properties(int fd, int guess)
{
  switch(fd_type[fd])
  {
    case FD_SOCKET:
      return fd_BUFFERED | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN;
    case FD_FILE:
    case FD_CONSOLE:
      return fd_INTERPROCESSABLE;

    case FD_PIPE:
      return fd_INTERPROCESSABLE | fd_BUFFERED;
    default: return 0;
  }
}

void fd_init()
{
  int e;
  WSADATA wsadata;
  
  mt_init(&fd_mutex);
  mt_lock(&fd_mutex);
  if(WSAStartup(MAKEWORD(1,1), &wsadata) != 0)
  {
    fatal("No winsock available.\n");
  }
  FDDEBUG(fprintf(stderr,"Using %s\n",wsadata.szDescription));
  
  fd_type[0]=FD_CONSOLE;
  da_handle[0]=(long)GetStdHandle(STD_INPUT_HANDLE);
  fd_type[1]=FD_CONSOLE;
  da_handle[1]=(long)GetStdHandle(STD_OUTPUT_HANDLE);
  fd_type[2]=FD_CONSOLE;
  da_handle[2]=(long)GetStdHandle(STD_ERROR_HANDLE);

  first_free_handle=3;
  for(e=3;e<MAX_OPEN_FILEDESCRIPTORS-1;e++)
    fd_type[e]=e+1;
  fd_type[e]=FD_NO_MORE_FREE;
  mt_unlock(&fd_mutex);
}

void fd_exit()
{
  WSACleanup();
  mt_destroy(&fd_mutex);
}

FD debug_fd_open(char *file, int open_mode, int create_mode)
{
  HANDLE x;
  FD fd;
  DWORD omode,cmode,amode;
  omode=0;
  FDDEBUG(fprintf(stderr,"fd_open(%s,0x%x,%o)\n",file,open_mode,create_mode));
  if(first_free_handle == FD_NO_MORE_FREE)
  {
    errno=EMFILE;
    return -1;
  }

  if(open_mode & fd_RDONLY) omode|=GENERIC_READ;
  if(open_mode & fd_WRONLY) omode|=GENERIC_WRITE;
  
  switch(open_mode & (fd_CREAT | fd_TRUNC | fd_EXCL))
  {
    case fd_CREAT | fd_TRUNC:
      cmode=CREATE_ALWAYS;
      break;

    case fd_TRUNC:
    case fd_TRUNC | fd_EXCL:
      cmode=TRUNCATE_EXISTING;
      break;

    case fd_CREAT:
      cmode=OPEN_ALWAYS; break;

    case fd_CREAT | fd_EXCL:
    case fd_CREAT | fd_EXCL | fd_TRUNC:
      cmode=CREATE_NEW;
      break;

    case 0:
    case fd_EXCL:
      cmode=OPEN_EXISTING;
      break;
  }

  if(create_mode & 4)
  {
    amode=FILE_ATTRIBUTE_NORMAL;
  }else{
    amode=FILE_ATTRIBUTE_READONLY;
  }
    
  x=CreateFile(file,
	       omode,
	       FILE_SHARE_READ | FILE_SHARE_WRITE,
	       NULL,
	       cmode,
	       amode,
	       NULL);

  
  if(x==INVALID_HANDLE_VALUE)
  {
    errno=GetLastError();
    return -1;
  }

  SetHandleInformation(x,HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);

  mt_lock(&fd_mutex);

  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=FD_FILE;
  da_handle[fd]=(long)x;

  mt_unlock(&fd_mutex);

  if(open_mode & fd_APPEND)
    fd_lseek(fd,0,SEEK_END);

  FDDEBUG(fprintf(stderr,"Opened %s file as %d (%d)\n",file,fd,x));

  return fd;
}

FD debug_fd_socket(int domain, int type, int proto)
{
  FD fd;
  SOCKET s;
  mt_lock(&fd_mutex);
  if(first_free_handle == FD_NO_MORE_FREE)
  {
    mt_unlock(&fd_mutex);
    errno=EMFILE;
    return -1;
  }
  mt_unlock(&fd_mutex);

  s=socket(domain, type, proto);

  if(s==INVALID_SOCKET)
  {
    errno=WSAGetLastError();
    return -1;
  }
  
  SetHandleInformation(s,HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);
  mt_lock(&fd_mutex);
  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=FD_SOCKET;
  da_handle[fd]=(long)s;
  mt_unlock(&fd_mutex);

  FDDEBUG(fprintf(stderr,"New socket: %d (%d)\n",fd,s));

  return fd;
}

int debug_fd_pipe(int fds[2] DMALLOC_LINE_ARGS)
{
  HANDLE files[2];
  mt_lock(&fd_mutex);
  if(first_free_handle == FD_NO_MORE_FREE)
  {
    mt_unlock(&fd_mutex);
    errno=EMFILE;
    return -1;
  }
  mt_unlock(&fd_mutex);
  if(!CreatePipe(&files[0], &files[1], NULL, 0))
  {
    errno=GetLastError();
    return -1;
  }
  
  FDDEBUG(fprintf(stderr,"ReadHANDLE=%d WriteHANDLE=%d\n",files[0],files[1]));
  
  SetHandleInformation(files[0],HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);
  SetHandleInformation(files[1],HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);
  mt_lock(&fd_mutex);
  fds[0]=first_free_handle;
  first_free_handle=fd_type[fds[0]];
  fd_type[fds[0]]=FD_PIPE;
  da_handle[fds[0]]=(long)files[0];

  fds[1]=first_free_handle;
  first_free_handle=fd_type[fds[1]];
  fd_type[fds[1]]=FD_PIPE;
  da_handle[fds[1]]=(long)files[1];

  mt_unlock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"New pipe: %d (%d) -> %d (%d)\n",fds[0],files[0], fds[1], fds[1]));;

#ifdef DEBUG_MALLOC
  debug_malloc_register_fd( fds[0], dmalloc_file, dmalloc_line );
  debug_malloc_register_fd( fds[1], dmalloc_file, dmalloc_line );
#endif
  
  return 0;
}

FD debug_fd_accept(FD fd, struct sockaddr *addr, int *addrlen)
{
  FD new_fd;
  SOCKET s;
  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"Accept on %d (%d)..\n",fd,da_handle[fd]));
  if(first_free_handle == FD_NO_MORE_FREE)
  {
    mt_unlock(&fd_mutex);
    errno=EMFILE;
    return -1;
  }
  if(fd_type[fd]!=FD_SOCKET)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1;
  }
  s=(SOCKET)da_handle[fd];
  mt_unlock(&fd_mutex);
  s=accept(s, addr, addrlen);
  if(s==INVALID_SOCKET)
  {
    errno=WSAGetLastError();
    FDDEBUG(fprintf(stderr,"Accept failed with errno %d\n",errno));
    return -1;
  }
  
  SetHandleInformation(s,HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);
  mt_lock(&fd_mutex);
  new_fd=first_free_handle;
  first_free_handle=fd_type[new_fd];
  fd_type[new_fd]=FD_SOCKET;
  da_handle[new_fd]=(long)s;

  FDDEBUG(fprintf(stderr,"Accept on %d (%d) returned new socket: %d (%d)\n",fd,da_handle[fd],new_fd,s));

  mt_unlock(&fd_mutex);

  return new_fd;
}


#define SOCKFUN(NAME,X1,X2) \
int PIKE_CONCAT(debug_fd_,NAME) X1 { SOCKET ret; \
  FDDEBUG(fprintf(stderr, #NAME " on %d (%d)\n",fd,da_handle[fd])); \
  mt_lock(&fd_mutex); \
  if(fd_type[fd] != FD_SOCKET) { \
     mt_unlock(&fd_mutex); \
     errno=ENOTSUPP; \
     return -1; \
   } \
  ret=(SOCKET)da_handle[fd]; \
  mt_unlock(&fd_mutex); \
   ret=NAME X2; \
   if(ret == SOCKET_ERROR) errno=WSAGetLastError(); \
   FDDEBUG(fprintf(stderr, #NAME " returned %d (%d)\n",ret,errno)); \
   return (int)ret; \
}

#define SOCKFUN1(NAME,T1) \
   SOCKFUN(NAME, (FD fd, T1 a), (ret, a) )

#define SOCKFUN2(NAME,T1,T2) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b), (ret, a, b) )

#define SOCKFUN3(NAME,T1,T2,T3) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b, T3 c), (ret, a, b, c) )

#define SOCKFUN4(NAME,T1,T2,T3,T4) \
   SOCKFUN(NAME, (FD fd,T1 a,T2 b,T3 c,T4 d), (ret,a,b,c,d) )

#define SOCKFUN5(NAME,T1,T2,T3,T4,T5) \
   SOCKFUN(NAME, (FD fd,T1 a,T2 b,T3 c,T4 d,T5 e), (ret,a,b,c,d,e))


SOCKFUN2(bind, struct sockaddr *, int)
SOCKFUN4(getsockopt,int,int,void*,int*)
SOCKFUN4(setsockopt,int,int,void*,int)
SOCKFUN3(recv,void *,int,int)
SOCKFUN2(getsockname,struct sockaddr *,int *)
SOCKFUN2(getpeername,struct sockaddr *,int *)
SOCKFUN5(recvfrom,void *,int,int,struct sockaddr *,int*)
SOCKFUN3(send,void *,int,int)
SOCKFUN5(sendto,void *,int,int,struct sockaddr *,int*)
SOCKFUN1(shutdown, int)
SOCKFUN1(listen, int)

int debug_fd_connect (FD fd, struct sockaddr *a, int len)
{
  SOCKET ret;
  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr, "connect on %d (%d)\n",fd,da_handle[fd]);
	  for(ret=0;ret<len;ret++)
	  fprintf(stderr," %02x",((unsigned char *)a)[ret]);
	  fprintf(stderr,"\n");
  )
  if(fd_type[fd] != FD_SOCKET)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1; 
  } 
  ret=(SOCKET)da_handle[fd];
  mt_unlock(&fd_mutex);
  ret=connect(ret,a,len); 
  if(ret == SOCKET_ERROR) errno=WSAGetLastError(); 
  FDDEBUG(fprintf(stderr, "connect returned %d (%d)\n",ret,errno)); 
  return (int)ret; 
}

int debug_fd_close(FD fd)
{
  long h;
  int type;
  mt_lock(&fd_mutex);
  h=(long)da_handle[fd];
  FDDEBUG(fprintf(stderr,"Closing %d (%d)\n",fd,da_handle[fd]));
  type=fd_type[fd];
  mt_unlock(&fd_mutex);
  switch(type)
  {
    case FD_SOCKET:
      if(closesocket((SOCKET)h))
      {
	errno=GetLastError();
	FDDEBUG(fprintf(stderr,"Closing %d (%d) failed with errno=%d\n",fd,da_handle[fd],errno));
	return -1;
      }
      break;

    default:
      if(!CloseHandle((HANDLE)h))
      {
	errno=GetLastError();
	return -1;
      }
  }
  FDDEBUG(fprintf(stderr,"%d (%d) closed\n",fd,da_handle[fd]));
  mt_lock(&fd_mutex);
  if(fd_type[fd]<FD_NO_MORE_FREE)
  {
    fd_type[fd]=first_free_handle;
    first_free_handle=fd;
  }
  mt_unlock(&fd_mutex);

  return 0;
}

long debug_fd_write(FD fd, void *buf, long len)
{
  DWORD ret;
  long handle;
  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"Writing %d bytes to %d (%d)\n",len,fd,da_handle[fd]));
  ret=fd_type[fd];
  handle=da_handle[fd];
  mt_unlock(&fd_mutex);
  
  switch(ret)
  {
    case FD_SOCKET:
      ret=send((SOCKET)handle, buf, len, 0);
      if(ret<0)
      {
	errno=WSAGetLastError();
	FDDEBUG(fprintf(stderr,"Write on %d failed (%d)\n",fd,errno));
	if (errno == ENOENT) {
	  /* UGLY kludge */
	  errno = WSAEWOULDBLOCK;
	}
	return -1;
      }
      FDDEBUG(fprintf(stderr,"Wrote %d bytes to %d)\n",len,fd));
      return ret;

    case FD_CONSOLE:
    case FD_FILE:
    case FD_PIPE:
      ret=0;
      if(!WriteFile((HANDLE)handle, buf, len, &ret,0) && ret<=0)
      {
	errno=GetLastError();
	FDDEBUG(fprintf(stderr,"Write on %d failed (%d)\n",fd,errno));
	return -1;
      }
      FDDEBUG(fprintf(stderr,"Wrote %d bytes to %d)\n",len,fd));
      return ret;

    default:
      errno=ENOTSUPP;
      return -1;
  }
}

long debug_fd_read(FD fd, void *to, long len)
{
  DWORD ret;
  int rret;
  long handle;

  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"Reading %d bytes from %d (%d) to %lx\n",len,fd,da_handle[fd],(int)(char *)to));
  ret=fd_type[fd];
  handle=da_handle[fd];
  mt_unlock(&fd_mutex);

  switch(ret)
  {
    case FD_SOCKET:
      rret=recv((SOCKET)handle, to, len, 0);
      if(rret<0)
      {
	errno=WSAGetLastError();
	FDDEBUG(fprintf(stderr,"Read on %d failed %ld\n",fd,errno));
	return -1;
      }
      FDDEBUG(fprintf(stderr,"Read on %d returned %ld\n",fd,rret));
      return rret;

    case FD_CONSOLE:
    case FD_FILE:
    case FD_PIPE:
      ret=0;
      if(!ReadFile((HANDLE)handle, to, len, &ret,0) && ret<=0)
      {
	errno=GetLastError();
	switch(errno)
	{
	  /* Pretend we reached the end of the file */
	  case ERROR_BROKEN_PIPE:
	    return 0;
	}
	FDDEBUG(fprintf(stderr,"Read failed %d\n",errno));
	return -1;
      }
      FDDEBUG(fprintf(stderr,"Read on %d returned %ld\n",fd,ret));
      return ret;

    default:
      errno=ENOTSUPP;
      return -1;
  }
}

long debug_fd_lseek(FD fd, long pos, int where)
{
  long ret;
  mt_lock(&fd_mutex);
  if(fd_type[fd]!=FD_FILE)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1;
  }
#if FILE_BEGIN != SEEK_SET || FILE_CURRENT != SEEK_CUR || FILE_END != SEEK_END
  switch(where)
  {
    case SEEK_SET: where=FILE_BEGIN; break;
    case SEEK_CUR: where=FILE_CURRENT; break;
    case SEEK_END: where=FILE_END; break;
  }
#endif
  ret=da_handle[fd];
  mt_unlock(&fd_mutex);

  ret=SetFilePointer((HANDLE)ret, pos, 0, where);
  if(ret == 0xffffffff)
  {
    errno=GetLastError();
    return -1;
  }
  return ret;
}

long debug_fd_ftruncate(FD fd, long len)
{
  long ret;
  LONG oldfp_lo, oldfp_hi;
  mt_lock(&fd_mutex);
  if(fd_type[fd]!=FD_FILE)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1;
  }
  ret=da_handle[fd];
  mt_unlock(&fd_mutex);

  oldfp_hi = 0;
  oldfp_lo = SetFilePointer((HANDLE)ret, 0, &oldfp_hi, FILE_CURRENT);
  if(oldfp_lo == 0xffffffff) {
    errno=GetLastError();
    if(errno != NO_ERROR)
      return -1;
  }
  if(SetFilePointer((HANDLE)ret, len, NULL, FILE_BEGIN) == 0xffffffff ||
     !SetEndOfFile((HANDLE)ret)) {
    errno=GetLastError();
    SetFilePointer((HANDLE)ret, oldfp_lo, &oldfp_hi, FILE_BEGIN);
    return -1;
  }
  if(SetFilePointer((HANDLE)ret, oldfp_lo, &oldfp_hi, FILE_BEGIN) == 0xffffffff) {
    errno=GetLastError();
    if(errno != NO_ERROR)
      return -1;
  }
  return 0;
}

int debug_fd_flock(FD fd, int oper)
{
  long ret;
  mt_lock(&fd_mutex);
  if(fd_type[fd]!=FD_FILE)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1;
  }
  ret=da_handle[fd];
  mt_unlock(&fd_mutex);

  if(oper & fd_LOCK_UN)
  {
    ret=UnlockFile((HANDLE)ret,
		   0,
		   0,
		   0xffffffff,
		   0xffffffff);
  }else{
    DWORD flags;
    OVERLAPPED tmp;
    MEMSET(&tmp, 0, sizeof(tmp));
    tmp.Offset=0;
    tmp.OffsetHigh=0;

    if(oper & fd_LOCK_EX)
      flags|=LOCKFILE_EXCLUSIVE_LOCK;

    if(oper & fd_LOCK_UN)
      flags|=LOCKFILE_FAIL_IMMEDIATELY;

    ret=LockFileEx((HANDLE)ret,
		   flags,
		   0,
		   0xffffffff,
		   0xffffffff,
		   &tmp);
  }
  if(ret<0)
  {
    errno=GetLastError();
    return -1;
  }
  
  return 0;
}


static long convert_filetime_to_time_t(FILETIME tmp)
{
  double t;
  t=tmp.dwHighDateTime * pow(2.0,32.0) + (double)tmp.dwLowDateTime;
  t/=10000000.0;
  t-=11644473600.0;
  return (long)floor(t);
}

int debug_fd_fstat(FD fd, struct stat *s)
{
  DWORD x;

  FILETIME c,a,m;
  FDDEBUG(fprintf(stderr,"fstat on %d (%d)\n",fd,da_handle[fd]));
  if(fd_type[fd]!=FD_FILE)
  {
    errno=ENOTSUPP;
    mt_unlock(&fd_mutex);
    return -1;
  }

  MEMSET(s, 0, sizeof(struct stat));
  s->st_nlink=1;

  switch(fd_type[fd])
  {
    case FD_SOCKET:
      s->st_mode=S_IFSOCK;
      break;

    default:
      switch(GetFileType((HANDLE)da_handle[fd]))
      {
	default:
	case FILE_TYPE_UNKNOWN: s->st_mode=0;        break;
	case FILE_TYPE_DISK:
	  s->st_mode=S_IFREG; 
	  s->st_size=GetFileSize((HANDLE)da_handle[fd],&x);
	  if(x)
	  {
	    s->st_size=0x7fffffff;
	  }
	  if(!GetFileTime((HANDLE)da_handle[fd], &c, &a, &m))
	  {
	    errno=GetLastError();
	    return -1;
	  }
	  s->st_ctime=convert_filetime_to_time_t(c);
	  s->st_atime=convert_filetime_to_time_t(a);
	  s->st_mtime=convert_filetime_to_time_t(m);
	  break;
	case FILE_TYPE_CHAR:    s->st_mode=S_IFCHR;  break;
	case FILE_TYPE_PIPE:    s->st_mode=S_IFIFO; break;
      }
  }
  s->st_mode |= 0666;
  return 0;
}

int debug_fd_select(int fds, FD_SET *a, FD_SET *b, FD_SET *c, struct timeval *t)
{
  int ret;
  ret=select(fds,a,b,c,t);
  if(ret==SOCKET_ERROR)
  {
    errno=WSAGetLastError();
    return -1;
  }
  return ret;
}


int debug_fd_ioctl(FD fd, int cmd, void *data)
{
  int ret;
  FDDEBUG(fprintf(stderr,"ioctl(%d (%d,%d,%p)\n",fd,da_handle[fd],cmd,data));
  switch(fd_type[fd])
  {
    case FD_SOCKET:
      ret=ioctlsocket((SOCKET)da_handle[fd], cmd, data);
      FDDEBUG(fprintf(stderr,"ioctlsocket returned %ld (%d)\n",ret,errno));
      if(ret==SOCKET_ERROR)
      {
	errno=WSAGetLastError();
	return -1;
      }
      return ret;

    default:
      errno=ENOTSUPP;
      return -1;
  }
}


FD debug_fd_dup(FD from)
{
  FD fd;
  HANDLE x,p=GetCurrentProcess();
#ifdef DEBUG
  if(fd_type[from]>=FD_NO_MORE_FREE)
    fatal("fd_dup() on file which is not open!\n");
#endif
  if(!DuplicateHandle(p,(HANDLE)da_handle[from],p,&x,0,0,DUPLICATE_SAME_ACCESS))
  {
    errno=GetLastError();
    return -1;
  }

  mt_lock(&fd_mutex);
  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=fd_type[from];
  da_handle[fd]=(long)x;
  mt_unlock(&fd_mutex);
  
  FDDEBUG(fprintf(stderr,"Dup %d (%d) to %d (%d)\n",from,da_handle[from],fd,x));
  return fd;
}

FD debug_fd_dup2(FD from, FD to)
{
  HANDLE x,p=GetCurrentProcess();
  if(!DuplicateHandle(p,(HANDLE)da_handle[from],p,&x,0,0,DUPLICATE_SAME_ACCESS))
  {
    errno=GetLastError();
    return -1;
  }

  mt_lock(&fd_mutex);
  if(fd_type[to] < FD_NO_MORE_FREE)
  {
    if(!CloseHandle((HANDLE)da_handle[to]))
    {
      errno=GetLastError();
      mt_unlock(&fd_mutex);
      return -1;
    }
  }else{
    int *prev,next;
    for(prev=&first_free_handle;(next=*prev) != FD_NO_MORE_FREE;prev=fd_type+next)
    {
      if(next==to)
      {
	*prev=fd_type[next];
	break;
      }
    }
  }
  fd_type[to]=fd_type[from];
  da_handle[to]=(long)x;
  mt_unlock(&fd_mutex);

  FDDEBUG(fprintf(stderr,"Dup2 %d (%d) to %d (%d)\n",from,da_handle[from],to,x));

  return to;
}

#endif /* HAVE_WINSOCK_H */

#ifdef EMULATE_DIRECT
DIR *opendir(char *dir)
{
  int len=strlen(dir);
  char *foo;
  DIR *ret=(DIR *)malloc(sizeof(DIR) + len+5);
  if(!ret)
  {
    errno=ENOMEM;
    return 0;
  }
  foo=sizeof(DIR) + (char *)ret;
  MEMCPY(foo, dir, len);

  if(len && foo[len-1]!='/') foo[len++]='/';
  foo[len++]='*';
  foo[len]=0;
/*  fprintf(stderr,"opendir(%s)\n",foo); */

  /* This may require appending a slash and a star... */
  ret->h=FindFirstFile( (LPCTSTR) foo, & ret->find_data);
  if(ret->h == INVALID_HANDLE_VALUE)
  {
    errno=ENOENT;
    free((char *)ret);
    return 0;
  }
  ret->first=1;
  return ret;
}

int readdir_r(DIR *dir, struct direct *tmp ,struct direct **d)
{
  if(dir->first)
  {
    *d=&dir->find_data;
    dir->first=0;
    return 0;
  }else{
    if(FindNextFile(dir->h,tmp))
    {
      *d=tmp;
      return 0;
    }
    *d=0;
    return 0;
  }
}

void closedir(DIR *dir)
{
  FindClose(dir->h);
  free((char *)dir);
}
#endif

#if 0

#ifdef FD_LINEAR
struct fd_mapper
{
  int size;
  void **data;
};

void init_fd_mapper(struct fd_mapper *x)
{
  x->size=64;
  x->data=(void **)xalloc(x->size*sizeof(void *));
}

void exit_fd_mapper(struct fd_mapper *x)
{
  free((char *)x->data);
}

void fd_mapper_set(struct fd_mapper *x, FD fd, void *data)
{
  while(fd>=x->size)
  {
    x->size*=2;
    x->data=(void **)realloc((char *)x->data, x->size*sizeof(void *));
    if(!x->data)
      fatal("Out of memory.\n");
    x->data=nd;
  }
  x->data[fd]=data;
  
}

void *fd_mapper_get(struct fd_mapper *x, FD fd)
{
  return x->data[fd];
}
#else /* FD_LINEAR */
struct fd_mapper_data
{
  FD x;
  void *data;
};
struct fd_mapper
{
  int num;
  int hsize;
  struct fd_mapper_data *data;
};

void init_fd_mapper(struct fd_mapper *x)
{
  int i;
  x->num=0;
  x->hsize=127;
  x->data=(struct fd_mapper_data *)xalloc(x->hsize*sizeof(struct fd_mapper_data));
  for(i=0;i<x->hsize;i++) x->data[i].fd=-1;
}

void exit_fd_mapper(struct fd_mapper *x)
{
  free((char *)x->data);
}

void fd_mapper_set(struct fd_mapper *x, FD fd, void *data)
{
  int hval;
  x->num++;
  if(x->num*2 > x->hsize)
  {
    struct fd_mapper_data *old=x->data;
    int i,old_size=x->hsize;
    x->hsize*=3;
    x->num=0;
    x->data=(struct fd_mapper_data *)xalloc(x->size*sizeof(struct fd_mapper_data *));
    for(i=0;i<x->size;i++) x->data[i].fd=-1;
    for(i=0;i<old_size;i++)
      if(old[i].fd!=-1)
	fd_mapper_set(x, old[i].fd, old[i].data);
  }

  hval=fd % x->hsize;
  while(x->data[hval].fd != -1)
  {
    hval++;
    if(hval==x->hsize) hval=0;
  }
  x->data[hval].fd=fd;
  x->data[hval].data=data;
}

void *fd_mapper_get(struct fd_mapper *x, FD fd)
{
  int hval=fd % x->hsize;
  while(x->data[hval].fd != fd)
  {
    hval++;
    if(hval==x->hsize) hval=0;
  }
  return x->data[hval].data;
}
#endif /* FD_LINEAR */


struct fd_data_hash
{
  FD fd;
  int key;
  struct fd_data_hash *next;
  void *data;
};

#define FD_DATA_PER_BLOCK 255

struct fd_data_hash_block
{
  struct fd_data_hash_block *next;
  struct fd_data_hash data[FD_DATA_PER_BLOCk];
};

static int keynum=0;
static unsigned int num_stored_keys=0;
static unsigned int hash_size=0;
static struct fd_data_hash *free_blocks=0;
static struct fd_data_hash **htable=0;
static fd_data_hash_block *hash_blocks=0;

int get_fd_data_key(void)
{
  return ++keynum;
}

void store_fd_data(FD fd, int key, void *data)
{
  struct fd_data_hash *p,**last;
  unsigned int hval=(fd + key * 53) % hash_size;

  for(last=htable[e];p=*last;last=&p->next)
  {
    if(p->fd == fd && p->key == key)
    {
      if(data)
      {
	p->data=data;
      }else{
	*last=p->next;
	p->next=free_blocks;
	free_blocks=p;
	num_stored_keys--;
      }
      return;
    }
  }
  if(!data) return;

  num_stored_keys++;

  if(num_stored_keys * 2 >= hash_size)
  {
    /* time to rehash */
    unsigned int h;
    unsigned int old_hsize=hash_size;
    unsigned fd_data_hash **old_htable=htable;
    if(!hash_size)
      hash_size=127;
    else
      hash_size*=3;

    htable=(struct fd_data_hash **)xalloc(hash_size * sizeof(struct fd_data_hash *));
    
    for(h=0;h<old_hsize;h++)
    {
      for(last=old_htable+e;p=*last;last=&p->next)
	store_fd_data(p->fd, p->key, p->data);
      *last=free_blocks;
      free_blocks=old_htable[h];
    }
    if(old_htable)
      free((char *)old_htable);
  }


  if(!free_blocks)
  {
    struct fd_data_hash_block *n;
    int e;
    n=ALLOC_STRUCT(fd_data_hash_block);
    n->next=hash_blocks;
    hash_blocks=n;
    for(e=0;e<FD_DATA_PER_BLOCK;e++)
    {
      n->data[e].next=free_blocks;
      free_blocks=n->data+e;
    }
  }
  
  p=free_blocks;
  free_blocks=p->next;
  p->fd=fd;
  p->key=key;
  p->data=data;
  p->next=htable[hval];
  htable[hval]=p;
}

void *get_fd_data(FD fd, int key)
{
  struct fd_data_hash *p,**last;
  unsigned int hval=(fd + key * 53) % hash_size;

  for(p=htable[hval];p;p=p->next)
    if(p->fd == fd && p->key == key)
      return p->data;

  return 0;
}


#define FD_EVENT_READ 1
#define FD_EVENT_WRITE 2
#define FD_EVENT_OOB 4

struct event
{
  int fd;
  int events;
};

#ifdef FDLIB_USE_SELECT

struct fd_waitor
{
  fd_FDSET rcustomers,wcustomers,xcustomers;
  fd_FDSET rtmp,wtmp,xtmp;
  FD last;
  int numleft;
  int max;
};

#define init_waitor(X) do { (X)->numleft=0; (X)->max=0; \
 fd_FDZERO(&X->rcustomers); \
 fd_FDZERO(&X->wcustomers); \
 fd_FDZERO(&X->xcustomers); \
 } while(0)

void fd_waitor_set_customer(struct fd_waitor *x, FD customer, int flags)
{
  if(flags & FD_EVENT_READ)
  {
    fd_FD_SET(& x->rcustomer, customer);
  }else{
    fd_FD_CLR(& x->rcustomer, customer);
  }

  if(flags & FD_EVENT_WRITE)
  {
    fd_FD_SET(& x->wcustomer, customer);
  }else{
    fd_FD_CLR(& x->wcustomer, customer);
  }

  if(flags & FD_EVENT_OOB)
  {
    fd_FD_SET(& x->xcustomer, customer);
  }else{
    fd_FD_CLR(& x->xcustomer, customer);
  }

  if(flags)
    if(customer>x->max) x->max=customer;
  else
    if(customer == x->max)
    {
      x->max--;
      while(
	    !fd_ISSET(& x->rcustomers,x->max) &&
	    !fd_ISSET(& x->wcustomers,x->max) &&
	    !fd_ISSET(& x->xcustomers,x->max)
	    )
	x->max--;
    }
}

int fd_waitor_idle(fd_waitor *x,
		  struct timeval *t,
		  struct event *e)
{
  int tmp;
  if(!x->numleft)
  {
    x->rtmp=x->rcustomers;
    x->wtmp=x->wcustomers;
    x->xtmp=x->xcustomers;

    tmp=select(x->max, & x->rtmp, & x->wtmp, & x->xtmp, &t);
    if(tmp<0) return 0;

    x->last=0;
    x->numleft=tmp;
  }
  while(x->numleft)
  {
    while(x->last<x->max)
    {
      int flags=
	(fd_FD_ISSET(& x->rtmp, x->last) ? FD_EVENT_READ : 0) |
	(fd_FD_ISSET(& x->wtmp, x->last) ? FD_EVENT_WRITE : 0) |
	(fd_FD_ISSET(& x->xtmp, x->last) ? FD_EVENT_OOB: 0);

      if(flags)
      {
	numleft--;

	e->fd=x->last
	e->event=flags;
	return 1;
      }
    }
  }
  return 0;
}

#endif /* FDLIB_USE_SELECT */

#ifdef FDLIB_USE_WAITFORMULTIPLEOBJECTS

#define FD_MAX 16384

struct fd_waitor
{
  int occupied;
  HANDLE customers[FD_MAX];
  FD pos_to_fd[FD_MAX];
  int fd_to_pos_key;
  int last_swap;
};

void fd_waitor_set_customer(fd_waitor *x, FD customer, int flags)
{
  HANDLE h=CreateEvent();
  x->customers[x->occupied]=h;
  x->pos_to_fd[x->occupied]=customer;
  fd_mapper_store(customer, x->fd_to_pos_key, x->occupied);
  x->occupied++;
}

void fd_waitor_remove_customer(fd_waitor *x, FD customer)
{
  int pos=(int)fd_mapper_get(customer, x->fd_to_pos_key);

  CloseHandle(x->customers[pos]);
  
  fd_mapper_store(customer, x->fd_to_pos_key, (void *)0);
  x->occupied--;
  if(x->occupied != pos)
  {
    x->customer[pos]=x->customer[x->occupied];
    x->pos_to_fd[pos]=x->pos_to_fd[x->occupied];
    fd_mapper_store(x->pos_to_fd[pos], x->fd_to_pos_key, (void *)pos);
  }
}

FD fd_waitor_idle(fd_waitor *x, struct timeval delay)
{
  DWORD ret,d=delay.tv_usec/1000;
  d+=MINIMUM(100000,delay.tv_sec) *1000;

  ret=WaitForMultipleObjects(x->occupied,
			     x->customers,
			     0,
			     delay);

  if(ret>= WAIT_OBJECT_0 && ret< WAIT_OBJECT_0 + x->occupied)
  {
    long tmp;
    ret-=WAIT_OBJECT_0;
    if(-- (x->last_swap) <= ret)
    {
      x->last_swap=x->occupied;
      if(x->occupied == ret) return ret;
    }
    tmp=customers[ret];
    customers[ret]=customers[x->last_swap];
    customers[x->last_swap]=tmp;

    tmp=pos_to_fd[ret];
    pos_to_fd[ret]=pos_to_fd[x->last_swap];
    pos_to_fd[x->last_swap]=tmp;

    fd_mapper_store(x->pos_to_fd[ret], x->fd_to_pos_key, ret);
    fd_mapper_store(x->pos_to_fd[x->last_swap], x->fd_to_pos_key, x->last_swap);
    return x->pos_to_fd[ret];
  }else{
    return -1;
  }
}

#endif /* FDLIB_USE_WAITFORMULTIPLEOBJECTS */

#endif /* 0 */
