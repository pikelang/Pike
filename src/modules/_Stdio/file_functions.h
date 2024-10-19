/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

#define CB_FUNC tOr(tFunc(tNone,tOr(tVoid,tMixed)), tZero)

/* function(string,string,void|int:int) */
FILE_FUNC("open",file_open, tFunc(tStr tStr tOr(tVoid,tInt),tInt))
#ifdef HAVE_OPENAT
/* function(string,string,void|int:int) */
FILE_FUNC("openat",file_openat, tFunc(tStr tStr tOr(tVoid,tInt),tObjImpl_STDIO_FD))
#endif
/* function(string|void:int) */
FILE_FUNC("close",file_close, tFunc(tOr(tStr,tVoid),tInt))
/* function(string|array(string),mixed...:int) */
FILE_FUNC("write",file_write,
          tOr4(tFunc(tStr8, tInt),
               tFuncV(tObj, tOr(tInt, tVoid), tInt),
               tFuncV(tArr(tStr8), tMixed, tInt),
               tFuncV(tAttr("sprintf_format", tStr8),
		      tAttr("sprintf_args", tMixed),tInt)))
/* function(int|void,int|void:string) */
FILE_FUNC("read_oob",file_read_oob, tFunc(tOr(tInt,tVoid) tOr(tInt,tVoid),tStr8))
/* function(string,mixed...:int) */
FILE_FUNC("write_oob",file_write_oob,
	  tOr3(tFunc(tStr, tInt),
	       tFuncV(tArr(tStr), tMixed, tInt),
	       tFuncV(tAttr("sprintf_format", tStr),
		      tAttr("sprintf_args", tMixed),tInt)))
/* TODO kqueue: we should add an read_fs_event() for when not using callback mode. */

#ifdef HAVE_PIKE_SEND_FD
FILE_FUNC("send_fd", file_send_fd, tFunc(tObjIs_STDIO_FD, tVoid))
#endif

/* function(int(-1..65535)|void:int(0..1)) */
FILE_FUNC("linger", file_linger,
	  tFunc(tOr3(tInt_10, tWord, tVoid), tInt01))

/* function(int(0..1)|void:int(0..1)) */
FILE_FUNC("set_nodelay", file_nodelay,
	  tFunc(tOr(tInt01, tVoid), tInt01))

#ifdef HAVE_FSYNC
/*  function(:int) */
FILE_FUNC("sync", file_sync, tFunc(tNone,tInt))
#endif /* HAVE_FSYNC */

/* function(int,int|void,int|void:int) */
FILE_FUNC("seek",file_seek,
          tOr(tFunc(tInt tOr(tNStr(tInt05),tVoid),tInt),
              tAttr("deprecated",tFunc(tInt tInt tOr(tInt,tVoid),tInt))))
/* function(:int) */
FILE_FUNC("tell",file_tell, tFunc(tNone,tInt))
/* function(int:int) */
FILE_FUNC("truncate",file_truncate, tFunc(tInt,tInt))
/* function(:object) */
FILE_FUNC("stat",file_stat, tFunc(tNone,tObjImpl_STDIO_STAT))
#ifdef HAVE_FSTATAT
FILE_FUNC("statat", file_statat,
	  tFunc(tStr tOr(tVoid, tInt01), tObjImpl_STDIO_STAT))
#ifdef HAVE_UNLINKAT
FILE_FUNC("unlinkat", file_unlinkat, tFunc(tStr, tInt01));
#endif /* HAVE_UNLINKAT */
#endif /* HAVE_FSTATAT */
#if defined(HAVE_FDOPENDIR) && defined(HAVE_OPENAT)
FILE_FUNC("get_dir", file_get_dir, tFunc(tOr(tStr,tVoid),tArr(tStr8)));
#endif /* HAVE_FDOPENDIR && HAVE_OPENAT */
/* function(:int) */
FILE_FUNC("errno",file_errno, tFunc(tNone,tInt))
/* function(:int) */
FILE_FUNC("mode",file_mode, tFunc(tNone,tInt))

