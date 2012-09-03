/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * Win32 DLL handling for jvm.c
 *
 * This is not a stand alone compilation unit; it is included
 * from jvm.c and from the configure test
 */

#include <tchar.h>
#ifndef CONFIGURE
#include "program.h"
#endif

#define JNI_CreateJavaVM createjavavm
typedef jint (JNICALL *createjavavmtype)(JavaVM **, void **, void *);
static createjavavmtype JNI_CreateJavaVM = NULL;
static HINSTANCE jvmdll = NULL;

static int open_nt_dll(void)
{
  LPCTSTR libname=_T("jvm");
  LPCTSTR keyname=_T("SOFTWARE\\JavaSoft\\Java Runtime Environment");
  HKEY key;
  TCHAR buffer[2*MAX_PATH+32];
  DWORD type, len = sizeof(buffer)-16;
  DWORD l;
  HINSTANCE kernel;
  DWORD (*getdlldir)(DWORD nBufferLength, LPTSTR lpBuffer);
  BOOL (*setdlldir)(LPCTSTR lpPathname);
  
  l = GetEnvironmentVariable("PIKE_JRE_JVMDLL", buffer, len);
  if (l > 0) {
    libname = buffer;
    len = l;
  }
  else if(RegOpenKeyEx(HKEY_CURRENT_USER, keyname, 0,
                       KEY_READ, &key) == ERROR_SUCCESS ||
          RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0,
                       KEY_READ, &key) == ERROR_SUCCESS) {
    LPCTSTR subkeyname=_T("1.2");
    HKEY subkey;

    if(ERROR_SUCCESS == RegQueryValueEx(key, _T("CurrentVersion"), 0,
					&type, buffer, &len) &&
       type == REG_SZ)
      subkeyname = buffer;
    
    if(RegOpenKeyEx(key, subkeyname, 0, KEY_READ, &subkey) ==
       ERROR_SUCCESS) {
      
      len = sizeof(buffer)-16;

      if(ERROR_SUCCESS == RegQueryValueEx(subkey, _T("RuntimeLib"), 0,
					  &type, buffer, &len))
	switch(type) {
	 case REG_SZ:
	   libname = buffer;
	   break;
	 case REG_EXPAND_SZ:
	   {
	     LPTSTR subbuffer = buffer+((len+1)/sizeof(TCHAR));
	     l = ExpandEnvironmentStrings(buffer, subbuffer,
					     sizeof(buffer)-len-2);
	     if(l && l <= sizeof(buffer)-len-2) {
	       libname = subbuffer;
	       len = l;
	     }
	   }
	   break;
	}
      RegCloseKey(subkey);
    }
    RegCloseKey(key);
  }

  /* Java 6 and 7 jvm.dll have dependencies on msvcr71.dll and msvcr100.dll
   * respectively. They are located in the parent directory of the one
   * containing jvm.dll. cf http://www.duckware.com/tech/java6msvcr71.html
   * for details.
   *
   * We temporarily truncate the "C:\Program Files\Java\jre\bin\client\jvm.dll"
   * from the registry to "C:\Program Files\Java\jre\bin", and call
   * SetDllDirectory() with it.
   */
  kernel = GetModuleHandle("kernel32");
  getdlldir = (void *)GetModuleHandle(kernel, "GetDllDirectoryA");
  setdlldir = (void *)GetModuleHandle(kernel, "SetDllDirectoryA");

  if (setdlldir) {
    int cnt = 0;

    for(l = len; l--;) {
      if (jvmdll[l] == '\\') {
	/* Go up two directory levels. */
	if (++cnt == 2) break;
      }
    }
    if (cnt == 2) {
      TCHAR dlldirbuffer[2*MAX_PATH];
      LPTSTR origdlldir = NULL;

      libname[l] = 0;

      if (getdlldir) {
	/* Retain the dlldir load path.
	 *
	 * FIXME: This does not take into account the newer
	 *        AddDllDirectory() API, and will thus lose
	 *        directories from the dlldir load path if
	 *        it has been used.
	 */
	if ((len = getdlldir(2*MAX_PATH, dlldirbuffer))) {
	  dlldirbuffer[len] = 0;
	  origdlldir = dlldirbuffer;
	}
      }

      setdlldir(libname);

      /* Restore the zapped diretory separator. */
      libname[l] = '\\';

      jvmdll = LoadLibrary(libname);

      /* Restore the original dlldir load path. */
      setdlldir(origdlldir);
    } else {
      /* Not enough directory levels. */
      jvmdll = LoadLibrary(libname);
    }
  } else {
    /* No SetDllDirectory(). */
    jvmdll = LoadLibrary(libname);
  }

  if (!jvmdll) {
#ifndef CONFIGURE
    yywarning("Failed to load JVM: '%s'\n", libname);
#endif
    return -1;
  }
  else {
    FARPROC proc;
    if(proc=GetProcAddress(jvmdll, "JNI_CreateJavaVM"))
      JNI_CreateJavaVM = (createjavavmtype)proc;
    else {
      if(FreeLibrary(jvmdll))
	jvmdll = NULL;
#ifndef CONFIGURE
      yywarning("Failed to create JVM.\n");
#endif
      return -2;
    }
  }
  return 0;
}

static void close_nt_dll()
{
  if(jvmdll != NULL && FreeLibrary(jvmdll))
    jvmdll = NULL;
}
