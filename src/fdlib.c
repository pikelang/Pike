/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: fdlib.c,v 1.67 2004/11/18 01:57:10 mast Exp $
*/

#include "global.h"
#include "fdlib.h"
#include "pike_error.h"
#include <math.h>

#ifdef HAVE_WINSOCK_H

/* Old versions of the headerfiles don't have this constant... */
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

#include "threads.h"

static MUTEX_T fd_mutex;

HANDLE da_handle[MAX_OPEN_FILEDESCRIPTORS];
int fd_type[MAX_OPEN_FILEDESCRIPTORS];
int first_free_handle;

/* #define FD_DEBUG */

#ifdef FD_DEBUG
#define FDDEBUG(X) X
#else
#define FDDEBUG(X)
#endif

#define ISSEPARATOR(a) ((a) == '\\' || (a) == '/')

#ifdef PIKE_DEBUG
static int IsValidHandle(HANDLE h)
{
  __try {
    HANDLE ret;
    if(DuplicateHandle(GetCurrentProcess(),
			h,
			GetCurrentProcess(),
			&ret,
			0,
			0,
			DUPLICATE_SAME_ACCESS))
    {
      CloseHandle(ret);
    }
  }

  __except (1) {
    return 0;
  }

  return 1;
}

PMOD_EXPORT HANDLE CheckValidHandle(HANDLE h)
{
  if(!IsValidHandle(h))
    Pike_fatal("Invalid handle!\n");
  return h;
}
#endif

PMOD_EXPORT char *debug_fd_info(int fd)
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

PMOD_EXPORT int debug_fd_query_properties(int fd, int guess)
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
    Pike_fatal("No winsock available.\n");
  }
  FDDEBUG(fprintf(stderr,"Using %s\n",wsadata.szDescription));
  
  fd_type[0] = FD_CONSOLE;
  da_handle[0] = GetStdHandle(STD_INPUT_HANDLE);
  fd_type[1] = FD_CONSOLE;
  da_handle[1] = GetStdHandle(STD_OUTPUT_HANDLE);
  fd_type[2] = FD_CONSOLE;
  da_handle[2] = GetStdHandle(STD_ERROR_HANDLE);

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

static INLINE time_t convert_filetime_to_time_t(FILETIME tmp)
{
#ifdef INT64
  return (((INT64) tmp.dwHighDateTime << 32)
	  + tmp.dwLowDateTime
	  - 116444736000000000) / 10000000;
#else
  double t;
  /* FILETIME is in 100ns since Jan 01, 1601 00:00 UTC.
   *
   * Offset to Jan 01, 1970 is thus 0x019db1ded53e8000 * 100ns.
   */
  if (tmp.dwLowDateTime < 0xd53e8000UL) {
    tmp.dwHighDateTime -= 0x019db1dfUL;	/* Note: Carry! */
    tmp.dwLowDateTime += 0x2ac18000UL;	/* Note: 2-compl */
  } else {
    tmp.dwHighDateTime -= 0x019db1deUL;
    tmp.dwLowDateTime -= 0xd53e8000UL;
  }
  t=tmp.dwHighDateTime * pow(2.0,32.0) + (double)tmp.dwLowDateTime;

  /* 1s == 10000000 * 100ns. */
  t/=10000000.0;
  return DO_NOT_WARN((long)floor(t));
#endif
}