/* function(int:void) */
FILE_FUNC("set_close_on_exec",file_set_close_on_exec, tFunc(tInt,tVoid))
/* function(:void) */
FILE_FUNC("set_nonblocking",file_set_nonblocking, tFunc(tNone,tVoid))

/*  function(object:void) */
/* Note: We have no way of specifying "object that inherits Pike.Backend". */
FILE_FUNC("set_backend", file_set_backend, tFunc(tObj,tVoid))
/*  function(void:object) */
FILE_FUNC("query_backend", file_query_backend, tFunc(tVoid,tObj))

/* function(mixed:void) */
FILE_FUNC("set_read_callback",file_set_read_callback, tFunc(CB_FUNC,tVoid))
/* function(mixed:void) */
FILE_FUNC("set_write_callback",file_set_write_callback, tFunc(CB_FUNC,tVoid))
/* function(mixed:void) */
FILE_FUNC("set_read_oob_callback",file_set_read_oob_callback, tFunc(CB_FUNC,tVoid))
/* function(mixed:void) */
FILE_FUNC("set_write_oob_callback",file_set_write_oob_callback, tFunc(CB_FUNC,tVoid))
/* function(mixed:void) */
FILE_FUNC("set_error_callback", file_set_error_callback, tFunc(CB_FUNC,tVoid))
/* function(mixed,int:void) */
FILE_FUNC("set_fs_event_callback",file_set_fs_event_callback, tFunc(tOr(tFunc(tInt,tOr(tVoid,tMixed)), tZero) tInt,tVoid))

FILE_FUNC("query_fs_event_flags",file_query_fs_event_flags, tFunc(tVoid,tInt))

#undef CB_FUNC


/* function(:void) */
FILE_FUNC("_enable_callbacks",file__enable_callbacks, tFunc(tNone,tVoid))
/* function(:void) */
FILE_FUNC("_disable_callbacks",file__disable_callbacks, tFunc(tNone,tVoid))

/* function(:void) */
FILE_FUNC("set_blocking",file_set_blocking, tFunc(tNone,tVoid))

FILE_FUNC ("is_open", file_is_open, tFunc(tNone,tInt))
/* function(:int) */
FILE_FUNC("query_fd",file_query_fd, tFunc(tNone,tInt))
/* function(void:int) */
FILE_FUNC("release_fd",file_release_fd, tFunc(tVoid,tInt))
/* function(int:void) */
FILE_FUNC("take_fd",file_take_fd, tFunc(tInt,tVoid))

/* function(object:int) */
/* Note: We have no way of specifying "object that inherits Fd or Fd_ref". */
FILE_FUNC("dup2",file_dup2, tFunc(tObj,tInt))
/* function(void:object) */
FILE_FUNC("dup",file_dup, tFunc(tNone,tObjImpl_STDIO_FD))
/* function(void|int:object) */
FILE_FUNC("pipe",file_pipe, tFunc(tOr(tVoid,tInt),tObjImpl_STDIO_FD))

/* function(int,string|void:void) */
FILE_FUNC("set_buffer",file_set_buffer, tFunc(tInt tOr(tStr,tVoid),tVoid))
/* function(int|string|void,string|void,int|void:int) */
FILE_FUNC("open_socket",file_open_socket,
	  tFunc(tOr3(tInt,tStr,tVoid) tOr(tStr,tVoid) tOr(tInt,tVoid),tInt))
/* function(string,int|string:int)|function(string,int|string,string,int|string:int) */
    FILE_FUNC("connect",file_connect, tOr3(tFunc(tStr tOr(tInt,tStr),tInt),tFunc(tStr tOr(tInt,tStr) tStr tOr(tInt,tStr),tInt),
                                           tFunc(tStr tOr(tInt,tStr) tStr tOr(tInt,tStr) tStr8,tStr)))
