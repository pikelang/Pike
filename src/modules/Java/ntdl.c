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

#define JNI_CreateJavaVM createjavavm
typedef jint (JNICALL *createjavavmtype)(JavaVM **, void **, void *);
static createjavavmtype JNI_CreateJavaVM = NULL;
static HINSTANCE jvmdll = NULL;

static int open_nt_dll(void)
{
  WCHAR buffer[2*MAX_PATH+32] = L"jvm";
  WCHAR keyname[] = L"SOFTWARE\\JavaSoft\\Java Runtime Environment";
  HKEY key;
  WCHAR *libname = buffer;
  DWORD type, len = sizeof(buffer)/2;
  DWORD l;
  HINSTANCE kernel;
  DWORD (WINAPI *getdlldir)(DWORD nBufferLength, WCHAR *lpBuffer) = NULL;
  BOOL (WINAPI *setdlldir)(WCHAR *lpPathname) = NULL;
  
  l = GetEnvironmentVariableW(L"PIKE_JRE_JVMDLL", buffer, len);
  if (l > 0) {
    len = l;
  }
  else if(RegOpenKeyExW(HKEY_CURRENT_USER, keyname, 0,
			KEY_READ, &key) == ERROR_SUCCESS ||
          RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyname, 0,
			KEY_READ, &key) == ERROR_SUCCESS) {
    WCHAR *subkeyname = L"1.2";
    HKEY subkey;

    if(ERROR_SUCCESS == RegQueryValueExW(key, L"CurrentVersion", 0,
					 &type, buffer, &len) &&
       type == REG_SZ) {
      subkeyname = buffer;
    }

    if(RegOpenKeyExW(key, subkeyname, 0, KEY_READ, &subkey) ==
       ERROR_SUCCESS) {

      len = sizeof(buffer)-16;

      if(ERROR_SUCCESS == RegQueryValueExW(subkey, L"RuntimeLib", 0,
					   &type, buffer, &len))
	switch(type) {
	 case REG_SZ:
	   len /= sizeof(*buffer);
	   break;
	 case REG_EXPAND_SZ:
	   {
	     WCHAR *subbuffer = buffer + MAX_PATH + 16;
	     l = ExpandEnvironmentStringsW(buffer, subbuffer, MAX_PATH + 16);
	     if(l && l <= MAX_PATH + 16) {
	       libname = subbuffer;
	       len = l;
	     }
	   }
	   break;
	}
      RegCloseKey(subkey);
    }
    RegCloseKey(key);
  } else {
    len = wcslen(buffer);
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
  kernel = GetModuleHandle(_T("kernel32"));
  getdlldir = (void *)GetProcAddress(kernel, "GetDllDirectoryW");
  setdlldir = (void *)GetProcAddress(kernel, "SetDllDirectoryW");

  if (!setdlldir) {
    /* Fallback to {Get,Set}CurrentDirectoryW(). */
    getdlldir = GetCurrentDirectoryW;
    setdlldir = SetCurrentDirectoryW;
  }

  if (setdlldir) {
    int cnt = 0;

    for(l = len; l--;) {
      if (libname[l] == '\\') {
	/* Go up two directory levels. */
	if (++cnt == 2) break;
      }
    }
    if (cnt == 2) {
      WCHAR dlldirbuffer[MAX_PATH + 16];
      WCHAR *origdlldir = NULL;

      libname[l] = 0;

      if (getdlldir) {
	/* Retain the dlldir load path.
	 *
	 * FIXME: This does not take into account the newer
	 *        AddDllDirectory() API, and will thus lose
	 *        directories from the dlldir load path if
	 *        it has been used.
	 */
	if ((len = getdlldir(MAX_PATH + 16, dlldirbuffer))) {
	  dlldirbuffer[len] = 0;
	  origdlldir = dlldirbuffer;
	}
      }

      setdlldir(libname);

      /* Restore the zapped diretory separator. */
      libname[l] = '\\';

      jvmdll = LoadLibraryW(libname);

      /* Restore the original dlldir load path. */
      setdlldir(origdlldir);
    } else {
      /* Not enough directory levels. */
      jvmdll = LoadLibraryW(libname);
    }
  } else {
    /* No SetDllDirectory(). */
    jvmdll = LoadLibraryW(libname);
  }

  if (!jvmdll) {
    return -1;
  }
  else {
    FARPROC proc;
    if(proc=GetProcAddress(jvmdll, "JNI_CreateJavaVM"))
      JNI_CreateJavaVM = (createjavavmtype)proc;
    else {
      if(FreeLibrary(jvmdll))
	jvmdll = NULL;
      return -2;
    }
  }
  return 0;
}

static void close_nt_dll(void)
{
  if(jvmdll != NULL && FreeLibrary(jvmdll))
    jvmdll = NULL;
}