/* The following replaces _stat in MS CRT since it's buggy on ntfs.
 * Axel Siebert explains:
 *
 *   On NTFS volumes, the time is stored as UTC, so it's easy to
 *   calculate the difference to January 1, 1601 00:00 UTC directly.
 *
 *   On FAT volumes, the time is stored as local time, as a heritage
 *   from MS-DOS days. This means that this local time must be
 *   converted to UTC first before calculating the FILETIME. During
 *   this conversion, all Windows versions exhibit the same bug, using
 *   the *current* DST status instead of the DST status at that time.
 *   So the FILETIME value can be off by one hour for FAT volumes.
 *
 *   Now the CRT _stat implementation uses FileTimeToLocalFileTime to
 *   convert this value back to local time - which suffers from the
 *   same DST bug. Then the undocumented function __loctotime_t is
 *   used to convert this local time to a time_t, and this function
 *   correctly accounts for DST at that time.
 *
 *   On FAT volumes, the conversion error while retrieving the
 *   FILETIME, and the conversion error of FileTimeToLocalFileTime
 *   exactly cancel each other out, so the final result of the _stat
 *   implementation is always correct.
 *
 *   On NTFS volumes however, there is no conversion error while
 *   retrieving the FILETIME because the file system already stores it
 *   as UTC, so the conversion error of FileTimeToLocalFileTime can
 *   cause the final result to be 1 hour off.
 *
 * The following implementation tries to do the correct thing based in
 * the filesystem type.
 */

static int fat_filetime_to_time_t (FILETIME *in, time_t *out)
{
  FILETIME local_ft;
  SYSTEMTIME local_st;
  if (!FileTimeToLocalFileTime (in, &local_ft) ||
      !FileTimeToSystemTime (&local_ft, &local_st)) {
    _dosmaperr (GetLastError());
    return 0;
  }
  else {
    *out = __loctotime_t (local_st.wYear, local_st.wMonth, local_st.wDay,
			  local_st.wHour, local_st.wMinute, local_st.wSecond,
			  -1);
    return 1;
  }
}

static int fat_filetimes_to_stattimes (FILETIME *creation,
				       FILETIME *last_access,
				       FILETIME *last_write,
				       PIKE_STAT_T *stat)
{
  if (!fat_filetime_to_time_t (last_write, &stat->st_mtime))
    return -1;

  if (last_access->dwLowDateTime || last_access->dwHighDateTime) {
    if (!fat_filetime_to_time_t (last_access, &stat->st_atime))
      return -1;
  }
  else
    stat->st_atime = stat->st_mtime;

  /* Putting the creation time in the last status change field.. :\ */
  if (creation->dwLowDateTime || creation->dwHighDateTime) {
    if (!fat_filetime_to_time_t (creation, &stat->st_ctime))
      return -1;
  }
  else
    stat->st_ctime = stat->st_mtime;
}

static void nonfat_filetimes_to_stattimes (FILETIME *creation,
					   FILETIME *last_access,
					   FILETIME *last_write,
					   PIKE_STAT_T *stat)
{
  buf->st_mtime = convert_filetime_to_time_t (findbuf.ftLastWriteTime);

  if (findbuf.ftLastAccessTime.dwLowDateTime ||
      findbuf.ftLastAccessTime.dwHighDateTime)
    buf->st_atime = convert_filetime_to_time_t (findbuf.ftLastAccessTime);
  else
    buf->st_atime = buf->st_mtime;

  /* Putting the creation time in the last status change field.. :\ */
  if (findbuf.ftCreationTime.dwLowDateTime ||
      findbuf.ftCreationTime.dwHighDateTime)
    buf->st_ctime = convert_filetime_to_time_t (findbuf.ftCreationTime);
  else
    buf->st_ctime = buf->st_mtime;
}

/*
 * IsUncRoot - returns TRUE if the argument is a UNC name specifying a
 *             root share.  That is, if it is of the form
 *             \\server\share\.  This routine will also return true if
 *             the argument is of the form \\server\share (no trailing
 *             slash).
 *
 *             Forward slashes ('/') may be used instead of
 *             backslashes ('\').
 */

static int IsUncRoot(char *path)
{
  /* root UNC names start with 2 slashes */
  if ( strlen(path) >= 5 &&     /* minimum string is "//x/y" */
       ISSEPARATOR(path[0]) &&
       ISSEPARATOR(path[1]) )
  {
    char * p = path + 2 ;
    
    /* find the slash between the server name and share name */
    while ( *++p )
      if ( ISSEPARATOR(*p) )
        break ;
    
    if ( *p && p[1] )
    {
      /* is there a further slash? */
      while ( *++p )
        if ( ISSEPARATOR(*p) )
          break ;
      
      /* final slash (if any) */
      if ( !*p || !p[1])
        return 1;
    }
  }
  
  return 0 ;
}