#ifdef HAVE_SYS_UN_H
/* function(string:int) */
FILE_FUNC("connect_unix",file_connect_unix, tFunc(tStr,tInt))
#endif /* HAVE_SYS_UN_H */
/* function(int|void:string) */
FILE_FUNC("query_address",file_query_address, tFunc(tOr(tInt01,tVoid),tStr))
#ifdef IP_MTU
/* function(:int) */
FILE_FUNC("query_mtu", file_query_mtu, tFunc(tNone, tInt))
#endif
/* function(void|string,void|string:void) */
FILE_FUNC("create",file_create, tFunc(tOr3(tVoid,tInt,tStr) tOr(tVoid,tStr) tOr(tVoid,tInt),tVoid))

#ifdef _REENTRANT
/* function(object:void) */
FILE_FUNC("proxy",file_proxy, tFunc(tObj,tVoid))
#endif

#if defined(HAVE_FD_FLOCK) || defined(HAVE_FD_LOCKF)
/* function(void|int:object) */
FILE_FUNC("lock",file_lock, tFunc(tOr(tVoid,tInt),tObjImpl_STDIO_FILE_LOCK_KEY))
/* function(void|int:object) */
FILE_FUNC("trylock",file_trylock, tFunc(tOr(tVoid,tInt),tObjImpl_STDIO_FILE_LOCK_KEY))
#endif

#if !defined(__NT__) && (defined(HAVE_POSIX_OPENPT) || defined(PTY_MASTER_PATHNAME))
/* function(string:int) */
FILE_FUNC("openpt",file_openpt, tFunc(tStr,tInt))
#endif

#if defined(HAVE_GRANTPT) || defined(USE_PT_CHMOD) || defined(USE_CHGPT)
/* function(void:string) */
FILE_FUNC("grantpt",file_grantpt, tFunc(tNone,tStr))
#endif

/* From termios.c */
#if defined(HAVE_TERMIOS_H) || defined(HAVE_SYS_TERMIOS_H) || defined(__NT__)
/* function(void:mapping) */
FILE_FUNC("tcgetattr",file_tcgetattr, tFunc(tNone, tMap(tStr7, tInt)))
#ifdef HAVE_TCGETATTR
/* function(mapping, void|string: int(0..1)) */
FILE_FUNC("tcsetattr", file_tcsetattr,
          tFunc(tMap(tStr7, tInt) tOr(tVoid, tStr7), tInt01))
/* function(int: int(0..1)) */
FILE_FUNC("tcsendbreak", file_tcsendbreak, tFunc(tInt, tInt01))
/* function(void|string: int(0..1)) */
FILE_FUNC("tcflush", file_tcflush, tFunc(tOr(tVoid, tStr7), tInt01))
FILE_FUNC("tcdrain", file_tcdrain, tFunc(tNone, tInt01))
/*    FILE_FUNC("tcflow",file_tcflow,"function(string:int)"); */
/*    FILE_FUNC("tcgetpgrp",file_tcgetpgrp,"function(void:int)"); */
/*    FILE_FUNC("tcsetpgrp",file_tcsetpgrp,"function(int:int)"); */
#endif
#ifdef TIOCSWINSZ
FILE_FUNC("tcsetsize", file_tcsetsize, tFunc(tIntPos tIntPos, tInt01))
#endif
#endif

#ifdef SO_KEEPALIVE
/* function(int:int) */
FILE_FUNC("set_keepalive",file_set_keepalive, tFunc(tInt,tInt))
#endif

/* function(int,int:int) */
FILE_FUNC("setsockopt",file_setsockopt, tFunc(tInt tInt,tInt))

#if defined(HAVE_FSETXATTR) && defined(HAVE_FGETXATTR) && defined(HAVE_FLISTXATTR)
FILE_FUNC( "listxattr", file_listxattr, tFunc(tVoid,tArr(tStr)))
FILE_FUNC( "setxattr", file_setxattr, tFunc(tStr tStr tInt,tInt))
FILE_FUNC( "getxattr", file_getxattr, tFunc(tStr,tStr))
FILE_FUNC( "removexattr", file_removexattr, tFunc(tStr,tInt))
#endif

#undef FILE_FUNC
#undef FILE_OBJ
