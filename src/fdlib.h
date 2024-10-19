/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#ifndef FDLIB_H
#define FDLIB_H

#include "global.h"
#include "pike_macros.h"

#include "pike_netlib.h"

#include <errno.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_SOCKET_H
#include <socket.h>
#endif /* HAVE_SOCKET_H */

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#endif

#include "pike_netlib.h"

#define fd_INTERPROCESSABLE   1
#define fd_CAN_NONBLOCK       2
#define fd_CAN_SHUTDOWN       4
#define fd_BUFFERED           8
#define fd_BIDIRECTIONAL     16
#define fd_REVERSE	     32
#define fd_SEND_FD           64
#define fd_TTY		    128


#ifdef HAVE_WINSOCK_H

#define HAVE_FD_FLOCK


#define FILE_CAPABILITIES (fd_INTERPROCESSABLE)
#define PIPE_CAPABILITIES (fd_INTERPROCESSABLE | fd_BUFFERED)
#define SOCKET_CAPABILITIES (fd_BIDIRECTIONAL | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN)
#define TTY_CAPABILITIES (fd_TTY | fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK)

#include <winbase.h>
#include <winioctl.h>

typedef int FD;

#if _INTEGRAL_MAX_BITS >= 64
typedef struct _stati64 PIKE_STAT_T;
typedef __int64 PIKE_OFF_T;
#define PRINTPIKEOFFT PRINTINT64
#else
typedef struct stat PIKE_STAT_T;
typedef off_t PIKE_OFF_T;
#define PRINTPIKEOFFT PRINTOFFT
#endif /* _INTEGRAL_MAX_BITS >= 64 */

/*
 * Some macros, structures and typedefs that are needed for ConPTY.
 */
#ifndef HAVE_HPCON
typedef VOID* HPCON;
#endif
#ifndef EXTENDED_STARTUPINFO_PRESENT
#define EXTENDED_STARTUPINFO_PRESENT 0x00080000
#endif
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif
#ifndef HAVE_LPPROC_THREAD_ATTRIBUTE_LIST
typedef struct _PROC_THREAD_ATTRIBUTE_ENTRY
{
  DWORD_PTR	Attribute;
  SIZE_T	cbSize;
  PVOID		lpValue;
} PROC_THREAD_ATTRIBUTE_ENTRY, *LPPROC_THREAD_ATTRIBUTE_ENTRY;
typedef struct _PROC_THREAD_ATTRIBUTE_LIST
{
  DWORD			dwFlags;
  ULONG			Size;
  ULONG			Count;
  ULONG			Reserved;
  PULONG		Unknown;
  PROC_THREAD_ATTRIBUTE_ENTRY	Entries[ANYSIZE_ARRAY];
} PROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;
#endif
#ifndef HAVE_LPSTARTUPINFOEXW
typedef struct _STARTUPINFOEXW
{
  STARTUPINFOW StartupInfo;
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList;
} STARTUPINFOEXW, *LPSTARTUPINFOEXW;
#endif

#ifndef HAVE_STRUCT_WINSIZE
/* Typically found in <termios.h>. */
struct winsize {
  unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
};
#endif

#ifndef TIOCGWINSZ
#define TIOCGWINSZ	_IOR('T', 0x13, struct winsize)
#define TIOCSWINSZ	_IOW('T', 0x14, struct winsize)
#endif

#ifdef HAVE__WUTIME64
#define fd_utimbuf	__utimbuf64
#else
#define fd_utimbuf	_utimbuf
#endif

#define SOCKFUN1(NAME,T1) PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) (FD,T1);
#define SOCKFUN2(NAME,T1,T2) PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) (FD,T1,T2);
#define SOCKFUN3(NAME,T1,T2,T3) PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) (FD,T1,T2,T3);
#define SOCKFUN4(NAME,T1,T2,T3,T4) PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) (FD,T1,T2,T3,T4);
#define SOCKFUN5(NAME,T1,T2,T3,T4,T5) PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) (FD,T1,T2,T3,T4,T5);