/* Note 1: s->st_mtime is the creation time for directories.
 * Note 2: s->st_ctime is set to the file creation time. It should
 * probably be the last access time to be closer to the unix
 * counterpart, but the creation time is admittedly more useful. */
int debug_fd_stat(const char *file, PIKE_STAT_T *buf)
{
  ptrdiff_t l = strlen(file);
  char fname[MAX_PATH];
  int drive;       		/* current = -1, A: = 0, B: = 1, ... */
  HANDLE hFind;
  WIN32_FIND_DATA findbuf;

  if(ISSEPARATOR (file[l-1]))
  {
    do l--;
    while(l && ISSEPARATOR (file[l]));
    l++;
    if(l+1 > sizeof(fname))
    {
      errno=EINVAL;
      return -1;
    }
    MEMCPY(fname, file, l);
    fname[l]=0;
    file=fname;
  }

  /* don't allow wildcards */
  if (strpbrk(file, "?*"))
  {
    errno = ENOENT;
    return(-1);
  }
  
  /* get disk from file */
  if (file[1] == ':')
  {
    if ( *file && !file[2] )
    {
      /* return an error if file is just drive letter and colon */
      errno = ENOENT;           
      return( -1 );
    }
    drive = toupper(*file) - 'A';
  }
  else
    drive = -1;
  
  /* get info for file */
  hFind = FindFirstFile(file, &findbuf);

  if ( hFind == INVALID_HANDLE_VALUE )
  {
    char abspath[ _MAX_PATH ];
    if ( !(strpbrk(file, "./\\") &&
	   _fullpath( abspath, file, _MAX_PATH ) &&
           /* root dir. ('C:\') or UNC root dir. ('\\server\share\') */
	   ((strlen( abspath ) == 3) || IsUncRoot(abspath)) &&
	   (GetDriveType( abspath ) > 1) ) )
    {
      errno = ENOENT;
      return( -1 );
    }
    
    /* Root directories ( C:\ and \\server\share\) are faked */
    findbuf.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    findbuf.nFileSizeHigh = 0;
    findbuf.nFileSizeLow = 0;
    findbuf.cFileName[0] = '\0';
    
    buf->st_mtime = __loctotime_t(1980,1,1,0,0,0, -1);
    buf->st_atime = buf->st_mtime;
    buf->st_ctime = buf->st_mtime;
  }

  else {
    char fstype[5]; /* Room for "FAT" and anything longer that begins with "FAT". */
    BOOL res;

    if (drive >= 0) {
      char root[4]; /* Room for "X:\" */
      root[0] = drive + 'A';
      root[1] = ':', root[2] = '\\', root[3] = 0;
      res = GetVolumeInformation (root, NULL, 0, NULL, NULL,NULL,
				  &fstype, sizeof (fstype));
    }
    else
      res = GetVolumeInformation (NULL, NULL, 0, NULL, NULL,NULL,
				  &fstype, sizeof (fstype));
    if (!res) {
      _dosmaperr (GetLastError());
      FindClose (hFind);
      return -1;
    }

    if (!strcmp (fstype, "FAT")) {
      if (!fat_filetimes_to_stattimes (&findbuf.ftCreationTime,
				       &findbuf.ftLastAccessTime,
				       &findbuf.ftLastWriteTime,
				       buf)) {
	FindClose (hFind);
	return -1;
      }
    }
    else
      /* Any non-fat filesystem is assumed to have sane timestamps. */
      nonfat_filetimes_to_stattimes (&findbuf.ftCreationTime,
				     &findbuf.ftLastAccessTime,
				     &findbuf.ftLastWriteTime,
				     buf);

    FindClose(hFind);
  }
  
  buf->st_mode = __dtoxmode(findbuf.dwFileAttributes, file);
  buf->st_nlink = 1;
#ifdef INT64
  buf->st_size = ((INT64) findbuf.nFileSizeHigh << 32) + findbuf.nFileSizeLow;
#else
  if (findbuf.nFileSizeHigh)
    buf->st_size = MAXDWORD;
  else
    buf->st_size = findbuf.nFileSizeLow;
#endif
  
  buf->st_uid = buf->st_gid = buf->st_ino = 0; /* unused entries */
  buf->st_rdev = buf->st_dev =
    (_dev_t) (drive >= 0 ? drive : _getdrive() - 1); /* A=0, B=1, ... */

  return(0);
}

