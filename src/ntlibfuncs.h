/*
 * Functions to lookup dynamically at load on NT.
 */

#ifdef __NT__

#ifndef NTLIB
/* NTLIB(LIBNAME) */
#error NTLIB macro not defined.
#endif
#ifndef NTLIBFUNC
/* NTLIBFUNC(LIBNAME, RETTYPE, SYMBOL, ARGLIST) */
#error NTLIBFUNC macro not defined.
#endif

NTLIB(kernel32);

NTLIBFUNC(kernel32, BOOL, MoveFileExW, (
  LPCWSTR lpExistingFileName,  /* file name     */
  LPCWSTR lpNewFileName,       /* new file name */
  DWORD dwFlags                /* move options  */
));

#endif /* __NT__ */
