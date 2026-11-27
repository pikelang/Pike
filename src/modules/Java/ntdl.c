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
#include <stdlib.h>
#include <wchar.h>

/* #define PIKE_DLL_DEBUG */

#define JNI_CreateJavaVM createjavavm
typedef jint (JNICALL *createjavavmtype)(JavaVM **, void **, void *);
static createjavavmtype JNI_CreateJavaVM = NULL;
static HINSTANCE jvmdll = NULL;

static WCHAR *read_jvm_registry_string(HKEY key, const WCHAR *name,
                                       DWORD *type)
{
  DWORD size = 0;
  LONG result;

  result = RegQueryValueExW(key, name, NULL, type, NULL, &size);
  while (result == ERROR_SUCCESS &&
         (*type == REG_SZ || *type == REG_EXPAND_SZ)) {
    DWORD length = size;
    WCHAR *value;

    if (size > (size_t)-1 - sizeof(WCHAR) ||
        !(value = malloc((size_t)size + sizeof(WCHAR))))
      return NULL;
    result = RegQueryValueExW(key, name, NULL, type, (LPBYTE)value, &length);
    if (result == ERROR_SUCCESS &&
        (*type == REG_SZ || *type == REG_EXPAND_SZ) &&
        !(length % sizeof(WCHAR))) {
      value[length / sizeof(WCHAR)] = 0;
      return value;
    }
    free(value);
    if (result != ERROR_MORE_DATA || length <= size) break;
    size = length;
    result = ERROR_SUCCESS;
  }
  return NULL;
}

static WCHAR *find_jvm(void)
{
  static const WCHAR env_name[] = L"PIKE_JRE_JVMDLL";
  static const WCHAR key_name[] = L"SOFTWARE\\JavaSoft\\Java Runtime Environment";
  DWORD size = GetEnvironmentVariableW(env_name, NULL, 0);
  HKEY root = HKEY_CURRENT_USER;
  int i;

  while (size) {
    WCHAR *path;
    DWORD length;

    if (size > (size_t)-1 / sizeof(WCHAR) ||
        !(path = malloc((size_t)size * sizeof(WCHAR))))
      return NULL;
    length = GetEnvironmentVariableW(env_name, path, size);
    if (length && length < size) return path;
    free(path);
    size = length;
  }

  for (i = 0; i < 2; i++, root = HKEY_LOCAL_MACHINE) {
    HKEY key, subkey;
    WCHAR *version, *path = NULL;
    DWORD type;

    if (RegOpenKeyExW(root, key_name, 0, KEY_READ, &key) != ERROR_SUCCESS)
      continue;
    version = read_jvm_registry_string(key, L"CurrentVersion", &type);
    if (version && type != REG_SZ) {
      free(version);
      version = NULL;
    }
    if ((version || i) &&
        RegOpenKeyExW(key, version ? version : L"1.2", 0, KEY_READ, &subkey)
        == ERROR_SUCCESS) {
      path = read_jvm_registry_string(subkey, L"RuntimeLib", &type);
      RegCloseKey(subkey);
    }
    free(version);
    RegCloseKey(key);

    if (!path) continue;
    if (type == REG_SZ) return path;

    size = ExpandEnvironmentStringsW(path, NULL, 0);
    while (size) {
      WCHAR *expanded;
      DWORD length;

      if (size > (size_t)-1 / sizeof(WCHAR) ||
          !(expanded = malloc((size_t)size * sizeof(WCHAR))))
        break;
      length = ExpandEnvironmentStringsW(path, expanded, size);
      if (length && length <= size) {
        free(path);
        return expanded;
      }
      free(expanded);
      size = length;
    }
    free(path);
  }
  return NULL;
}

static WCHAR *save_dll_directory(DWORD (WINAPI *getdir)(DWORD, WCHAR *),
                                 BOOL *saved)
{
  DWORD size;

  *saved = FALSE;
  SetLastError(ERROR_SUCCESS);
  size = getdir(0, NULL);
  while (size) {
    WCHAR *path;
    DWORD length;

    if (size > (size_t)-1 / sizeof(WCHAR) ||
        !(path = malloc((size_t)size * sizeof(WCHAR))))
      return NULL;
    SetLastError(ERROR_SUCCESS);
    length = getdir(size, path);
    if (length && length < size) {
      *saved = TRUE;
      return path;
    }
    free(path);
    size = length;
  }
  /* An unset DLL directory is also a valid state to restore. */
  *saved = GetLastError() == ERROR_SUCCESS;
  return NULL;
}

static int open_nt_dll(void)
{
  WCHAR *libname = find_jvm();
  HINSTANCE kernel;
  DWORD (WINAPI *getdlldir)(DWORD, WCHAR *) = NULL;
  BOOL (WINAPI *setdlldir)(const WCHAR *) = NULL;

#ifdef PIKE_DLL_DEBUG
  fwprintf(stderr, L"JVM libname: %ls\r\n", libname ? libname : L"jvm");
#endif

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

  {
    WCHAR *original = NULL;
    BOOL changed = FALSE;
    WCHAR *parent = NULL;

    if (libname && getdlldir && setdlldir) {
      WCHAR *separator = wcsrchr(libname, L'\\');

      if (separator) {
        *separator = 0;
        parent = wcsrchr(libname, L'\\');
        *separator = L'\\';
      }
    }
    if (parent) {
      BOOL saved;

      original = save_dll_directory(getdlldir, &saved);
      if (saved) {
        *parent = 0;
        changed = setdlldir(libname);
        *parent = L'\\';
      }
    }
    jvmdll = LoadLibraryW(libname ? libname : L"jvm");
    if (changed) setdlldir(original);
    free(original);
  }
  free(libname);

  if (!jvmdll) {
    return -1;
  }
  else {
    FARPROC proc;
    if((proc=GetProcAddress(jvmdll, "JNI_CreateJavaVM")))
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