PMOD_EXPORT FD debug_fd_open(const char *file, int open_mode, int create_mode)
{
  HANDLE x;
  FD fd;
  DWORD omode,cmode,amode;

  ptrdiff_t l = strlen(file);
  char fname[MAX_PATH];

  if(ISSEPARATOR (file[l-1]))
  {
    do l--;
    while(l && ISSEPARATOR (file[l]));
    l++;
    if(l+1 > sizeof(fname))
    {
      errno=EINVAL;
      return -1;
    }
    MEMCPY(fname, file, l);
    fname[l]=0;
    file=fname;
  }

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

  if(create_mode & 0222)
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

  
  if(x == DO_NOT_WARN(INVALID_HANDLE_VALUE))
  {
    errno=GetLastError();
    return -1;
  }

  SetHandleInformation(x,HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);

  mt_lock(&fd_mutex);

  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=FD_FILE;
  da_handle[fd] = x;

  mt_unlock(&fd_mutex);

  if(open_mode & fd_APPEND)
    fd_lseek(fd,0,SEEK_END);

  FDDEBUG(fprintf(stderr,"Opened %s file as %d (%d)\n",file,fd,x));

  return fd;
}

PMOD_EXPORT FD debug_fd_socket(int domain, int type, int proto)
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
  
  SetHandleInformation((HANDLE)s,
		       HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
  mt_lock(&fd_mutex);
  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd] = FD_SOCKET;
  da_handle[fd] = (HANDLE)s;
  mt_unlock(&fd_mutex);

  FDDEBUG(fprintf(stderr,"New socket: %d (%d)\n",fd,s));

  return fd;
}

PMOD_EXPORT int debug_fd_pipe(int fds[2] DMALLOC_LINE_ARGS)
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
  da_handle[fds[0]] = files[0];

  fds[1]=first_free_handle;
  first_free_handle=fd_type[fds[1]];
  fd_type[fds[1]]=FD_PIPE;
  da_handle[fds[1]] = files[1];

  mt_unlock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"New pipe: %d (%d) -> %d (%d)\n",fds[0],files[0], fds[1], fds[1]));;

#ifdef DEBUG_MALLOC
  debug_malloc_register_fd( fds[0], DMALLOC_LOCATION());
  debug_malloc_register_fd( fds[1], DMALLOC_LOCATION());
#endif
  
  return 0;
}