#define fd_info(fd) debug_fd_info(dmalloc_touch_fd(fd))
#define fd_isatty(fd) debug_fd_isatty(dmalloc_touch_fd(fd))
#define fd_query_properties(fd,Y) \
        debug_fd_query_properties(dmalloc_touch_fd(fd),(Y))
#define fd_stat(F,BUF) debug_fd_stat(F,BUF)
#define fd_lstat(F,BUF) debug_fd_stat(F,BUF)
#define fd_utime(F, BUF)	debug_fd_utime(F, BUF)
#define fd_truncate(F,LEN)	debug_fd_truncate(F,LEN)
#define fd_link(OLD,NEW)	debug_fd_link(OLD,NEW)
#define fd_symlink(F,PATH)	debug_fd_symlink(F,PATH)
#define fd_readlink(F,B,SZ)	debug_fd_readlink(F,B,SZ)
#define fd_rmdir(DIR)	debug_fd_rmdir(DIR)
#define fd_unlink(FILE)	debug_fd_unlink(FILE)
#define fd_mkdir(DIR,MODE)	debug_fd_mkdir(DIR,MODE)
#define fd_rename(O,N)	debug_fd_rename(O,N)
#define fd_chdir(DIR)	debug_fd_chdir(DIR)
#define fd_get_current_dir_name()	debug_fd_get_current_dir_name()
#define fd_normalize_path(PATH)	debug_fd_normalize_path(PATH)
#define fd_open(X,Y,Z) dmalloc_register_fd(debug_fd_open((X),(Y)|fd_BINARY,(Z)))
#define fd_socket(X,Y,Z) dmalloc_register_fd(debug_fd_socket((X),(Y),(Z)))
#define fd_pipe(X) debug_fd_pipe( (X) DMALLOC_POS )
#define fd_accept(X,Y,Z) dmalloc_register_fd(debug_fd_accept((X),(Y),(Z)))
#define fd_accept4(X,Y,Z,F) dmalloc_register_fd(accept4((X),(Y),(Z),(F)))

#define fd_bind(fd,X,Y) debug_fd_bind(dmalloc_touch_fd(fd), (X), (Y))
#define fd_getsockopt(fd,X,Y,Z,Q) debug_fd_getsockopt(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q))
#define fd_setsockopt(fd,X,Y,Z,Q) debug_fd_setsockopt(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q))
#define fd_recv(fd,X,Y,Z) debug_fd_recv(dmalloc_touch_fd(fd), (X), (Y),(Z))
#define fd_getsockname(fd,X,Y) debug_fd_getsockname(dmalloc_touch_fd(fd), (X), (Y))
#define fd_getpeername(fd,X,Y) debug_fd_getpeername(dmalloc_touch_fd(fd), (X), (Y))
#define fd_recvfrom(fd,X,Y,Z,Q,P) debug_fd_recvfrom(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q),(P))
#define fd_send(fd,X,Y,Z) debug_fd_send(dmalloc_touch_fd(fd), (X), (Y),(Z))
#define fd_sendto(fd,X,Y,Z,Q,P) debug_fd_sendto(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q),(P))
#define fd_shutdown(fd,X) debug_fd_shutdown(dmalloc_touch_fd(fd), (X))
#define fd_listen(fd,X) debug_fd_listen(dmalloc_touch_fd(fd), (X))
#define fd_close(fd) debug_fd_close(dmalloc_close_fd(fd))
#define fd_write(fd,X,Y) debug_fd_write(dmalloc_touch_fd(fd),(X),(Y))
#define fd_writev(fd,X,Y) debug_fd_writev(dmalloc_touch_fd(fd),(X),(Y))
#define fd_read(fd,X,Y) debug_fd_read(dmalloc_touch_fd(fd),(X),(Y))
#define fd_lseek(fd,X,Y) debug_fd_lseek(dmalloc_touch_fd(fd),(X),(Y))
#define fd_ftruncate(fd,X) debug_fd_ftruncate(dmalloc_touch_fd(fd),(X))
#define fd_flock(fd,X) debug_fd_flock(dmalloc_touch_fd(fd),(X))
#define fd_fstat(fd,X) debug_fd_fstat(dmalloc_touch_fd(fd),(X))
#define fd_select debug_fd_select /* fixme */
#define fd_ioctl(fd,X,Y) debug_fd_ioctl(dmalloc_touch_fd(fd),(X),(Y))
#define fd_dup(fd) dmalloc_register_fd(debug_fd_dup(dmalloc_touch_fd(fd)))
#define fd_dup2(fd,to) dmalloc_register_fd(debug_fd_dup2(dmalloc_touch_fd(fd),dmalloc_close_fd(to)))
#define fd_connect(fd,X,Z) debug_fd_connect(dmalloc_touch_fd(fd),(X),(Z))
#define fd_inet_ntop(af,addr,cp,sz) debug_fd_inet_ntop(af,addr,cp,sz)
#define fd_openpty(m,s,name,term,win) debug_fd_openpty(m,s,name,term,win)


