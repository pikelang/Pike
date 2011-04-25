/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
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
  LPCTSTR libname=_T("jvm");
  LPCTSTR keyname=_T("SOFTWARE\\JavaSoft\\Java Runtime Environment");
  HKEY key;
  TCHAR buffer[2*MAX_PATH+32];
  DWORD type, len = sizeof(buffer)-16;
  DWORD l;
  
  l = GetEnvironmentVariable("PIKE_JRE_JVMDLL", buffer, len);
  if (l > 0) {
    libname = buffer;
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
	     type = ExpandEnvironmentStrings(buffer, subbuffer,
					     sizeof(buffer)-len-2);
	     if(type && type<=sizeof(buffer)-len-2)
	       libname = subbuffer;
	   }
	   break;
	}
      RegCloseKey(subkey);
    }
    RegCloseKey(key);
  }
  if((jvmdll=LoadLibrary(libname))==NULL)
    return -1;
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

static void close_nt_dll()
{
  if(jvmdll != NULL && FreeLibrary(jvmdll))
    jvmdll = NULL;
}
