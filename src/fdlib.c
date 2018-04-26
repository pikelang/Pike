/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * This file contains wrappers for NT system calls that
 * implement the corresponding POSIX system calls.
 * One major difference compared to the native wrappers
 * in crt.lib is that filenames are assumed to be UTF-8-
 * encoded, with a fallback to Latin-1.
 *
 * The UTF-8 filenames are recoded to UTF16 and used
 * with the wide versions of the NT system calls.
 */

#include "global.h"
#include "fdlib.h"
#include "pike_error.h"
#include <math.h>
#include <ctype.h>
#include <assert.h>

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

#if defined(HAVE_WINSOCK_H)

#include <time.h>

/* Old versions of the headerfiles don't have this constant... */
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

#ifndef ENOTSOCK
#define ENOTSOCK	WSAENOTSOCK
#endif

#include "threads.h"

/* Mutex protecting da_handle, fd_type, fd_busy and first_free_handle. */
static MUTEX_T fd_mutex;

/* Condition indicating that some field of fd_busy has been cleared. */
static COND_T fd_cond;

/* HANDLEs corresponding to the fd of the same index. */
HANDLE da_handle[FD_SETSIZE];

/* Next free fd when >= 0.
 *
 * When < 0:
 *   FD_NO_MORE_FREE (-1)
 *     End marker for the free list.
 *
 *   FD_FILE (-2)
 *     Handle from CreateFileW().
 *
 *   FD_CONSOLE (-3)
 *     Handle from GetStdHandle().
 *
 *   FD_SOCKET (-4)
 *     socket_fd from socket().
 *
 *   FD_PIPE (-5)
 *     Handle from CreatePipe().
 */
int fd_type[FD_SETSIZE];

/* Indication whether the corresponding fd is in use. */
int fd_busy[FD_SETSIZE];

/* Next free fd. FD_NO_MORE_FREE (-1) if all fds allocated. */
int first_free_handle;

/* #define FD_DEBUG */
/* #define FD_STAT_DEBUG */

#ifdef FD_DEBUG
#define FDDEBUG(X) X
#else
#define FDDEBUG(X)
#endif

#ifdef FD_STAT_DEBUG
#define STATDEBUG(X) X
#else
#define STATDEBUG(X) do {} while (0)
#endif


#ifdef USE_DL_MALLOC
/* NB: We use some calls that allocate memory with the libc malloc(). */
static inline void libc_free(void *ptr);
#else
#define libc_free(PTR)	free(PTR)
#endif

/* _dosmaperr is internal but still exported in the dll interface. */
__declspec(dllimport) void __cdecl _dosmaperr(unsigned long);

PMOD_EXPORT void set_errno_from_win32_error (unsigned long err)
{
  /* _dosmaperr handles the common I/O errors from GetLastError, but
   * not the winsock codes. */
  _dosmaperr (err);

  /* Let through any error that _dosmaperr didn't map, and which
   * doesn't conflict with the errno range in msvcrt. */
  if (errno == EINVAL && err > STRUNCATE /* 80 */) {
    switch (err) {
      /* Special cases for the error codes above STRUNCATE that
       * _dosmaperr actively map to EINVAL. */
      case ERROR_INVALID_PARAMETER: /* 87 */
      case ERROR_INVALID_HANDLE: /* 124 */
      case ERROR_NEGATIVE_SEEK:	/* 131 */
	return;
      case ERROR_DIRECTORY: /* 267 */
	errno = ENOTDIR; /* [Bug 7271] */
	return;
    }

    /* FIXME: This lets most winsock codes through as-is, e.g.
     * WSAEWOULDBLOCK (10035) instead of EAGAIN. There are symbolic
     * constants for all of them in the System module, but the
     * duplicate values still complicates code on the Windows
     * platform, so they ought to be mapped to the basic codes where
     * possible, I think. That wouldn't be compatible, though.
     * /mast */

    errno = err;
  }
}

/* Dynamic load of functions that don't exist in all Windows versions. */

#undef NTLIB
#define NTLIB(LIB)					\
  static HINSTANCE PIKE_CONCAT3(Pike_NT_, LIB, _lib);

#undef NTLIBFUNC
#define NTLIBFUNC(LIB, RET, NAME, ARGLIST)				\
  typedef RET (WINAPI * PIKE_CONCAT3(Pike_NT_, NAME, _type)) ARGLIST;	\
  static PIKE_CONCAT3(Pike_NT_, NAME, _type) PIKE_CONCAT(Pike_NT_, NAME)

#include "ntlibfuncs.h"

#define ISSEPARATOR(a) ((a) == '\\' || (a) == '/')

#ifdef PIKE_DEBUG
static int IsValidHandle(HANDLE h)
{
#ifndef __GNUC__
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
#endif /* !__GNUC__ */

  return 1;
}

PMOD_EXPORT HANDLE CheckValidHandle(HANDLE h)
{
  if(!IsValidHandle(h))
    Pike_fatal("Invalid handle!\n");
  return h;
}
#endif

static int fd_to_handle(int fd, int *type, HANDLE *handle)
{
  int ret = -1;

  if (fd >= FD_NO_MORE_FREE) {
    mt_lock(&fd_mutex);
    while (fd_busy[fd]) {
      co_wait(&fd_cond, &fd_mutex);
    }
    if (fd_type[fd] < 0) {
      if (type) *type = fd_type[fd];
      if (handle) *handle = da_handle[fd];
      fd_busy[fd] = 1;
      ret = 0;
    } else {
      errno = EBADF;
    }
    mt_unlock(&fd_mutex);
  } else {
    errno = EBADF;
  }

  return ret;
}

static int fd_to_socket(int fd, SOCKET *socket)
{
  int ret = -1;

  if (fd >= FD_NO_MORE_FREE) {
    mt_lock(&fd_mutex);
    while (fd_busy[fd]) {
      co_wait(&fd_cond, &fd_mutex);
    }
    if (fd_type[fd] == FD_SOCKET) {
      if (socket) *socket = (SOCKET)da_handle[fd];
      fd_busy[fd] = 1;
      ret = 0;
    } else if (fd_type[fd] < 0) {
      errno = ENOTSOCK;
    } else {
      errno = EBADF;
    }
    mt_unlock(&fd_mutex);
  } else {
    errno = EBADF;
  }

  return ret;
}

static int allocate_fd(int type, HANDLE handle)
{
  int fd;

  if (type >= FD_NO_MORE_FREE) {
    errno = EINVAL;
    return -1;
  }

  mt_lock(&fd_mutex);

  fd = first_free_handle;

  if (fd >= 0) {
    assert(!fd_busy[fd]);
    assert(fd_type[fd] >= -1);

    first_free_handle = fd_type[fd];
    fd_type[fd] = type;
    da_handle[fd] = handle;
    fd_busy[fd] = 1;
  } else {
    errno = EMFILE;
  }

  mt_unlock(&fd_mutex);

  return fd;
}