/* Prototypes begin here */
PMOD_EXPORT void set_errno_from_win32_error (unsigned long err);
void free_pty(struct my_pty *pty);
int fd_to_handle(int fd, int *type, HANDLE *handle, int exclusive);
void release_fd(int fd);
PMOD_EXPORT char *debug_fd_info(int fd);
PMOD_EXPORT int debug_fd_isatty(int fd);
PMOD_EXPORT int debug_fd_query_properties(int fd, int guess);
void fd_init(void);
void fd_exit(void);
p_wchar1 *low_dwim_utf8_to_utf16(const p_wchar0 *str, size_t len);
PMOD_EXPORT p_wchar1 *pike_dwim_utf8_to_utf16(const p_wchar0 *str);
PMOD_EXPORT p_wchar0 *pike_utf16_to_utf8(const p_wchar1 *str);
PMOD_EXPORT int debug_fd_stat(const char *file, PIKE_STAT_T *buf);
PMOD_EXPORT int debug_fd_utime(const char *file, struct fd_utimbuf *buf);
PMOD_EXPORT int debug_fd_truncate(const char *file, INT64 len);
PMOD_EXPORT int debug_fd_link(const char *oldpath, const char *newpath);
PMOD_EXPORT int debug_fd_symlink(const char *target, const char *linkpath);
PMOD_EXPORT ptrdiff_t debug_fd_readlink(const char *file,
                                        char *buf, size_t bufsiz);
PMOD_EXPORT int debug_fd_rmdir(const char *dir);
PMOD_EXPORT int debug_fd_unlink(const char *file);
PMOD_EXPORT int debug_fd_mkdir(const char *dir, int mode);
PMOD_EXPORT int debug_fd_rename(const char *old, const char *new);
PMOD_EXPORT int debug_fd_chdir(const char *dir);
PMOD_EXPORT char *debug_fd_get_current_dir_name(void);
PMOD_EXPORT char *debug_fd_normalize_path(const char *path);
PMOD_EXPORT FD debug_fd_open(const char *file, int open_mode, int create_mode);
PMOD_EXPORT FD debug_fd_socket(int domain, int type, int proto);
PMOD_EXPORT int debug_fd_pipe(int fds[2] DMALLOC_LINE_ARGS);
PMOD_EXPORT FD debug_fd_accept(FD fd, struct sockaddr *addr, ACCEPT_SIZE_T *addrlen);
SOCKFUN2(bind, struct sockaddr *, int)
PMOD_EXPORT int debug_fd_connect (FD fd, struct sockaddr *a, int len);
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
PMOD_EXPORT int debug_fd_close(FD fd);
PMOD_EXPORT ptrdiff_t debug_fd_write(FD fd, void *buf, ptrdiff_t len);
PMOD_EXPORT ptrdiff_t debug_fd_writev(FD fd, struct iovec *iov, ptrdiff_t n);
PMOD_EXPORT ptrdiff_t debug_fd_read(FD fd, void *to, size_t len);
PMOD_EXPORT PIKE_OFF_T debug_fd_lseek(FD fd, PIKE_OFF_T pos, int where);
PMOD_EXPORT int debug_fd_ftruncate(FD fd, PIKE_OFF_T len);
PMOD_EXPORT int debug_fd_flock(FD fd, int oper);
PMOD_EXPORT int debug_fd_fstat(FD fd, PIKE_STAT_T *s);
PMOD_EXPORT int debug_fd_select(int fds, FD_SET *a, FD_SET *b, FD_SET *c, struct timeval *t);
PMOD_EXPORT int debug_fd_ioctl(FD fd, int cmd, void *data);
PMOD_EXPORT FD debug_fd_dup(FD from);
PMOD_EXPORT FD debug_fd_dup2(FD from, FD to);
PMOD_EXPORT const char *debug_fd_inet_ntop(int af, const void *addr,
					   char *cp, size_t sz);
