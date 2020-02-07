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

/* The following are needed for pty handling,
 * and taken from <consoleapi.h> and <processthreadsapi.h>.
 */

NTLIBFUNC(kernel32, HRESULT, CreatePseudoConsole, (
  COORD size,
  HANDLE hInput,
  HANDLE hOutput,
  DWORD dwFlags,
  HPCON *phPC
));

NTLIBFUNC(kernel32, HRESULT, ResizePseudoConsole, (
  HPCON hPC,
  COORD size
));

NTLIBFUNC(kernel32, VOID, ClosePseudoConsole, (
  HPCON hPC
));

NTLIBFUNC(kernel32, BOOL, InitializeProcThreadAttributeList, (
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  DWORD dwAttributeCount,
  DWORD dwFlags,
  PSIZE_T lpSize
));

NTLIBFUNC(kernel32, VOID, DeleteProcThreadAttributeList, (
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList
));

NTLIBFUNC(kernel32, BOOL, UpdateProcThreadAttribute, (
  LPPROC_THREAD_ATTRIBUTE_LIST lpAttributeList,
  DWORD dwFlags,
  DWORD_PTR Attribute,
  PVOID lpValue,
  SIZE_T cbSize,
  PVOID lpPreviousValue,
  PSIZE_T lpReturnSize
));

#endif /* __NT__ */
