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

/* #define PIKE_DLL_DEBUG */

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
  DWORD type, len = sizeof(buffer)/sizeof(WCHAR);
  DWORD l;
  HINSTANCE kernel;
  DWORD (WINAPI *getdlldir)(DWORD nBufferLength, WCHAR *lpBuffer) = NULL;
  BOOL (WINAPI *setdlldir)(WCHAR *lpPathname) = NULL;

  l = GetEnvironmentVariableW(L"PIKE_JRE_JVMDLL", buffer, len);
  if (l > 0) {
    len = l;
  } else {
    HKEY root = HKEY_CURRENT_USER;
    int i;
    for (i = 0; i < 2; i++, root = HKEY_LOCAL_MACHINE) {
      WCHAR *subkeyname = L"1.2";
      HKEY subkey;

      if (RegOpenKeyExW(root, keyname, 0, KEY_READ, &key) != ERROR_SUCCESS)
	continue;

      len = MAX_PATH * sizeof(WCHAR);
      if ((RegQueryValueExW(key, L"CurrentVersion", 0,
                            &type, (LPBYTE)buffer, &len) == ERROR_SUCCESS) &&
	  type == REG_SZ) {
	subkeyname = buffer;
      } else {
	/* NB: The key exists, but is empty or similar (eg remnants
	 *     from an old installation). Try the next root key.
	 */
	if (!i) {
	  RegCloseKey(key);
	  continue;
	}
	/* Use the default ("1.2" above). */
      }

#ifdef PIKE_DLL_DEBUG
      fwprintf(stderr, L"JVM subkeyname: %s\r\n", subkeyname);
#endif /* PIKE_DLL_DEBUG */

      if(RegOpenKeyExW(key, subkeyname, 0, KEY_READ, &subkey) ==
	 ERROR_SUCCESS) {

        len = MAX_PATH * sizeof(WCHAR);

	if(ERROR_SUCCESS == RegQueryValueExW(subkey, L"RuntimeLib", 0,
                                             &type, (LPBYTE)buffer, &len)) {
          RegCloseKey(subkey);
          RegCloseKey(key);

	  switch(type) {
	  case REG_SZ:
	    len /= sizeof(WCHAR);
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
          default:
            /* Unsupported value. */
            continue;
	  }
	  /* Success! */
	  break;
#ifdef PIKE_DLL_DEBUG
	} else {
	  fwprintf(stderr, L"JVM: No RuntimeLib!\r\n");
#endif /* PIKE_DLL_DEBUG */
	}
	RegCloseKey(subkey);
#ifdef PIKE_DLL_DEBUG
      } else {
	fwprintf(stderr, L"JVM: No subkey!\r\n");
#endif /* PIKE_DLL_DEBUG */
      }
      RegCloseKey(key);
      len = wcslen(buffer);
    }
  }

#ifdef PIKE_DLL_DEBUG
  fwprintf(stderr, L"JVM libname: %s\r\n", libname?libname:L"NULL");
#endif /* PIKE_DLL_DEBUG */

  if (!libname) {
    /* JVM not found. */
    return -1;
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

#ifdef PIKE_DLL_DEBUG
      fwprintf(stderr, L"JVM dlldir: %s\r\n", libname);
#endif /* PIKE_DLL_DEBUG */

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
#ifdef PIKE_DLL_DEBUG
    fwprintf(stderr, L"JVM Warning: no SetDllDirectory\r\n");
#endif /* PIKE_DLL_DEBUG */

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