PMOD_EXPORT int debug_fd_openpty(int *master, int *slave,
				 char *ignored_name,
				 void *ignored_term,
				 struct winsize *winp);
/* Prototypes end here */

#undef SOCKFUN1
#undef SOCKFUN2
#undef SOCKFUN3
#undef SOCKFUN4
#undef SOCKFUN5

#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#endif

#ifndef ENOTSUPP
#define ENOTSUPP WSAEOPNOTSUPP
#endif

#define fd_RDONLY 1
#define fd_WRONLY 2
#define fd_RDWR 3
#define fd_ACCMODE 3
#define fd_APPEND 4
#define fd_CREAT 8
#define fd_TRUNC 16
#define fd_EXCL 32
#define fd_BINARY 0
#define fd_LARGEFILE 0

#define fd_shutdown_read SD_RECEIVE
#define fd_shutdown_write SD_SEND
#define fd_shutdown_both SD_BOTH

#define FD_PTY -6
#define FD_PIPE -5
#define FD_SOCKET -4
#define FD_CONSOLE -3
#define FD_FILE -2
#define FD_NO_MORE_FREE -1


#define fd_LOCK_SH 1
#define fd_LOCK_EX 2
#define fd_LOCK_UN 4
#define fd_LOCK_NB 8

struct my_pty
{
  INT32 refs;		/* Total number of references. */
  INT32 fd_refs;	/* Number of references from da_handle[]. */
  HPCON conpty;		/* Only valid for master side. */
  struct my_pty *other;	/* Other end (if any), NULL otherwise. */
  struct pid_status *clients; /* List of client processes. */
  HANDLE read_pipe;	/* Pipe that supports read(). */
  HANDLE write_pipe;	/* Pipe that supports write(). */
  COORD winsize;	/* There's no API to fetch the window size. */
};

#ifdef PIKE_DEBUG
#define fd_check_fd(X) do { if(fd_type[X]>=0) Pike_fatal("FD_SET on closed fd %d (%d) %s:%d.\n",X,da_handle[X],__FILE__,__LINE__); }while(0)
#else
#define fd_check_fd(X)
#endif /* PIKE_DEBUG */

extern HANDLE da_handle[FD_SETSIZE];
extern int fd_type[FD_SETSIZE];

#define fd_FD_CLR(X,Y) FD_CLR((SOCKET)da_handle[X],Y)
#define fd_FD_SET(X,Y) \
 do { fd_check_fd(X); FD_SET((SOCKET)da_handle[X],Y); }while(0)
#define fd_FD_ISSET(X,Y) FD_ISSET((SOCKET)da_handle[X],Y)
#define fd_FD_ZERO(X) FD_ZERO(X)

#ifndef S_IFSOCK
#define S_IFSOCK 0140000
#endif

#ifndef S_IFIFO
#define S_IFIFO 0010000
#endif

/* Make dynamically loaded functions available to the rest of pike. */
#undef NTLIB
#define NTLIB(LIB)

#undef NTLIBFUNC
#define NTLIBFUNC(LIB, RET, NAME, ARGLIST)				\
  typedef RET (WINAPI * PIKE_CONCAT3(Pike_NT_, NAME, _type)) ARGLIST;	\
  extern PIKE_CONCAT3(Pike_NT_, NAME, _type) PIKE_CONCAT(Pike_NT_, NAME)

#include "ntlibfuncs.h"


/* This may be inaccurate! /Hubbe */
#if defined(__NT__) && !defined(__MINGW32__)
#define EMULATE_DIRECT
#endif