PMOD_EXPORT FD debug_fd_accept(FD fd, struct sockaddr *addr,
			       ACCEPT_SIZE_T *addrlen)
{
  FD new_fd;
  SOCKET s;
  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"Accept on %d (%ld)..\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));
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
  
  SetHandleInformation((HANDLE)s,
		       HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
  mt_lock(&fd_mutex);
  new_fd=first_free_handle;
  first_free_handle=fd_type[new_fd];
  fd_type[new_fd]=FD_SOCKET;
  da_handle[new_fd] = (HANDLE)s;

  FDDEBUG(fprintf(stderr,"Accept on %d (%ld) returned new socket: %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]),
		  new_fd, PTRDIFF_T_TO_LONG((ptrdiff_t)s)));

  mt_unlock(&fd_mutex);

  return new_fd;
}


#define SOCKFUN(NAME,X1,X2) \
PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) X1 { \
  SOCKET s; \
  int ret; \
  FDDEBUG(fprintf(stderr, #NAME " on %d (%ld)\n", \
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]))); \
  mt_lock(&fd_mutex); \
  if(fd_type[fd] != FD_SOCKET) { \
     mt_unlock(&fd_mutex); \
     errno = ENOTSUPP; \
     return -1; \
   } \
  s = (SOCKET)da_handle[fd]; \
  mt_unlock(&fd_mutex); \
  ret = NAME X2; \
  if(ret == SOCKET_ERROR) { \
    errno = WSAGetLastError(); \
    ret = -1; \
  } \
  FDDEBUG(fprintf(stderr, #NAME " returned %d (%d)\n", ret, errno)); \
  return ret; \
}

#define SOCKFUN1(NAME,T1) \
   SOCKFUN(NAME, (FD fd, T1 a), (s, a) )

#define SOCKFUN2(NAME,T1,T2) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b), (s, a, b) )

#define SOCKFUN3(NAME,T1,T2,T3) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b, T3 c), (s, a, b, c) )

#define SOCKFUN4(NAME,T1,T2,T3,T4) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b, T3 c, T4 d), (s, a, b, c, d) )

#define SOCKFUN5(NAME,T1,T2,T3,T4,T5) \
   SOCKFUN(NAME, (FD fd, T1 a, T2 b, T3 c, T4 d, T5 e), (s, a, b, c, d, e))


SOCKFUN2(bind, struct sockaddr *, int)
SOCKFUN4(getsockopt,int,int,void*,ACCEPT_SIZE_T *)
SOCKFUN4(setsockopt,int,int,void*,int)
SOCKFUN3(recv,void *,int,int)
SOCKFUN2(getsockname,struct sockaddr *,ACCEPT_SIZE_T *)
SOCKFUN2(getpeername,struct sockaddr *,ACCEPT_SIZE_T *)
SOCKFUN5(recvfrom,void *,int,int,struct sockaddr *,ACCEPT_SIZE_T *)
SOCKFUN3(send,void *,int,int)
SOCKFUN5(sendto,void *,int,int,struct sockaddr *,unsigned int)
SOCKFUN1(shutdown, int)
SOCKFUN1(listen, int)

PMOD_EXPORT int debug_fd_connect (FD fd, struct sockaddr *a, int len)
{
  SOCKET ret;
  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr, "connect on %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]));
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
  return DO_NOT_WARN((int)ret);
}

PMOD_EXPORT int debug_fd_close(FD fd)
{
  HANDLE h;
  int type;
  mt_lock(&fd_mutex);
  h = da_handle[fd];
  FDDEBUG(fprintf(stderr,"Closing %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));
  type=fd_type[fd];
  mt_unlock(&fd_mutex);
  switch(type)
  {
    case FD_SOCKET:
      if(closesocket((SOCKET)h))
      {
	errno=GetLastError();
	FDDEBUG(fprintf(stderr,"Closing %d (%ld) failed with errno=%d\n",
			fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]),
			errno));
	return -1;
      }
      break;

    default:
      if(!CloseHandle(h))
      {
	errno=GetLastError();
	return -1;
      }
  }
  FDDEBUG(fprintf(stderr,"%d (%ld) closed\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));
  mt_lock(&fd_mutex);
  if(fd_type[fd]<FD_NO_MORE_FREE)
  {
    fd_type[fd]=first_free_handle;
    first_free_handle=fd;
  }
  mt_unlock(&fd_mutex);

  return 0;
}

PMOD_EXPORT ptrdiff_t debug_fd_write(FD fd, void *buf, ptrdiff_t len)
{
  int kind;
  HANDLE handle;

  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr, "Writing %d bytes to %d (%d)\n",
		  len, fd, da_handle[fd]));
  kind = fd_type[fd];
  handle = da_handle[fd];
  mt_unlock(&fd_mutex);
  
  switch(kind)
  {
    case FD_SOCKET:
      {
	ptrdiff_t ret = send((SOCKET)handle, buf,
			     DO_NOT_WARN((int)len),
			     0);
	if(ret<0)
	{
	  errno = WSAGetLastError();
	  FDDEBUG(fprintf(stderr, "Write on %d failed (%d)\n", fd, errno));
	  if (errno == 1) {
	    /* UGLY kludge */
	    errno = WSAEWOULDBLOCK;
	  }
	  return -1;
	}
	FDDEBUG(fprintf(stderr, "Wrote %d bytes to %d)\n", len, fd));
	return ret;
      }

    case FD_CONSOLE:
    case FD_FILE:
    case FD_PIPE:
      {
	DWORD ret = 0;
	if(!WriteFile(handle, buf,
		      DO_NOT_WARN((DWORD)len),
		      &ret,0) && ret<=0)
	{
	  errno = GetLastError();
	  FDDEBUG(fprintf(stderr, "Write on %d failed (%d)\n", fd, errno));
	  return -1;
	}
	FDDEBUG(fprintf(stderr, "Wrote %ld bytes to %d)\n", (long)ret, fd));
	return ret;
      }

    default:
      errno=ENOTSUPP;
      return -1;
  }
}

PMOD_EXPORT ptrdiff_t debug_fd_read(FD fd, void *to, ptrdiff_t len)
{
  DWORD ret;
  ptrdiff_t rret;
  HANDLE handle;

  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr,"Reading %d bytes from %d (%d) to %lx\n",
		  len, fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]),
		  PTRDIFF_T_TO_LONG((ptrdiff_t)to)));
  ret=fd_type[fd];
  handle=da_handle[fd];
  mt_unlock(&fd_mutex);

  switch(ret)
  {
    case FD_SOCKET:
      rret=recv((SOCKET)handle, to,
		DO_NOT_WARN((int)len),
		0);
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
      if(len && !ReadFile(handle, to,
			  DO_NOT_WARN((DWORD)len),
			  &ret,0) && ret<=0)
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

PMOD_EXPORT PIKE_OFF_T debug_fd_lseek(FD fd, PIKE_OFF_T pos, int where)
{
  PIKE_OFF_T ret;
  HANDLE h;

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
  h = da_handle[fd];
  mt_unlock(&fd_mutex);

#ifdef INT64
  if (pos >= ((INT64) 1 << 32)) {
    LONG high = DO_NOT_WARN ((LONG) (pos >> 32));
    DWORD err;
    pos &= (1 << 32) - 1;
    ret = SetFilePointer (h, DO_NOT_WARN ((LONG) pos), &high, where);
    if (ret == INVALID_SET_FILE_POINTER &&
	(err = GetLastError()) != NO_ERROR) {
      errno = err;
      return -1;
    }
    ret += (INT64) high << 32;
  }
  else
#endif
  {
    ret = SetFilePointer (h, DO_NOT_WARN ((LONG) pos), NULL, where);
    if(ret == INVALID_SET_FILE_POINTER)
    {
      errno=GetLastError();
      return -1;
    }
  }

  return ret;
}

PMOD_EXPORT int debug_fd_ftruncate(FD fd, PIKE_OFF_T len)
{
  HANDLE h;
  LONG oldfp_lo, oldfp_hi, len_hi;
  DWORD err;

  mt_lock(&fd_mutex);
  if(fd_type[fd]!=FD_FILE)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1;
  }
  h = da_handle[fd];
  mt_unlock(&fd_mutex);

  oldfp_hi = 0;
  oldfp_lo = SetFilePointer(h, 0, &oldfp_hi, FILE_CURRENT);
  if(!~oldfp_lo) {
    err = GetLastError();
    if(err != NO_ERROR) {
      errno = err;
      return -1;
    }
  }

#ifdef INT64
  len_hi = DO_NOT_WARN ((LONG) (len >> 32));
  len &= (1 << 32) - 1;
#else
  len_hi = 0;
#endif

  if (SetFilePointer (h, DO_NOT_WARN ((LONG) len), &len_hi, FILE_BEGIN) ==
      INVALID_SET_FILE_POINTER &&
      (err = GetLastError()) != NO_ERROR) {
    SetFilePointer(h, oldfp_lo, &oldfp_hi, FILE_BEGIN);
    errno = err;
    return -1;
  }

  if(!SetEndOfFile(h)) {
    errno=GetLastError();
    SetFilePointer(h, oldfp_lo, &oldfp_hi, FILE_BEGIN);
    return -1;
  }

  if (oldfp_hi < len_hi || oldfp_hi == len_hi && oldfp_lo < len)
    if(!~SetFilePointer(h, oldfp_lo, &oldfp_hi, FILE_BEGIN)) {
      err = GetLastError();
      if(err != NO_ERROR) {
	errno = err;
	return -1;
      }
    }

  return 0;
}