static int reallocate_fd(int fd, int type, HANDLE handle)
{
  int prev_fd;
  if ((fd < 0) || (fd >= FD_SETSIZE) || (type >= FD_NO_MORE_FREE)) {
    errno = EINVAL;
    return -1;
  }

  mt_lock(&fd_mutex);

  while (fd_busy[fd]) {
    co_wait(&fd_cond, &fd_mutex);
  }

  if (fd_type[fd] < FD_NO_MORE_FREE) {
    goto reallocate;
  }

  prev_fd = first_free_handle;

  if (prev_fd == fd) {
    assert(!fd_busy[fd]);
    assert(fd_type[fd] >= -1);
    first_free_handle = fd_type[fd];
    goto found;
  }

  while (prev_fd != FD_NO_MORE_FREE) {
    if (fd_type[prev_fd] == fd) {
      /* Found. */
      assert(!fd_busy[fd]);
      assert(fd_type[fd] >= -1);
      fd_type[prev_fd] = fd_type[fd];
      goto found;
    }
    prev_fd = fd_type[prev_fd];
  }

  errno = EMFILE;
  mt_unlock(&fd_mutex);
  return -1;

 reallocate:
  if (fd_type[fd] == FD_SOCKET) {
    closesocket((SOCKET)da_handle[fd]);
  } else {
    CloseHandle(da_handle[fd]);
  }

  /* FALLTHRU */
 found:
  fd_type[fd] = type;
  da_handle[fd] = handle;
  fd_busy[fd] = 1;
  mt_unlock(&fd_mutex);
  return fd;
}

static void release_fd(int fd)
{
  if ((fd < 0) || (fd >= FD_SETSIZE)) {
    return;
  }

  mt_lock(&fd_mutex);

  assert(fd_busy[fd]);

  fd_busy[fd] = 0;
  co_broadcast(&fd_cond);
  mt_unlock(&fd_mutex);
}

static void free_fd(int fd)
{
  if ((fd < 0) || (fd >= FD_SETSIZE)) {
    return;
  }

  mt_lock(&fd_mutex);

  if (fd_type[fd] < FD_NO_MORE_FREE) {
    assert(fd_busy[fd]);

    fd_type[fd] = first_free_handle;
    first_free_handle = fd;
    fd_busy[fd] = 0;
    co_broadcast(&fd_cond);
  }
  mt_unlock(&fd_mutex);
}

static void set_fd_handle(int fd, HANDLE handle)
{
  if (fd < 0) {
    return;
  }

  mt_lock(&fd_mutex);
  assert(fd_busy[fd]);

  da_handle[fd] = handle;
  mt_unlock(&fd_mutex);
}

PMOD_EXPORT char *debug_fd_info(int fd)
{
  if(fd<0)
    return "BAD";

  if(fd > FD_SETSIZE)
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
  int type;

  if (fd_to_handle(fd, &type, NULL) < 0) return 0;
  release_fd(fd);

  switch(type)
  {
    case FD_SOCKET:
      return fd_BUFFERED | fd_CAN_NONBLOCK | fd_CAN_SHUTDOWN;

    case FD_FILE:
      return fd_INTERPROCESSABLE;

    case FD_CONSOLE:
      return fd_CAN_NONBLOCK | fd_INTERPROCESSABLE;

    case FD_PIPE:
      return fd_INTERPROCESSABLE | fd_BUFFERED;
    default: return 0;
  }
}

void fd_init(void)
{
  int e;
  WSADATA wsadata;
  OSVERSIONINFO osversion;
  
  mt_init(&fd_mutex);
  co_init(&fd_cond);
  mt_lock(&fd_mutex);

  if(WSAStartup(MAKEWORD(1,1), &wsadata) != 0)
  {
    Pike_fatal("No winsock available.\n");
  }
  FDDEBUG(fprintf(stderr,"Using %s\n",wsadata.szDescription));

  memset(fd_busy, 0, sizeof(fd_busy));

  fd_type[0] = FD_CONSOLE;
  da_handle[0] = GetStdHandle(STD_INPUT_HANDLE);
  fd_type[1] = FD_CONSOLE;
  da_handle[1] = GetStdHandle(STD_OUTPUT_HANDLE);
  fd_type[2] = FD_CONSOLE;
  da_handle[2] = GetStdHandle(STD_ERROR_HANDLE);

  first_free_handle=3;
  for(e=3;e<FD_SETSIZE-1;e++)
    fd_type[e]=e+1;
  fd_type[e]=FD_NO_MORE_FREE;
  mt_unlock(&fd_mutex);

  /* MoveFileEx doesn't exist in W98 and earlier. */
  /* Correction, it exists but does not work -Hubbe */
  osversion.dwOSVersionInfoSize = sizeof(osversion);
  if (GetVersionEx(&osversion) &&
      (osversion.dwPlatformId != VER_PLATFORM_WIN32s) &&	/* !win32s */
      (osversion.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS))	/* !win9x */
  {
#undef NTLIB
#define NTLIB(LIBNAME)						\
    PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib) =			\
      LoadLibrary(TOSTR(LIBNAME))

#undef NTLIBFUNC
#define NTLIBFUNC(LIBNAME, RETTYPE, SYMBOL, ARGLIST) do {	\
      if (PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib)) {		\
	PIKE_CONCAT(Pike_NT_, SYMBOL) =				\
	  (PIKE_CONCAT3(Pike_NT_, SYMBOL, _type))		\
	  GetProcAddress(PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib),	\
			 TOSTR(SYMBOL));			\
      }								\
    } while(0)
#include "ntlibfuncs.h"
  }
}

void fd_exit(void)
{
#undef NTLIB
#define NTLIB(LIBNAME) do {					\
    if (PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib)) {		\
      if (FreeLibrary(PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib))) {	\
	PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib) = NULL;		\
      }								\
    }								\
  } while(0)
#undef NTLIBFUNC
#define NTLIBFUNC(LIBNAME, RETTYPE, SYMBOL, ARGLIST) do {	\
    if (!PIKE_CONCAT3(Pike_NT_, LIBNAME, _lib)) {		\
      PIKE_CONCAT(Pike_NT_, SYMBOL) = NULL;			\
    }								\
  } while(0)
#include "ntlibfuncs.h"

  WSACleanup();
  mt_destroy(&fd_mutex);
}