#ifdef EMULATE_DIRECT

struct direct {
  char *d_name;
};
#define dirent direct
#define MAXPATHLEN MAX_PATH
#define NAMLEN(dirent) strlen((dirent)->d_name)

typedef struct DIR_s
{
  int first;
  WIN32_FIND_DATAW find_data;
  HANDLE h;
  struct direct direct;
} DIR;

PMOD_EXPORT DIR *opendir(char *dir);
PMOD_EXPORT int readdir_r(DIR *dir, struct direct *tmp ,struct direct **d);
PMOD_EXPORT void closedir(DIR *dir);

#define HAVE_POSIX_READDIR_R

/* Do not use these... */
#if 0
/* Why not? Want to use this one for _getdrive. /mast */
#undef HAVE_DIRECT_H
#endif /*0 */

#undef HAVE_NDIR_H
#undef HAVE_SYS_NDIR_H
#undef HAVE_DIRENT_H

#endif /* EMULATE_DIRECT */



#else /* HAVE_WINSOCK_H */


typedef int FD;
typedef struct stat PIKE_STAT_T;
typedef off_t PIKE_OFF_T;
#define PRINTPIKEOFFT PRINTOFFT

#define fd_info(X) ""
#define fd_init()
#define fd_exit()

#define fd_RDONLY O_RDONLY
#define fd_WRONLY O_WRONLY
#define fd_RDWR O_RDWR
#ifdef O_ACCMODE
#define fd_ACCMODE O_ACCMODE
#else
#define fd_ACCMODE (fd_RDONLY|fd_WRONLY|fd_RDWR)
#endif /* O_ACCMODE */
#define fd_APPEND O_APPEND
#define fd_CREAT O_CREAT
#define fd_TRUNC O_TRUNC
#define fd_EXCL O_EXCL

#ifdef O_BINARY
#define fd_BINARY O_BINARY
#else
#define fd_BINARY 0
#endif /* O_BINARY */

#ifdef O_LARGEFILE
#define fd_LARGEFILE O_LARGEFILE
#else /* !O_LARGEFILE */
#define fd_LARGEFILE 0
#endif /* O_LARGEFILE */

#define fd_utimbuf	utimbuf

#define fd_isatty(F) isatty(F)
#define fd_query_properties(X,Y) ( fd_INTERPROCESSABLE | (Y))

#define fd_stat(F,BUF) stat(F,BUF)
#define fd_lstat(F,BUF) lstat(F,BUF)
#define fd_utime(F, BUF)	utime(F, BUF)
#ifdef HAVE_TRUNCATE64
#define fd_truncate(F,LEN)	truncate64(F,LEN)
#else
#define fd_truncate(F,LEN)	truncate(F,LEN)
#endif
#define fd_link(OLD,NEW)	link(OLD,NEW)
#define fd_symlink(F,PATH)	symlink(F,PATH)
#define fd_readlink(F,B,SZ)	readlink(F,B,SZ)
#define fd_rmdir(DIR)	rmdir(DIR)
#define fd_unlink(FILE)	unlink(FILE)
#if MKDIR_ARGS == 2
#define fd_mkdir(DIR,MODE)	mkdir(DIR,MODE)
#else
/* Most OS's should have MKDIR_ARGS == 2 nowadays fortunately. */
static int PIKE_UNUSED_ATTRIBUTE debug_fd_mkdir(const char *dir, int mode)
{
  /* NB: Attempt to set the mode via the umask. */
  int mask = umask(~mode & 0777);
  int ret;

  umask(~mode & mask & 0777);
  ret = mkdir(dir);
  umask(mask);

  /* NB: We assume that the umask trick worked. */
  return ret;
}
#define fd_mkdir(DIR,MODE)	debug_fd_mkdir(DIR,MODE)
#endif
#define fd_rename(O,N)	rename(O,N)
#define fd_chdir(DIR)	chdir(DIR)
#if defined(HAVE_GET_CURRENT_DIR_NAME) && !defined(USE_DL_MALLOC)
/* Glibc extension... */
#define fd_get_current_dir_name()	get_current_dir_name()
#elif defined(HAVE_WORKING_GETCWD) && !defined(USE_DL_MALLOC)
#if HAVE_WORKING_GETCWD
/* Glibc and win32 (HAVE_WORKING_GETCWD == 1).
 *
 * getcwd(NULL, 0) gives malloced buffer.
 */