PMOD_EXPORT int debug_fd_flock(FD fd, int oper)
{
  long ret;
  HANDLE h;
  mt_lock(&fd_mutex);
  if(fd_type[fd]!=FD_FILE)
  {
    mt_unlock(&fd_mutex);
    errno=ENOTSUPP;
    return -1;
  }
  h = da_handle[fd];
  mt_unlock(&fd_mutex);

  if(oper & fd_LOCK_UN)
  {
    ret=UnlockFile(h,
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

    ret=LockFileEx(h,
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


/* Note: s->st_ctime is set to the file creation time. It should
 * probably be the last access time to be closer to the unix
 * counterpart, but the creation time is admittedly more useful. */
PMOD_EXPORT int debug_fd_fstat(FD fd, PIKE_STAT_T *s)
{
  FILETIME c,a,m;

  mt_lock(&fd_mutex);
  FDDEBUG(fprintf(stderr, "fstat on %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));
  if(fd_type[fd]!=FD_FILE)
  {
    errno=ENOTSUPP;
    mt_unlock(&fd_mutex);
    return -1;
  }

  MEMSET(s, 0, sizeof(PIKE_STAT_T));
  s->st_nlink=1;

  switch(fd_type[fd])
  {
    case FD_SOCKET:
      s->st_mode=S_IFSOCK;
      break;

    default:
      switch(GetFileType(da_handle[fd]))
      {
	default:
	case FILE_TYPE_UNKNOWN: s->st_mode=0;        break;

	case FILE_TYPE_DISK:
	  s->st_mode=S_IFREG;
	  {
	    DWORD high, err;
	    s->st_size=GetFileSize(da_handle[fd],&high);
	    if (s->st_size == INVALID_FILE_SIZE &&
		(err = GetLastError()) != NO_ERROR) {
	      errno = err;
	      mt_unlock(&fd_mutex);
	      return -1;
	    }
#ifdef INT64
	    s->st_size += (INT64) high << 32;
#else
	    if (high) s->st_size = MAXDWORD;
#endif
	  }
	  if(!GetFileTime(da_handle[fd], &c, &a, &m))
	  {
	    _dosmaperr (GetLastError());
	    mt_unlock(&fd_mutex);
	    return -1;
	  }

	  /* FIXME: Determine the filesystem type to use
	   * fat_filetimes_to_stattimes when necessary. */

	  /* FIXME: Even if we use fat_filetimes_to_stattimes, the
	   * time conversion will still get incorrect in the time
	   * frame between a DST change and the next reboot. From msdn
	   * (http://msdn.microsoft.com/library/en-us/sysinfo/base/file_times.asp):
	   *
	   *   FAT records times on disk in local time. GetFileTime
	   *   retrieves cached UTC times from FAT. When it becomes
	   *   daylight saving time, the time retrieved by GetFileTime
	   *   will be off an hour, because the cache has not been
	   *   updated. When you restart the machine, the cached time
	   *   retrieved by GetFileTime will be correct.
	   *
	   * We'd have to look at the uptime to correct for that.
	   * *puke* */

	  nonfat_filetimes_to_stattimes (&c, &a, &m, s);
	  break;

	case FILE_TYPE_CHAR:    s->st_mode=S_IFCHR;  break;
	case FILE_TYPE_PIPE:    s->st_mode=S_IFIFO; break;
      }
  }
  s->st_mode |= 0666;
  mt_unlock(&fd_mutex);
  return 0;
}


#ifdef FD_DEBUG
static void dump_FDSET(FD_SET *x, int fds)
{
  if(x)
  {
    int e, first=1;
    fprintf(stderr,"[");
    for(e=0;e<fds;e++)
    {
      if(FD_ISSET(da_handle[e],x))
      {
	if(!first) fprintf(stderr,",",e);
	fprintf(stderr,"%d",e);
	first=0;
      }
    }
    fprintf(stderr,"]");
  }else{
    fprintf(stderr,"0");
  }
}
#endif

/* FIXME:
 * select with no fds should call Sleep()
 * If the backend works correctly, fds is zero when there are no fds.
 * /Hubbe
 */
PMOD_EXPORT int debug_fd_select(int fds, FD_SET *a, FD_SET *b, FD_SET *c, struct timeval *t)
{
  int ret;

  FDDEBUG(
    int e;
    fprintf(stderr,"Select(%d,",fds);
    dump_FDSET(a,fds);
    dump_FDSET(b,fds);
    dump_FDSET(c,fds);
    fprintf(stderr,",(%ld,%06ld));\n", (long) t->tv_sec,(long) t->tv_usec);
    )

  ret=select(fds,a,b,c,t);
  if(ret==SOCKET_ERROR)
  {
    errno=WSAGetLastError();
    FDDEBUG(fprintf(stderr,"select->%d, errno=%d\n",ret,errno));
    return -1;
  }

  FDDEBUG(
    fprintf(stderr,"    ->(%d,",fds);
    dump_FDSET(a,fds);
    dump_FDSET(b,fds);
    dump_FDSET(c,fds);
    fprintf(stderr,",(%ld,%06ld));\n", (long) t->tv_sec,(long) t->tv_usec);
    )

  return ret;
}


PMOD_EXPORT int debug_fd_ioctl(FD fd, int cmd, void *data)
{
  int ret;
  FDDEBUG(fprintf(stderr,"ioctl(%d (%ld,%d,%p)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]), cmd, data));
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


PMOD_EXPORT FD debug_fd_dup(FD from)
{
  FD fd;
  HANDLE x,p=GetCurrentProcess();

  mt_lock(&fd_mutex);
#ifdef DEBUG
  if(fd_type[from]>=FD_NO_MORE_FREE)
    Pike_fatal("fd_dup() on file which is not open!\n");
#endif
  if(!DuplicateHandle(p,da_handle[from],p,&x,0,0,DUPLICATE_SAME_ACCESS))
  {
    errno=GetLastError();
    mt_unlock(&fd_mutex);
    return -1;
  }

  fd=first_free_handle;
  first_free_handle=fd_type[fd];
  fd_type[fd]=fd_type[from];
  da_handle[fd] = x;
  mt_unlock(&fd_mutex);
  
  FDDEBUG(fprintf(stderr,"Dup %d (%ld) to %d (%d)\n",
		  from, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[from]), fd, x));
  return fd;
}

PMOD_EXPORT FD debug_fd_dup2(FD from, FD to)
{
  HANDLE x,p=GetCurrentProcess();

  mt_lock(&fd_mutex);
  if(!DuplicateHandle(p,da_handle[from],p,&x,0,0,DUPLICATE_SAME_ACCESS))
  {
    errno=GetLastError();
    mt_unlock(&fd_mutex);
    return -1;
  }

  if(fd_type[to] < FD_NO_MORE_FREE)
  {
    if(!CloseHandle(da_handle[to]))
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
  da_handle[to] = x;
  mt_unlock(&fd_mutex);

  FDDEBUG(fprintf(stderr,"Dup2 %d (%d) to %d (%d)\n",
		  from, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[from]), to, x));

  return to;
}

#endif /* HAVE_WINSOCK_H */

#ifdef EMULATE_DIRECT
PMOD_EXPORT DIR *opendir(char *dir)
{
  ptrdiff_t len=strlen(dir);
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
  if(ret->h == DO_NOT_WARN(INVALID_HANDLE_VALUE))
  {
    errno=ENOENT;
    free((char *)ret);
    return 0;
  }
  ret->first=1;
  return ret;
}

PMOD_EXPORT int readdir_r(DIR *dir, struct direct *tmp ,struct direct **d)
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

PMOD_EXPORT void closedir(DIR *dir)
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
      Pike_fatal("Out of memory.\n");
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
  
  fd_mapper_store(customer, x->fd_to_pos_key, NULL);
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
