#include "fdlib.h"

#ifdef HAVE_WINSOCK_H

#define FD_SOCKET -4
#define FD_CONSOLE -3
#define FD_FILE -2
#define FD_NO_MORE_FREE -1

long da_handle[MAX_OPEN_FILEDESCRIPTORS];
static int fd_type[MAX_OPEN_FILEDESCRIPTORS];
int first_free_handle;

void fd_init()
{
  int e;
  WSADATA wsadata;
  
  if(WSAStartup(MAKEWORD(2,0), &wsadata) != 0)
  {
    fatal("No winsock available.\n");
  }
/*  fprintf(stderr,"Using %s\n",wsadata.szDescription); */
  
  fd_type[0]=FD_CONSOLE;
  da_handle[0]=GetStdHandle(STD_INPUT_HANDLE);
  fd_type[1]=FD_CONSOLE;
  da_handle[1]=GetStdHandle(STD_OUTPUT_HANDLE);
  fd_type[2]=FD_CONSOLE;
  da_handle[2]=GetStdHandle(STD_ERROR_HANDLE);

  first_free_handle=3;
  for(e=3;e<MAX_OPEN_FILEDESCRIPTORS-1;e++)
    fd_type[e]=e+1;
  fd_type[e]=FD_NO_MORE_FREE;
}

void fd_exit()
{
  WSACleanup();
}

FD fd_open(char *file, int open_mode, int create_mode)
{
  HANDLE x;
  FD fd;
  DWORD omode,cmode,amode;
  omode=0;
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

  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=FD_FILE;
  da_handle[fd]=x;

  return fd;
}

FD fd_socket(int domain, int type, int proto)
{
  FD fd;
  SOCKET s;
  if(first_free_handle == FD_NO_MORE_FREE)
  {
    errno=EMFILE;
    return -1;
  }
  s=socket(domain, type, proto);
  if(s==INVALID_SOCKET)
  {
    errno=WSAGetLastError();
    return -1;
  }
  
  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=FD_SOCKET;
  da_handle[fd]=(HANDLE)s;

  return fd;
}

FD fd_accept(FD fd, struct sockaddr *addr, int *addrlen)
{
  FD new_fd;
  SOCKET s;
  if(first_free_handle == FD_NO_MORE_FREE)
  {
    errno=EMFILE;
    return -1;
  }
  if(fd_type[fd]!=FD_SOCKET)
  {
    errno=ENOTSUPP;
    return -1;
  }
  s=accept((SOCKET)da_handle[fd], addr, addrlen);
  if(s==INVALID_SOCKET)
  {
    errno=WSAGetLastError();
    return -1;
  }
  
  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=FD_SOCKET;
  da_handle[fd]=(HANDLE)s;
  return fd;
}

#define SOCKFUN(NAME,X1,X2) \
int PIKE_CONCAT(fd_,NAME) X1 { int ret; \
  if(fd_type[fd] != FD_SOCKET) { \
     errno=ENOTSUPP; \
     return -1; \
   } \
   ret=NAME X2; \
   if(ret == SOCKET_ERROR) errno=WSAGetLastError(); \
   return ret; \
}

#define SOCKFUN1(NAME,T1) \
   SOCKFUN(NAME, (FD fd, T1 a), ((SOCKET)da_handle[fd], a) )

#define SOCKFUN2(NAME,T1,T2) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b), ((SOCKET)da_handle[fd], a, b) )

#define SOCKFUN3(NAME,T1,T2,T3) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b, T3 c), ((SOCKET)da_handle[fd], a, b, c) )

#define SOCKFUN4(NAME,T1,T2,T3,T4) \
   SOCKFUN(NAME, (FD fd,T1 a,T2 b,T3 c,T4 d), ((SOCKET)da_handle[fd],a,b,c,d) )

#define SOCKFUN5(NAME,T1,T2,T3,T4,T5) \
   SOCKFUN(NAME, (FD fd,T1 a,T2 b,T3 c,T4 d,T5 e), ((SOCKET)da_handle[fd],a,b,c,d,e))


SOCKFUN2(bind, struct sockaddr *, int)
SOCKFUN2(connect, struct sockaddr *, int)
SOCKFUN4(getsockopt,int,int,void*,int*)
SOCKFUN4(setsockopt,int,int,void*,int)
SOCKFUN2(getsockname,struct sockaddr *,int*)
SOCKFUN2(getpeername,struct sockaddr *,int*)
SOCKFUN3(recv,void *,int,int)
SOCKFUN5(recvfrom,void *,int,int,struct sockaddr *,int*)
SOCKFUN5(sendto,void *,int,int,struct sockaddr *,int*)
SOCKFUN1(shutdown, int)
SOCKFUN1(listen, int)

int fd_close(FD fd)
{
  if(!CloseHandle(da_handle[fd]))
  {
    errno=GetLastError();
    return -1;
  }
  fd_type[fd]=first_free_handle;
  first_free_handle=fd;
  return 0;
}

long fd_write(FD fd, void *buf, long len)
{
  DWORD ret;
  switch(fd_type[fd])
  {
    case FD_SOCKET:
      ret=send((SOCKET)da_handle[fd], buf, len, 0);
      if(ret<0)
      {
	errno=WSAGetLastError();
	return -1;
      }
      return ret;

    case FD_CONSOLE:
    case FD_FILE:
      if(!WriteFile(da_handle[fd], buf, len, &ret,0) && !ret)
      {
	errno=GetLastError();
	return -1;
      }
      return ret;

    default:
      errno=ENOTSUPP;
      return -1;
  }
}

long fd_read(FD fd, void *buf, long len)
{
  DWORD ret;
  switch(fd_type[fd])
  {
    case FD_SOCKET:
      ret=recv((SOCKET)da_handle[fd], buf, len, 0);
      if(ret<0)
      {
	errno=WSAGetLastError();
	return -1;
      }
      return ret;

    case FD_CONSOLE:
    case FD_FILE:
      if(!ReadFile(da_handle[fd], buf, len, &ret,0) && !ret)
      {
	errno=GetLastError();
	return -1;
      }
      return ret;

    default:
      errno=ENOTSUPP;
      return -1;
  }
}

long fd_lseek(FD fd, long pos, int where)
{
  long ret;
  if(fd_type[fd]!=FD_FILE)
  {
    errno=ENOTSUPP;
    return -1;
  }

  ret=LZSeek(da_handle[fd], pos, where);
  if(ret<0)
  {
    errno=GetLastError();
    return -1;
  }
  return ret;
}

int fd_fstat(FD fd, struct stat *s)
{
  int ret;
  if(fd_type[fd]!=FD_FILE)
  {
    errno=ENOTSUPP;
    return -1;
  }

  ret=fstat(da_handle[fd], s);
  if(ret<0)
  {
    errno=GetLastError();
    return -1;
  }
  return ret;
}

int fd_select(int fds, FD_SET *a, FD_SET *b, FD_SET *c, struct timeval *t)
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


int fd_ioctl(FD fd, int cmd, void *data)
{
  int ret;
  switch(fd_type[fd])
  {
    case FD_SOCKET:
      ret=ioctlsocket((SOCKET)da_handle[fd], cmd, data);
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


FD fd_dup(FD fd)
{
  errno=ENOTSUPP;
  return -1;
}

FD fd_dup2(FD to, FD from)
{
  errno=ENOTSUPP;
  return -1;
}

#endif