#define fd_get_current_dir_name()	getcwd(NULL, 0)
#else
/* Solaris-style (HAVE_WORKING_GETCWD == 0).
 *
 * getcwd(NULL, x) gives malloced buffer as long as x > 0, and the path fits.
 */
static char PIKE_UNUSED_ATTRIBUTE *debug_get_current_dir_name(void)
{
  char *buf;
  size_t buf_size = 128;
  do {
    buf = getcwd(NULL, buf_size);
    if (buf) return buf;
    buf_size <<= 1;
  } while (errno == ERANGE);
  return NULL;
}
#define fd_get_current_dir_name()	debug_get_current_dir_name()
#endif
#else
static char PIKE_UNUSED_ATTRIBUTE *debug_get_current_dir_name(void)
{
  char *buf;
  size_t buf_size = 128;
  do {
    buf = malloc(buf_size);

    if (!buf) {
      errno = ENOMEM;
      return NULL;
    }

    if (!getcwd(buf, buf_size-1)) {
      free(buf);

      if (errno == ERANGE) {
	buf_size <<= 1;
	continue;
      }

      return NULL;
    }
    return buf;
  } while (1);
}
#define fd_get_current_dir_name()	debug_get_current_dir_name()
#endif
#define fd_open(X,Y,Z) dmalloc_register_fd(open((X),(Y)|fd_BINARY,(Z)))
#define fd_socket(X,Y,Z) dmalloc_register_fd(socket((X),(Y),(Z)))
#define fd_pipe pipe /* FIXME */
#define fd_accept(X,Y,Z) dmalloc_register_fd(accept((X),(Y),(Z)))
#define fd_accept4(X,Y,Z,F) dmalloc_register_fd(accept4((X),(Y),(Z),(F)))

#define fd_bind(fd,X,Y) bind(dmalloc_touch_fd(fd), (X), (Y))
#define fd_getsockopt(fd,X,Y,Z,Q) getsockopt(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q))
#define fd_setsockopt(fd,X,Y,Z,Q) setsockopt(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q))
#define fd_recv(fd,X,Y,Z) recv(dmalloc_touch_fd(fd), (X), (Y),(Z))
#define fd_getsockname(fd,X,Y) getsockname(dmalloc_touch_fd(fd), (X), (Y))
#define fd_getpeername(fd,X,Y) getpeername(dmalloc_touch_fd(fd), (X), (Y))
#define fd_recvfrom(fd,X,Y,Z,Q,P) recvfrom(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q),(P))
#define fd_send(fd,X,Y,Z) send(dmalloc_touch_fd(fd), (X), (Y),(Z))
#define fd_sendto(fd,X,Y,Z,Q,P) sendto(dmalloc_touch_fd(fd), (X),(Y),(Z),(Q),(P))
#define fd_shutdown(fd,X) shutdown(dmalloc_touch_fd(fd), (X))
#define fd_listen(fd,X) listen(dmalloc_touch_fd(fd), (X))

#ifdef HAVE_BROKEN_F_SETFD
#define fd_close(fd) (set_close_on_exec(fd,0),close(dmalloc_close_fd(fd)))
#else /* !HAVE_BROKEN_F_SETFD */
#define fd_close(fd) close(dmalloc_close_fd(fd))
#endif /* HAVE_BROKEN_F_SETFD */