static INLINE time_t convert_filetime_to_time_t(FILETIME *tmp)
{
  /* FILETIME is in 100ns since Jan 01, 1601 00:00 UTC.
   *
   * Offset to Jan 01, 1970 is thus 0x019db1ded53e8000 * 100ns.
   */
#ifdef INT64
  return (((INT64) tmp->dwHighDateTime << 32)
	  + tmp->dwLowDateTime
	  - 0x019db1ded53e8000) / 10000000;
#else
  double t;
  if (tmp->dwLowDateTime < 0xd53e8000UL) {
    tmp->dwHighDateTime -= 0x019db1dfUL;	/* Note: Carry! */
    tmp->dwLowDateTime += 0x2ac18000UL;	/* Note: 2-compl */
  } else {
    tmp->dwHighDateTime -= 0x019db1deUL;
    tmp->dwLowDateTime -= 0xd53e8000UL;
  }
  t=tmp->dwHighDateTime * pow(2.0,32.0) + (double)tmp->dwLowDateTime;

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
 *   correctly accounts for DST at that time(*).
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
 *
 * Note also that the UTC timestamps are cached by the OS. In the case
 * of FAT volumes that means that the current DST setting might be
 * different when FileTimeToLocalFileTime runs than it was when the
 * FILETIME was calculated, so the calculation errors might not cancel
 * each other out shortly after DST changes. A reboot is necessary
 * after a DST change to ensure that no cached timestamps remain.
 * There is nothing we can do to correct this error. :(
 *
 * See also http://msdn.microsoft.com/library/en-us/sysinfo/base/file_times.asp
 *
 * *) Actually it doesn't in all cases: The overlapping hour(s) when
 * going from DST to normal time are ambiguous and since there's no
 * DST flag it has to make an arbitrary choice.
 */

static time_t local_time_to_utc (time_t t)
/* Converts a time_t containing local time to a real time_t (i.e. UTC).
 *
 * Note that there's an ambiguity in the autumn transition from DST to
 * normal time: The returned timestamp can be one hour off in the hour
 * where the clock is "turned back". This function always consider
 * that hour to be in normal time.
 *
 * Might return -1 if the time is outside the valid range.
 */
{
  int tz_secs, dl_secs;

  /* First adjust for the time zone. */
#ifdef HAVE__GET_TIMEZONE
  _get_timezone (&tz_secs);
#else
  tz_secs = _timezone;
#endif
  t += tz_secs;

  /* Then DST. */
#ifdef HAVE__GET_DAYLIGHT
  _get_daylight (&dl_secs);
#else
  dl_secs = _daylight;
#endif
  if (dl_secs) {
    /* See if localtime thinks this is in DST. */
    int isdst;
#ifdef HAVE_LOCALTIME_S
    struct tm ts;
    if (localtime_s (&ts, &t)) return -1;
    isdst = ts.tm_isdst;
#else
    struct tm *ts;
    /* FIXME: A mutex is necessary to avoid a race here, but we
     * can't protect all calls to localtime anyway. */
    if (!(ts = localtime (&t))) return -1;
    isdst = ts->tm_isdst;
#endif
    if (isdst) t += dl_secs;
  }

  return t;
}

static time_t fat_filetime_to_time_t (FILETIME *in)
{
  FILETIME local_ft;
  if (!FileTimeToLocalFileTime (in, &local_ft)) {
    set_errno_from_win32_error (GetLastError());
    return -1;
  }
  else {
    time_t t = convert_filetime_to_time_t (&local_ft);
    /* Now we have a strange creature, namely a time_t that contains
     * local time. */
    return local_time_to_utc (t);
  }
}

static int fat_filetimes_to_stattimes (FILETIME *creation,
				       FILETIME *last_access,
				       FILETIME *last_write,
				       PIKE_STAT_T *stat)
{
  time_t t;

  if ((t = fat_filetime_to_time_t (last_write)) < 0)
    return -1;
  stat->st_mtime = t;

  if (last_access->dwLowDateTime || last_access->dwHighDateTime) {
    if ((t = fat_filetime_to_time_t (last_access)) < 0)
      return -1;
    stat->st_atime = t;
  }
  else
    stat->st_atime = stat->st_mtime;

  /* Putting the creation time in the last status change field.. :\ */
  if (creation->dwLowDateTime || creation->dwHighDateTime) {
    if ((t = fat_filetime_to_time_t (creation)) < 0)
      return -1;
    stat->st_ctime = t;
  }
  else
    stat->st_ctime = stat->st_mtime;

  return 0;
}

static void nonfat_filetimes_to_stattimes (FILETIME *creation,
					   FILETIME *last_access,
					   FILETIME *last_write,
					   PIKE_STAT_T *stat)
{
  stat->st_mtime = convert_filetime_to_time_t (last_write);

  if (last_access->dwLowDateTime ||
      last_access->dwHighDateTime)
    stat->st_atime = convert_filetime_to_time_t(last_access);
  else
    stat->st_atime = stat->st_mtime;

  /* Putting the creation time in the last status change field.. :\ */
  if (creation->dwLowDateTime ||
      creation->dwHighDateTime)
    stat->st_ctime = convert_filetime_to_time_t (creation);
  else
    stat->st_ctime = stat->st_mtime;
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

static int IsUncRoot(const p_wchar1 *path)
{
  /* root UNC names start with 2 slashes */
  if ( wcslen(path) >= 5 &&     /* minimum string is "//x/y" */
       ISSEPARATOR(path[0]) &&
       ISSEPARATOR(path[1]) )
  {
    const p_wchar1 *p = path + 2 ;
    
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

PMOD_EXPORT p_wchar1 *pike_dwim_utf8_to_utf16(const p_wchar0 *str)
{
  /* NB: Maximum expansion factor is 2.
   *
   *   UTF8	UTF16	Range			Expansion
   *   1 byte	2 bytes	U000000 - U00007f	2
   *   2 bytes	2 bytes	U000080 - U0007ff	1
   *   3 bytes	2 bytes	U000800 - U00ffff	0.67
   *   4 bytes	4 bytes	U010000 - U10ffff	1
   *
   * NB: Some extra padding at the end for NUL and adding
   *     of terminating slashes, etc.
   */
  size_t len = strlen(str);
  p_wchar1 *res = malloc((len + 4) * sizeof(p_wchar1));
  size_t i = 0, j = 0;

  if (!res) {
    return NULL;
  }

  while (i < len) {
    p_wchar0 c = str[i++];
    p_wchar2 ch, mask = 0x3f;
    if (!(c & 0x80)) {
      /* US-ASCII */
      res[j++] = c;
      continue;
    }
    if (!(c & 0x40)) {
      /* Continuation character. Invalid. Retry as Latin-1. */
      goto latin1_to_utf16;
    }
    ch = c;
    while (c & 0x40) {
      p_wchar0 cc = str[i++];
      if ((cc & 0xc0) != 0x80) {
	/* Expected continuation character. */
	goto latin1_to_utf16;
      }
      ch = ch<<6 | (cc & 0x3f);
      mask |= mask << 5;
      c <<= 1;
    }
    ch &= mask;
    if (ch < 0) {
      goto latin1_to_utf16;
    }
    if (ch < 0x10000) {
      res[j++] = ch;
      continue;
    }
    ch -= 0x10000;
    if (ch >= 0x100000) {
      goto latin1_to_utf16;
    }
    /* Encode with surrogates. */
    res[j++] = 0xd800 | ch >> 10;
    res[j++] = 0xdc00 | (ch & 0x3ff);
  }
  goto done;

 latin1_to_utf16:
  /* DWIM: Assume Latin-1. Just widen the string. */
  for (j = 0; j < len; j++) {
    res[j] = str[j];
  }

 done:
  res[j++] = 0;	/* NUL-termination. */
  return res;
}

PMOD_EXPORT p_wchar0 *pike_utf16_to_utf8(const p_wchar1 *str)
{
  /* NB: Maximum expansion factor is 1.5.
   *
   *   UTF16	UTF8	Range			Expansion
   *   2 bytes	1 byte	U000000 - U00007f	0.5
   *   2 bytes	2 bytes	U000080 - U0007ff	1
   *   2 bytes	3 bytes	U000800 - U00d7ff	1.5
   *   2 bytes	2 bytes	U00d800 - U00dfff	1
   *   2 bytes	3 bytes	U00e000 - U00ffff	1.5
   *   4 bytes	4 bytes	U010000 - U10ffff	1
   *
   * NB: Some extra padding at the end for NUL and adding
   *     of terminating slashes, etc.
   */
  size_t i = 0, j = 0;
  size_t sz = 0;
  p_wchar1 c;
  p_wchar0 *ret;

  while ((c = str[i++])) {
    sz++;
    if (c < 0x80) continue;
    sz++;
    if (c < 0x0800) continue;
    if ((c & 0xf800) == 0xd800) {
      /* One half of a surrogate pair. */
      continue;
    }
    sz++;
  }
  sz++;	/* NUL termination. */

  ret = malloc(sz);
  if (!ret) return NULL;

  for (i = 0; (c = str[i]); i++) {
    if (c < 0x80) {
      ret[j++] = DO_NOT_WARN(c & 0x7f);
      continue;
    }
    if (c < 0x800) {
      ret[j++] = 0xc0 | (c>>6);
      ret[j++] = 0x80 | (c & 0x3f);
      continue;
    }
    if ((c & 0xf800) == 0xd800) {
      /* Surrogate */
      if ((c & 0xfc00) == 0xd800) {
	p_wchar2 ch = str[++i];
	if ((ch & 0xfc00) != 0xdc00) {
	  free(ret);
	  return NULL;
	}
	ch = 0x100000 | (ch & 0x3ff) | ((c & 0x3ff)<<10);
	ret[j++] = 0xf0 | (ch >> 18);
	ret[j++] = 0x80 | ((ch >> 12) & 0x3f);
	ret[j++] = 0x80 | ((ch >> 6) & 0x3f);
	ret[j++] = 0x80 | (ch & 0x3f);
	continue;
      }
      free(ret);
      return NULL;
    }
    ret[j++] = 0xe0 | (c >> 12);
    ret[j++] = 0x80 | ((c >> 6) & 0x3f);
    ret[j++] = 0x80 | (c & 0x3f);
  }
  ret[j++] = 0;
  return ret;
}

/* Note 1: s->st_mtime is the creation time for non-root directories.
 *
 * Note 2: Root directories (e.g. C:\) and network share roots (e.g.
 * \\server\foo\) have no time information at all. All timestamps are
 * set to one year past the start of the epoch for these.
 *
 * Note 3: s->st_ctime is set to the file creation time. It should
 * probably be the last access time to be closer to the unix
 * counterpart, but the creation time is admittedly more useful. */
PMOD_EXPORT int debug_fd_stat(const char *file, PIKE_STAT_T *buf)
{
  ptrdiff_t l;
  p_wchar1 *fname;
  int drive;       		/* current = -1, A: = 0, B: = 1, ... */
  HANDLE hFind;
  WIN32_FIND_DATAW findbuf;
  int exec_bits = 0;

  /* Note: On NT the following characters are illegal in filenames:
   *  \ / : * ? " < > |
   *
   * The first three are valid in paths, so check for the remaining 6.
   */
  if (strpbrk(file, "*?\"<>|"))
  {
    errno = ENOENT;
    return(-1);
  }

  fname = pike_dwim_utf8_to_utf16(file);
  if (!fname) {
    errno = ENOMEM;
    return -1;
  }

  l = wcslen(fname);
  /* Get rid of terminating slashes. */
  while (l && ISSEPARATOR(fname[l-1])) {
    fname[--l] = 0;
  }

  /* get disk from file */
  if (fname[1] == ':')
    drive = toupper(*fname) - 'A';
  else
    drive = -1;


  /* Look at the extension to see if a file appears to be executable. */
  if ((l >= 4) && (fname[l-4] == '.')) {
    char ext[4];
    int i;
    for (i = 0; i < 3; i++) {
      p_wchar1 c = fname[l + i - 4];
      ext[i] = (c > 128)?0:tolower(c);
    }
    ext[3] = 0;
    if (!strcmp (ext, "exe") ||
	!strcmp (ext, "cmd") ||
	!strcmp (ext, "bat") ||
	!strcmp (ext, "com"))
      exec_bits = 0111;	/* Execute perm for all. */
  }

  STATDEBUG (fprintf (stderr, "fd_stat %s drive %d\n", file, drive));

  /* get info for file */
  hFind = FindFirstFileW(fname, &findbuf);

  if ( hFind == INVALID_HANDLE_VALUE )
  {
    UINT drive_type;
    p_wchar1 *abspath;

    STATDEBUG (fprintf (stderr, "check root dir\n"));

    if (!strpbrk(file, ":./\\")) {
      STATDEBUG (fprintf (stderr, "no path separators\n"));
      free(fname);
      errno = ENOENT;
      return -1;
    }

    /* NB: One extra byte for the terminating separator. */
    abspath = malloc((l + _MAX_PATH + 1 + 1) * sizeof(p_wchar1));
    if (!abspath) {
      free(fname);
      errno = ENOMEM;
      return -1;
    }

    if (!_wfullpath( abspath, fname, l + _MAX_PATH ) ||
	/* Neither root dir ('C:\') nor UNC root dir ('\\server\share\'). */
	(wcslen(abspath) > 3 && !IsUncRoot (abspath))) {
      STATDEBUG (fprintf (stderr, "not a root %S\n", abspath));
      free(abspath);
      free(fname);
      errno = ENOENT;
      return -1;
    }

    free(fname);
    fname = NULL;

    STATDEBUG (fprintf (stderr, "abspath: %S\n", abspath));

    l = wcslen(abspath);
    if (!ISSEPARATOR (abspath[l - 1])) {
      /* Ensure there's a slash at the end or else GetDriveType
       * won't like it. */
      abspath[l] = '\\';
      abspath[l + 1] = 0;
    }

    drive_type = GetDriveTypeW (abspath);
    if (drive_type == DRIVE_UNKNOWN || drive_type == DRIVE_NO_ROOT_DIR) {
      STATDEBUG (fprintf (stderr, "invalid drive type: %u\n", drive_type));
      free(abspath);
      errno = ENOENT;
      return -1;
    }

    free(abspath);
    abspath = NULL;

    STATDEBUG (fprintf (stderr, "faking root stat\n"));

    /* Root directories (e.g. C:\ and \\server\share\) are faked */
    findbuf.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
    findbuf.nFileSizeHigh = 0;
    findbuf.nFileSizeLow = 0;
    findbuf.cFileName[0] = '\0';

    /* Don't have any timestamp info, so set it to some date. The
     * following is ridiculously complex, but it's compatible with the
     * stat function in MS CRT. */
    {
      struct tm t;
      t.tm_year = 1980 - 1900;
      t.tm_mon  = 0;
      t.tm_mday = 1;
      t.tm_hour = 0;
      t.tm_min  = 0;
      t.tm_sec  = 0;
      t.tm_isdst = -1;
      buf->st_mtime = mktime (&t);
      buf->st_atime = buf->st_mtime;
      buf->st_ctime = buf->st_mtime;
    }
  }

  else {
    p_wchar1 fstype[50];
    /* Really only need room in this buffer for "FAT" and anything
     * longer that begins with "FAT", but since GetVolumeInformation
     * has shown to trig unreliable error codes for a too short buffer
     * (see below), we allocate ample space. */

    BOOL res;

    fstype[0] = '-';
    fstype[1] = 0;

    if (fname[1] == ':') {
      /* Construct a string "X:\" in fname. */
      fname[0] = toupper(fname[0]);
      fname[2] = '\\';
      fname[3] = 0;
    } else {
      free(fname);
      fname = NULL;
    }

    res = GetVolumeInformationW (fname, NULL, 0, NULL, NULL,NULL,
				 fstype, sizeof(fstype)/sizeof(fstype[0]));

    if (fname) {
      free(fname);
      fname = NULL;
    }

    STATDEBUG (fprintf (stderr, "found, vol info: %d, %S\n",
			res, fstype));

    if (!res) {
      unsigned long w32_err = GetLastError();
      /* Get ERROR_MORE_DATA if the fstype buffer wasn't long enough,
       * so let's ignore it. That error is also known to be reported
       * as ERROR_BAD_LENGTH in Vista and 7. */
      if (w32_err != ERROR_MORE_DATA && w32_err != ERROR_BAD_LENGTH) {
	STATDEBUG (fprintf (stderr, "GetVolumeInformation failure: %d\n",
			    w32_err));
	set_errno_from_win32_error (w32_err);
	FindClose (hFind);
	return -1;
      }
    }

    if (res && (fstype[0] == 'F') && (fstype[1] == 'A') && (fstype[2] == 'T')) {
      if (!fat_filetimes_to_stattimes (&findbuf.ftCreationTime,
				       &findbuf.ftLastAccessTime,
				       &findbuf.ftLastWriteTime,
				       buf)) {
	STATDEBUG (fprintf (stderr, "fat_filetimes_to_stattimes failed.\n"));
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

  if (findbuf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    /* Always label a directory as executable. Note that this also
     * catches special locations like root dirs and network share
     * roots. */
    buf->st_mode = S_IFDIR | 0111;
  else {
    buf->st_mode = S_IFREG | exec_bits;
  }

  /* The file is read/write unless the read only flag is set. */
  if (findbuf.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
    buf->st_mode |= 0444;	/* Read perm for all. */
  else
    buf->st_mode |= 0666;	/* Read and write perm for all. */

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

PMOD_EXPORT int debug_fd_truncate(const char *file, INT64 len)
{
  p_wchar1 *fname = pike_dwim_utf8_to_utf16(file);
  HANDLE h;
  LONG high;

  if (!fname) {
    errno = ENOMEM;
    return -1;
  }

  h = CreateFileW(fname, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
		  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  free(fname);

  if (h == INVALID_HANDLE_VALUE) {
    errno = GetLastError();
    return -1;
  }

  high = DO_NOT_WARN((LONG)(len >> 32));
  len &= (INT64)0xffffffffUL;

  if (SetFilePointer(h, DO_NOT_WARN((long)len), &high, FILE_BEGIN) ==
      INVALID_SET_FILE_POINTER) {
    DWORD err = GetLastError();
    if (err != NO_ERROR) {
      errno = err;
      CloseHandle(h);
      return -1;
    }
  }
  if (!SetEndOfFile(h)) {
    errno = GetLastError();
    CloseHandle(h);
    return -1;
  }
  CloseHandle(h);
  return 0;
}

PMOD_EXPORT int debug_fd_rmdir(const char *dir)
{
  p_wchar1 *dname = pike_dwim_utf8_to_utf16(dir);
  int ret;

  if (!dname) {
    errno = ENOMEM;
    return -1;
  }

  ret = _wrmdir(dname);

  if (ret && (errno == EACCES)) {
    PIKE_STAT_T st;
    if (!fd_stat(dir, &st) && !(st.st_mode & _S_IWRITE) &&
	!_wchmod(dname, st.st_mode | _S_IWRITE)) {
      /* Retry with write permission. */
      ret = _wrmdir(dname);

      if (ret) {
	/* Failed anyway. Restore original permissions. */
	int err = errno;
	_wchmod(dname, st.st_mode);
	errno = err;
      }
    }
  }

  free(dname);

  return ret;
}

PMOD_EXPORT int debug_fd_unlink(const char *file)
{
  p_wchar1 *fname = pike_dwim_utf8_to_utf16(file);
  int ret;

  if (!fname) {
    errno = ENOMEM;
    return -1;
  }

  ret = _wunlink(fname);

  if (ret && (errno == EACCES)) {
    PIKE_STAT_T st;
    if (!fd_stat(file, &st) && !(st.st_mode & _S_IWRITE) &&
	!_wchmod(fname, st.st_mode | _S_IWRITE)) {
      /* Retry with write permission. */
      ret = _wunlink(fname);

      if (ret) {
	/* Failed anyway. Restore original permissions. */
	int err = errno;
	_wchmod(fname, st.st_mode);
	errno = err;
      }
    }
  }

  free(fname);

  return ret;
}

PMOD_EXPORT int debug_fd_mkdir(const char *dir, int mode)
{
  p_wchar1 *dname = pike_dwim_utf8_to_utf16(dir);
  int ret;
  int mask;

  if (!dname) {
    errno = ENOMEM;
    return -1;
  }

  mask = _umask(~mode & 0777);
  _umask(~mode & mask & 0777);
  ret = _wmkdir(dname);
  _wchmod(dname, mode & ~mask & 0777);
  _umask(mask);

  free(dname);

  return ret;
}

PMOD_EXPORT int debug_fd_rename(const char *old, const char *new)
{
  p_wchar1 *oname = pike_dwim_utf8_to_utf16(old);
  p_wchar1 *nname;
  int ret;

  if (!oname) {
    errno = ENOMEM;
    return -1;
  }

  nname = pike_dwim_utf8_to_utf16(new);
  if (!nname) {
    free(oname);
    errno = ENOMEM;
    return -1;
  }

  if (Pike_NT_MoveFileExW) {
    int retried = 0;
    PIKE_STAT_T st;
    ret = 0;
  retry:
    if (!Pike_NT_MoveFileExW(oname, nname, MOVEFILE_REPLACE_EXISTING)) {
      DWORD err = GetLastError();
      if ((err == ERROR_ACCESS_DENIED) && !retried) {
	/* This happens when the destination is an already existing
	 * directory. On POSIX this operation is valid if oname and
	 * nname are directories, and the nname directory is empty.
	 */
	if (!fd_stat(old, &st) && ((st.st_mode & S_IFMT) == S_IFDIR) &&
	    !fd_stat(new, &st) && ((st.st_mode & S_IFMT) == S_IFDIR) &&
	    !_wrmdir(nname)) {
	  /* Succeeded in removing the destination. This implies
	   * that it was an empty directory.
	   *
	   * Retry.
	   */
	  retried = 1;
	  goto retry;
	}
      }
      if (retried) {
	/* Recreate the destination directory that we deleted above. */
	_wmkdir(nname);
	/* NB: st still contains the flags from the original nname dir. */
	_wchmod(nname, st.st_mode & 0777);
      }
      ret = -1;
      set_errno_from_win32_error(err);
    }
  } else {
    /* Fall back to rename() for W98 and earlier. Unlike MoveFileEx,
     * it can't move directories between directories. */
    ret = _wrename(oname, nname);
  }

  free(oname);
  free(nname);

  return ret;
}

PMOD_EXPORT int debug_fd_chdir(const char *dir)
{
  p_wchar1 *dname = pike_dwim_utf8_to_utf16(dir);
  int ret;

  if (!dname) {
    errno = ENOMEM;
    return -1;
  }

  ret = _wchdir(dname);

  free(dname);

  return ret;
}

PMOD_EXPORT char *debug_fd_get_current_dir_name(void)
{
  /* NB: Windows CRT _wgetcwd() has a special case for buf == NULL,
   *     where the buffer is instead allocated with malloc(), and
   *     len is the minimum buffer size to allocate.
   */
  p_wchar1 *utf16buffer = _wgetcwd(NULL, 0);
  p_wchar0 *utf8buffer;

  if (!utf16buffer) {
    errno = ENOMEM;
    return NULL;
  }

  utf8buffer = pike_utf16_to_utf8(utf16buffer);
  libc_free(utf16buffer);

  if (!utf8buffer) {
    errno = ENOMEM;
    return NULL;
  }

  return utf8buffer;
}

PMOD_EXPORT FD debug_fd_open(const char *file, int open_mode, int create_mode)
{
  HANDLE x;
  FD fd;
  DWORD omode,cmode = 0,amode;

  ptrdiff_t l;
  p_wchar1 *fname;


  /* Note: On NT the following characters are illegal in filenames:
   *  \ / : * ? " < > |
   *
   * The first three are valid in paths, so check for the remaining 6.
   */
  if (strpbrk(file, "*?\"<>|"))
  {
    /* ENXIO:
     *   "The file is a device special file and no corresponding device
     *    exists."
     */
    errno = ENXIO;
    return -1;
  }

  fname = pike_dwim_utf8_to_utf16(file);
  if (!fname) {
    errno = ENOMEM;
    return -1;
  }

  l = wcslen(fname);

  /* Get rid of terminating slashes. */
  while (l && ISSEPARATOR(fname[l-1])) {
    fname[--l] = 0;
  }

  omode=0;
  FDDEBUG(fprintf(stderr, "fd_open(%S, 0x%x, %o)\n",
		  fname, open_mode, create_mode));

  fd = allocate_fd(FD_FILE, INVALID_HANDLE_VALUE);
  if (fd < 0) {
    free(fname);
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
    
  x=CreateFileW(fname,
		omode,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		cmode,
		amode,
		NULL);

  
  if(x == DO_NOT_WARN(INVALID_HANDLE_VALUE))
  {
    unsigned long err = GetLastError();
    free(fname);
    if (err == ERROR_INVALID_NAME)
      /* An invalid name means the file doesn't exist. This is
       * consistent with fd_stat, opendir, and unix. */
      errno = ENOENT;
    else
      set_errno_from_win32_error (err);
    return -1;
  }

  SetHandleInformation(x,HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);

  set_fd_handle(fd, x);

  release_fd(fd);

  if(open_mode & fd_APPEND)
    fd_lseek(fd,0,SEEK_END);

  FDDEBUG(fprintf(stderr, "Opened %S file as %d (%d)\n", fname, fd, x));

  free(fname);

  return fd;
}

PMOD_EXPORT FD debug_fd_socket(int domain, int type, int proto)
{
  FD fd;
  SOCKET s;

  fd = allocate_fd(FD_SOCKET, (HANDLE)INVALID_SOCKET);
  if (fd < 0) return -1;

  s=socket(domain, type, proto);

  if(s==INVALID_SOCKET)
  {
    DWORD err = WSAGetLastError();
    free_fd(fd);
    set_errno_from_win32_error (err);
    return -1;
  }
  
  SetHandleInformation((HANDLE)s,
		       HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);

  set_fd_handle(fd, (HANDLE)s);

  release_fd(fd);

  FDDEBUG(fprintf(stderr,"New socket: %d (%d)\n",fd,s));

  return fd;
}

PMOD_EXPORT int debug_fd_pipe(int fds[2] DMALLOC_LINE_ARGS)
{
  int tmp_fds[2];
  HANDLE files[2];

  tmp_fds[0] = allocate_fd(FD_PIPE, INVALID_HANDLE_VALUE);
  if (tmp_fds[0] < 0) {
    return -1;
  }
  tmp_fds[1] = allocate_fd(FD_PIPE, INVALID_HANDLE_VALUE);
  if (tmp_fds[1] < 0) {
    free_fd(tmp_fds[0]);
    return -1;
  }

  if(!CreatePipe(&files[0], &files[1], NULL, 0))
  {
    DWORD err = GetLastError();
    free_fd(tmp_fds[0]);
    free_fd(tmp_fds[1]);
    set_errno_from_win32_error (err);
    return -1;
  }
  
  FDDEBUG(fprintf(stderr,"ReadHANDLE=%d WriteHANDLE=%d\n",files[0],files[1]));
  
  SetHandleInformation(files[0],HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);
  SetHandleInformation(files[1],HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE,0);

  fds[0] = tmp_fds[0];
  set_fd_handle(fds[0], files[0]);
  release_fd(fds[0]);

  fds[1] = tmp_fds[1];
  set_fd_handle(fds[1], files[1]);
  release_fd(fds[1]);

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

  FDDEBUG(fprintf(stderr,"Accept on %d (%ld)..\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));

  if (fd_to_socket(fd, &s) < 0) return -1;

  new_fd = allocate_fd(FD_SOCKET, (HANDLE)INVALID_SOCKET);
  if (new_fd < 0) return -1;

  s = accept(s, addr, addrlen);

  if(s==INVALID_SOCKET)
  {
    DWORD err = WSAGetLastError();
    free_fd(new_fd);
    set_errno_from_win32_error(err);
    FDDEBUG(fprintf(stderr,"Accept failed with errno %d\n",errno));
    return -1;
  }
  
  SetHandleInformation((HANDLE)s,
		       HANDLE_FLAG_INHERIT|HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
  set_fd_handle(new_fd, (HANDLE)s);
  release_fd(new_fd);

  FDDEBUG(fprintf(stderr,"Accept on %d (%ld) returned new socket: %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]),
		  new_fd, PTRDIFF_T_TO_LONG((ptrdiff_t)s)));

  return new_fd;
}


#define SOCKFUN(NAME,X1,X2) \
PMOD_EXPORT int PIKE_CONCAT(debug_fd_,NAME) X1 { \
  SOCKET s; \
  int ret; \
  FDDEBUG(fprintf(stderr, #NAME " on %d (%ld)\n", \
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]))); \
  if (fd_to_socket(fd, &s) < 0) return -1; \
  ret = NAME X2; \
  release_fd(fd); \
  if(ret == SOCKET_ERROR) { \
    set_errno_from_win32_error (WSAGetLastError()); \
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
  FDDEBUG(fprintf(stderr, "connect on %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]));
	  for(ret=0;ret<len;ret++)
	  fprintf(stderr," %02x",((unsigned char *)a)[ret]);
	  fprintf(stderr,"\n");
  );
  if (fd_to_socket(fd, &ret) < 0) return -1;

  ret=connect(ret,a,len);

  release_fd(fd);

  if(ret == SOCKET_ERROR) set_errno_from_win32_error (WSAGetLastError());
  FDDEBUG(fprintf(stderr, "connect returned %d (%d)\n",ret,errno)); 
  return DO_NOT_WARN((int)ret);
}

PMOD_EXPORT int debug_fd_close(FD fd)
{
  HANDLE h;
  int type;

  if (fd_to_handle(fd, &type, &h) < 0) return -1;

  FDDEBUG(fprintf(stderr, "Closing %d (%ld)\n", fd, PTRDIFF_T_TO_LONG(h)));

  free_fd(fd);

  switch(type)
  {
    case FD_SOCKET:
      if(closesocket((SOCKET)h))
      {
	set_errno_from_win32_error (GetLastError());
	FDDEBUG(fprintf(stderr,"Closing %d (%ld) failed with errno=%d\n",
			fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]),
			errno));
	return -1;
      }
      break;

    default:
      if(!CloseHandle(h))
      {
	set_errno_from_win32_error (GetLastError());
	return -1;
      }
  }

  FDDEBUG(fprintf(stderr,"%d (%ld) closed\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));

  return 0;
}

PMOD_EXPORT ptrdiff_t debug_fd_write(FD fd, void *buf, ptrdiff_t len)
{
  int kind;
  HANDLE handle;

  FDDEBUG(fprintf(stderr, "Writing %d bytes to %d (%d)\n",
		  len, fd, da_handle[fd]));

  if (fd_to_handle(fd, &kind, &handle) < 0) return -1;

  switch(kind)
  {
    case FD_SOCKET:
      {
	ptrdiff_t ret = send((SOCKET)handle, buf,
			     DO_NOT_WARN((int)len),
			     0);
	release_fd(fd);
	if(ret<0)
	{
	  set_errno_from_win32_error (WSAGetLastError());
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
	  set_errno_from_win32_error (GetLastError());
	  FDDEBUG(fprintf(stderr, "Write on %d failed (%d)\n", fd, errno));
	  release_fd(fd);
	  return -1;
	}
	FDDEBUG(fprintf(stderr, "Wrote %ld bytes to %d)\n", (long)ret, fd));
	release_fd(fd);
	return ret;
      }

    default:
      errno=ENOTSUPP;
      release_fd(fd);
      return -1;
  }
}

PMOD_EXPORT ptrdiff_t debug_fd_read(FD fd, void *to, ptrdiff_t len)
{
  int type;
  DWORD ret;
  ptrdiff_t rret;
  HANDLE handle;

  FDDEBUG(fprintf(stderr,"Reading %d bytes from %d (%d) to %lx\n",
		  len, fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]),
		  PTRDIFF_T_TO_LONG((ptrdiff_t)to)));

  if (fd_to_handle(fd, &type, &handle) < 0) return -1;

  switch(type)
  {
    case FD_SOCKET:
      rret=recv((SOCKET)handle, to,
		DO_NOT_WARN((int)len),
		0);
      release_fd(fd);
      if(rret<0)
      {
	set_errno_from_win32_error (WSAGetLastError());
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
	unsigned int err = GetLastError();
	release_fd(fd);
	set_errno_from_win32_error (err);
	switch(err)
	{
	  /* Pretend we reached the end of the file */
	  case ERROR_BROKEN_PIPE:
	    return 0;
	}
	FDDEBUG(fprintf(stderr,"Read failed %d\n",errno));
	return -1;
      }
      FDDEBUG(fprintf(stderr,"Read on %d returned %ld\n",fd,ret));
      release_fd(fd);
      return ret;

    default:
      errno=ENOTSUPP;
      release_fd(fd);
      return -1;
  }
}

PMOD_EXPORT PIKE_OFF_T debug_fd_lseek(FD fd, PIKE_OFF_T pos, int where)
{
  PIKE_OFF_T ret;
  int type;
  HANDLE h;

  if (fd_to_handle(fd, &type, &h) < 0) return -1;
  if(type != FD_FILE)
  {
    release_fd(fd);
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

  {
#ifdef INT64
#ifdef HAVE_SETFILEPOINTEREX
    /* Windows NT based. */
    LARGE_INTEGER li_pos;
    LARGE_INTEGER li_ret;
    li_pos.QuadPart = pos;
    li_ret.QuadPart = 0;
    if(!SetFilePointerEx(h, li_pos, &li_ret, where)) {
      set_errno_from_win32_error (GetLastError());
      release_fd(fd);
      return -1;
    }
    ret = li_ret.QuadPart;
#else /* !HAVE_SETFILEPOINTEREX */
    /* Windows 9x based. */
    LONG high = DO_NOT_WARN((LONG)(pos >> 32));
    DWORD err;
    pos &= ((INT64) 1 << 32) - 1;
    ret = SetFilePointer(h, DO_NOT_WARN((LONG)pos), &high, where);
    if (ret == INVALID_SET_FILE_POINTER &&
	(err = GetLastError()) != NO_ERROR) {
      set_errno_from_win32_error (err);
      release_fd(fd);
      return -1;
    }
    ret += (INT64) high << 32;
#endif /* HAVE_SETFILEPOINTEREX */
#else /* !INT64 */
    ret = SetFilePointer(h, (LONG)pos, NULL, where);
    if(ret == INVALID_SET_FILE_POINTER)
    {
      set_errno_from_win32_error (GetLastError());
      release_fd(fd);
      return -1;
    }
#endif /* INT64 */
  }

  release_fd(fd);

  return ret;
}

PMOD_EXPORT int debug_fd_ftruncate(FD fd, PIKE_OFF_T len)
{
  int type;
  HANDLE h;
  LONG oldfp_lo, oldfp_hi, len_hi;
  DWORD err;

  if (fd_to_handle(fd, &type, &h) < 0) return -1;
  if(type != FD_FILE)
  {
    release_fd(fd);
    errno=ENOTSUPP;
    return -1;
  }

  oldfp_hi = 0;
  oldfp_lo = SetFilePointer(h, 0, &oldfp_hi, FILE_CURRENT);
  if(!~oldfp_lo) {
    err = GetLastError();
    if(err != NO_ERROR) {
      release_fd(fd);
      set_errno_from_win32_error (err);
      return -1;
    }
  }

#ifdef INT64
  len_hi = DO_NOT_WARN ((LONG) (len >> 32));
  len &= ((INT64) 1 << 32) - 1;
#else
  len_hi = 0;
#endif

  if (SetFilePointer (h, DO_NOT_WARN ((LONG) len), &len_hi, FILE_BEGIN) ==
      INVALID_SET_FILE_POINTER &&
      (err = GetLastError()) != NO_ERROR) {
    SetFilePointer(h, oldfp_lo, &oldfp_hi, FILE_BEGIN);
    release_fd(fd);
    set_errno_from_win32_error (err);
    return -1;
  }

  if(!SetEndOfFile(h)) {
    set_errno_from_win32_error (GetLastError());
    SetFilePointer(h, oldfp_lo, &oldfp_hi, FILE_BEGIN);
    release_fd(fd);
    return -1;
  }

  if (oldfp_hi < len_hi || (oldfp_hi == len_hi && oldfp_lo < len))
    if(!~SetFilePointer(h, oldfp_lo, &oldfp_hi, FILE_BEGIN)) {
      err = GetLastError();
      if(err != NO_ERROR) {
	release_fd(fd);
	set_errno_from_win32_error (err);
	return -1;
      }
    }

  release_fd(fd);
  return 0;
}

PMOD_EXPORT int debug_fd_flock(FD fd, int oper)
{
  long ret;
  int type;
  HANDLE h;

  if (fd_to_handle(fd, &type, &h) < 0) return -1;
  if(type != FD_FILE)
  {
    release_fd(fd);
    errno=ENOTSUPP;
    return -1;
  }

  if(oper & fd_LOCK_UN)
  {
    ret=UnlockFile(h,
		   0,
		   0,
		   0xffffffff,
		   0xffffffff);
  }else{
    DWORD flags = 0;
    OVERLAPPED tmp;
    memset(&tmp, 0, sizeof(tmp));
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

  release_fd(fd);

  if(ret<0)
  {
    set_errno_from_win32_error (GetLastError());
    return -1;
  }
  
  return 0;
}


/* Note: s->st_ctime is set to the file creation time. It should
 * probably be the last access time to be closer to the unix
 * counterpart, but the creation time is admittedly more useful. */
PMOD_EXPORT int debug_fd_fstat(FD fd, PIKE_STAT_T *s)
{
  int type;
  HANDLE h;
  FILETIME c,a,m;

  FDDEBUG(fprintf(stderr, "fstat on %d (%ld)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd])));
  if (fd_to_handle(fd, &type, &h) < 0) return -1;
  if (type != FD_FILE)
  {
    release_fd(fd);
    errno=ENOTSUPP;
    return -1;
  }

  memset(s, 0, sizeof(PIKE_STAT_T));
  s->st_nlink=1;

  switch(type)
  {
    case FD_SOCKET:
      s->st_mode=S_IFSOCK;
      break;

    default:
      switch(GetFileType(h))
      {
	default:
	case FILE_TYPE_UNKNOWN: s->st_mode=0;        break;

	case FILE_TYPE_DISK:
	  s->st_mode=S_IFREG;
	  {
	    DWORD high, err;
	    s->st_size=GetFileSize(h, &high);
	    if (s->st_size == INVALID_FILE_SIZE &&
		(err = GetLastError()) != NO_ERROR) {
	      release_fd(fd);
	      set_errno_from_win32_error (err);
	      return -1;
	    }
#ifdef INT64
	    s->st_size += (INT64) high << 32;
#else
	    if (high) s->st_size = MAXDWORD;
#endif
	  }

	  if(!GetFileTime(h, &c, &a, &m))
	  {
	    set_errno_from_win32_error (GetLastError());
	    release_fd(fd);
	    return -1;
	  }

	  /* FIXME: Determine the filesystem type to use
	   * fat_filetimes_to_stattimes when necessary. */

	  nonfat_filetimes_to_stattimes (&c, &a, &m, s);
	  break;

	case FILE_TYPE_CHAR:    s->st_mode=S_IFCHR;  break;
	case FILE_TYPE_PIPE:    s->st_mode=S_IFIFO; break;
      }
  }
  release_fd(fd);
  s->st_mode |= 0666;
  return 0;
}


#ifdef FD_DEBUG
static void dump_FDSET(FD_SET *x, int fds)
{
  if(x)
  {
    int e, first=1;
    fprintf(stderr,"[");
    for(e = 0; e < FD_SET_SIZE; e++)
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
    set_errno_from_win32_error (WSAGetLastError());
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
  SOCKET s;

  FDDEBUG(fprintf(stderr,"ioctl(%d (%ld,%d,%p)\n",
		  fd, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[fd]), cmd, data));

  if (fd_to_socket(fd, &s) < 0) return -1;

  ret = ioctlsocket(s, cmd, data);

  FDDEBUG(fprintf(stderr,"ioctlsocket returned %ld (%d)\n",ret,errno));

  release_fd(fd);

  if(ret==SOCKET_ERROR)
  {
    set_errno_from_win32_error (WSAGetLastError());
    return -1;
  }

  return ret;
}


PMOD_EXPORT FD debug_fd_dup(FD from)
{
  FD fd;
  int type;
  HANDLE h,x,p=GetCurrentProcess();

  if (fd_to_handle(from, &type, &h) < 0) return -1;

  fd = allocate_fd(type,
		   (type == FD_SOCKET)?
		   (HANDLE)INVALID_SOCKET:INVALID_HANDLE_VALUE);

  if(!DuplicateHandle(p, h, p, &x, 0, 0, DUPLICATE_SAME_ACCESS))
  {
    DWORD err = GetLastError();
    free_fd(fd);
    release_fd(from);
    set_errno_from_win32_error(err);
    return -1;
  }

  release_fd(from);

  set_fd_handle(fd, x);

  release_fd(fd);

  FDDEBUG(fprintf(stderr,"Dup %d (%ld) to %d (%d)\n",
		  from, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[from]), fd, x));
  return fd;
}

PMOD_EXPORT FD debug_fd_dup2(FD from, FD to)
{
  int type;
  HANDLE h,x,p=GetCurrentProcess();

  if ((from == to) || (to < 0) || (to >= FD_SETSIZE)) {
    errno = EINVAL;
    return -1;
  }

  if (fd_to_handle(from, &type, &h) < 0) return -1;

  if(!DuplicateHandle(p, h, p, &x, 0, 0, DUPLICATE_SAME_ACCESS))
  {
    release_fd(from);
    set_errno_from_win32_error (GetLastError());
    return -1;
  }

  release_fd(from);

  /* NB: Dead-lock proofed by never holding the busy lock for
   *     both from and to.
   */

  if (reallocate_fd(to, type, x) < 0) {
    release_fd(to);

    if (type == FD_SOCKET) {
      closesocket((SOCKET)x);
    } else {
      CloseHandle(x);
    }
    return -1;
  }

  release_fd(to);

  FDDEBUG(fprintf(stderr,"Dup2 %d (%d) to %d (%d)\n",
		  from, PTRDIFF_T_TO_LONG((ptrdiff_t)da_handle[from]), to, x));

  return to;
}

PMOD_EXPORT const char *debug_fd_inet_ntop(int af, const void *addr,
					   char *cp, size_t sz)
{
  static char *(*inet_ntop_funp)(int, void*, char *, size_t);
  static int tried;
  static HINSTANCE ws2_32lib;

  if (!inet_ntop_funp) {
    if (!tried) {
      tried = 1;
      if ((ws2_32lib = LoadLibrary("Ws2_32"))) {
	FARPROC proc;
	if ((proc = GetProcAddress(ws2_32lib, "InetNtopA"))) {
	  inet_ntop_funp = (char *(*)(int, void *, char *, size_t))proc;
	}
      }
    }
    if (!inet_ntop_funp) {
      const unsigned char *q = (const unsigned char *)addr;
      if (af == AF_INET) {
	snprintf(cp, sz, "%d.%d.%d.%d", q[0], q[1], q[2], q[3]);
	return cp;
#ifdef AF_INET6
      } else if (af == AF_INET6) {
	int i;
	char *buf = cp;
	int got_zeros = 0;
	for (i=0; i < 8; i++) {
	  size_t val = (q[0]<<8) | q[1];
	  if (!val) {
	    if (!got_zeros) {
	      snprintf(buf, sz, ":");
	      got_zeros = 1;
	      goto next;
	    } else if (got_zeros == 1) goto next;
	  }
	  got_zeros |= got_zeros << 1;
	  if (i) {
	    snprintf(buf, sz, ":%x", val);
	  } else {
	    snprintf(buf, sz, "%x", val);
	  }
	next:
	  sz -= strlen(buf);
	  buf += strlen(buf);
	  q += 2;
	}
	if (got_zeros == 1) {
	  snprintf(buf, sz, ":");
	  sz -= strlen(buf);
	}
	return cp;
#endif
      }
      return NULL;
    }
  }
  return inet_ntop_funp(af, addr, cp, sz);
}
#endif /* HAVE_WINSOCK_H && !__GNUC__ */

#ifdef EMULATE_DIRECT
PMOD_EXPORT DIR *opendir(char *dir)
{
  ptrdiff_t len;
  p_wchar1 *foo;
  DIR *ret = malloc(sizeof(DIR));

  if(!ret)
  {
    errno=ENOMEM;
    return 0;
  }

  foo = pike_dwim_utf8_to_utf16(dir);
  if (!foo) {
    free(ret);
    errno = ENOMEM;
    return NULL;
  }

  len = wcslen(foo);

  /* This may require appending a slash and a star... */
  if(len && !ISSEPARATOR(foo[len-1])) foo[len++]='/';
  foo[len++]='*';
  foo[len]=0;

/*  fprintf(stderr, "opendir(%S)\n", foo); */

  ret->h = FindFirstFileW(foo, &ret->find_data);
  free(foo);

  if(ret->h == DO_NOT_WARN(INVALID_HANDLE_VALUE))
  {
    /* FIXME: Handle empty directories. */
    errno=ENOENT;
    free(ret);
    return NULL;
  }

  ret->direct.d_name = NULL;
  ret->first=1;
  return ret;
}

PMOD_EXPORT struct dirent *readdir(DIR *dir)
{
  if(!dir->first)
  {
    if(!FindNextFileW(dir->h, &dir->find_data))
    {
      errno = ENOENT;
      return NULL;
    }
  } else {
    dir->first = 0;
  }

  if (dir->direct.d_name) {
    free(dir->direct.d_name);
    dir->direct.d_name = NULL;
  }

  dir->direct.d_name = pike_utf16_to_utf8(dir->find_data.cFileName);

  if (dir->direct.d_name) return &dir->direct;
  errno = ENOMEM;
  return NULL;
}

PMOD_EXPORT void closedir(DIR *dir)
{
  FindClose(dir->h);
  if (dir->direct.d_name) {
    free(dir->direct.d_name);
  }
  free(dir);
}
#endif

#if defined(HAVE_WINSOCK_H) && defined(USE_DL_MALLOC)
/* NB: We use some calls above that allocate memory with the libc malloc. */
#undef free
static inline void libc_free(void *ptr)
{
  if (ptr) free(ptr);
}
#endif