#define fd_write(fd,X,Y) write(dmalloc_touch_fd(fd),(X),(Y))
#define fd_writev(fd,X,Y) writev(dmalloc_touch_fd(fd),(X),(Y))
#define fd_read(fd,X,Y) read(dmalloc_touch_fd(fd),(X),(Y))
#define fd_lseek(fd,X,Y) lseek(dmalloc_touch_fd(fd),(X),(Y))
#ifdef HAVE_FTRUNCATE64
#define fd_ftruncate(fd,X) ftruncate64(dmalloc_touch_fd(fd),(X))
#else
#define fd_ftruncate(fd,X) ftruncate(dmalloc_touch_fd(fd),(X))
#endif
#define fd_fstat(fd,X) fstat(dmalloc_touch_fd(fd),(X))
#define fd_select select /* fixme */
#define fd_ioctl(fd,X,Y) ioctl(dmalloc_touch_fd(fd),(X),(Y))
#define fd_dup(fd) dmalloc_register_fd(dup(dmalloc_touch_fd(fd)))
#define fd_connect(fd,X,Z) connect(dmalloc_touch_fd(fd),(X),(Z))
#ifdef HAVE_INET_NTOP
#define fd_inet_ntop(af,addr,cp,sz) inet_ntop(af,addr,cp,sz)
#endif /* HAVE_INET_NTOP */

#ifdef HAVE_BROKEN_F_SETFD
#define fd_dup2(fd,to) (set_close_on_exec(to,0), dmalloc_register_fd(dup2(dmalloc_touch_fd(fd),dmalloc_close_fd(to))))
#else /* !HAVE_BROKEN_F_SETFD */
#define fd_dup2(fd,to) dmalloc_register_fd(dup2(dmalloc_touch_fd(fd),dmalloc_close_fd(to)))
#endif /* HAVE_BROKEN_F_SETFD */

#define fd_socketpair socketpair /* fixme */

#define fd_fd_set fd_set
#define fd_FD_CLR FD_CLR
#define fd_FD_SET FD_SET
#define fd_FD_ISSET FD_ISSET
#define fd_FD_ZERO FD_ZERO

#ifdef HAVE_FLOCK
#define HAVE_FD_FLOCK
#define fd_flock(fd,X) flock(dmalloc_touch_fd(fd),(X))
#define fd_LOCK_SH LOCK_SH
#define fd_LOCK_EX LOCK_EX
#define fd_LOCK_UN LOCK_UN
#define fd_LOCK_NB LOCK_NB
#else
#ifdef HAVE_LOCKF
#define HAVE_FD_LOCKF
#define fd_LOCK_EX F_LOCK
#define fd_LOCK_UN F_ULOCK
#define fd_LOCK_NB F_TLOCK

#define fd_lockf(fd,mode) lockf(dmalloc_touch_fd(fd),mode,0)
#endif /* HAVE_LOCKF */
#endif /* HAVE_FLOCK */


#define fd_shutdown_read 0
#define fd_shutdown_write 1
#define fd_shutdown_both 2

#define FILE_CAPABILITIES (fd_INTERPROCESSABLE | fd_CAN_NONBLOCK)
#ifndef __amigaos__
#define PIPE_CAPABILITIES (fd_INTERPROCESSABLE | fd_BUFFERED | fd_CAN_NONBLOCK)
#endif
#define UNIX_SOCKET_CAPABILITIES (fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK | fd_SEND_FD)
#define SOCKET_CAPABILITIES (fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN)
#define TTY_CAPABILITIES (fd_TTY | fd_INTERPROCESSABLE | fd_BIDIRECTIONAL | fd_CAN_NONBLOCK)

#ifdef HAVE_OPENPTY
#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#define fd_openpty	openpty	/* FIXME */
#endif

#endif /* Don't HAVE_WINSOCK_H */

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#ifndef S_ISREG
#ifdef S_IFREG
#define S_ISREG(mode)   (((mode) & (S_IFMT)) == (S_IFREG))
#else /* !S_IFREG */
#define S_ISREG(mode)   (((mode) & (_S_IFMT)) == (_S_IFREG))
#endif /* S_IFREG */
#endif /* !S_ISREG */

#ifndef S_IFIFO
#define S_IFIFO  0x1000
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0xc000
#endif

PMOD_EXPORT int pike_make_pipe(int *fds);
PMOD_EXPORT int fd_from_object(struct object *o);
PMOD_EXPORT void create_proxy_pipe(struct object *o, int for_reading);
PMOD_EXPORT struct object *file_make_object_from_fd(int fd, int mode, int guess);
PMOD_EXPORT extern struct program *port_program;

#endif /* FDLIB_H */
