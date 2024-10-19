/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
*/

/*
 * NT system calls for Pike
 *
 * Fredrik Hubinette, 1998-08-22
 */

#include "global.h"

#include "system_machine.h"
#include "system.h"

#include <errno.h>

#ifdef __NT__
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#else
#include <winsock.h>
#endif
#include <accctrl.h>
#include <lm.h>
#define SECURITY_WIN32
#define SEC_SUCCESS(Status) ((Status) >= 0)
#ifndef __MINGW32__
#include <sspi.h>
#else
#include <security.h>
#endif

/* These are defined by winerror.h in recent SDKs. */
#ifndef SEC_E_INSUFFICIENT_MEMORY
#include <issperr.h>
#endif

/*
 * Get some wrappers for functions not implemented in old versions
 * of WIN32. Needs a Platform SDK installed. The SDK included in
 * MSVS 6.0 is not enough.
 */
#ifdef HAVE_NEWAPIS_H
#define COMPILE_NEWAPIS_STUBS
/* We want GetLongPathName()... */
#define WANT_GETLONGPATHNAME_WRAPPER
#include <NewAPIs.h>
#endif

#include "program.h"
#include "stralloc.h"
#include "threads.h"
#include "module_support.h"
#include "array.h"
#include "mapping.h"
#include "constants.h"
#include "builtin_functions.h"
#include "interpret.h"
#include "operators.h"
#include "fdlib.h"
#include "bignum.h"

#define sp Pike_sp

static void throw_nt_error(int err)
/*
 *  Give string equivalents to some of the more common NT error codes.
 */
{
  char *msg;

  switch (err)
  {
    case ERROR_FILE_NOT_FOUND:
      msg = "File not found.";
      break;

    case ERROR_PATH_NOT_FOUND:
      msg = "Path not found.";
      break;

    case ERROR_INVALID_DATA:
      msg = "Invalid data.";
      break;

    case ERROR_ACCESS_DENIED:
      msg = "Access denied.";
      break;

    case ERROR_LOGON_FAILURE:
      msg = "Logon failure (access denied).";
      break;

    case ERROR_INVALID_PARAMETER:
      msg = "Invalid parameter.";
      break;

    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUTOFMEMORY:
      msg = "Out of memory.";
      break;

    case ERROR_INVALID_NAME:
      msg = "Invalid name.";
      break;

    case ERROR_INVALID_LEVEL:
      msg = "Invalid level.";
      break;

    case ERROR_NO_SUCH_ALIAS:
      msg = "No such alias.";
      break;

    case ERROR_NO_SUCH_DOMAIN:
      msg = "No such domain.";
      break;

    case ERROR_NO_LOGON_SERVERS:
      msg = "No log-on servers.";
      break;

    case ERROR_BAD_NETPATH:
      msg = "Network path not found.";
      break;

    case ERROR_BAD_NET_NAME:
      msg = "The network name cannot be found.";
      break;

    case ERROR_NO_TRUST_LSA_SECRET:
      msg = "No trust LSA secret (client not trusted).";
      break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
      msg = "No trust SAM account (server/password not trusted).";
      break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
      msg = "Domain trust inconsistent.";
      break;

    case NERR_DCNotFound:
      msg = "Domain controller not found.";
      break;

    case NERR_InvalidComputer:
      msg = "Invalid computer.";
      break;

    case NERR_UserNotFound:
      msg = "User not found.";
      break;

    case NERR_GroupNotFound:
      msg = "Group not found.";
      break;

    case NERR_ClientNameNotFound:
      msg = "Client name not found.";
      break;

    case RPC_S_SERVER_UNAVAILABLE:
      msg = "RPC server unavailable.";
      break;

    case NERR_Success:
      msg = "Strange error (NERR_Success).";
      break;

    default:
      Pike_error("Unknown error 0x%04x (%d)\n", err, err);
      return;
  }
  Pike_error("%s\n", msg);
}

static void f_cp(INT32 args)
{
  char *from, *to;
  int ret;
  get_all_args(NULL, args, "%c%c", &from, &to);
  ret=CopyFile(from, to, 0);
  if(!ret) set_errno_from_win32_error (GetLastError());
  pop_n_elems(args);
  push_int(ret);
}

/*! @module System
 */

static void push_tchar(const TCHAR *buf, DWORD len)
{
  push_string(make_shared_binary_pcharp(
    MKPCHARP(buf,my_log2(sizeof(TCHAR))),len));
}

static void push_regvalue(DWORD type, char* buffer, DWORD len, int no_expand)
{
  struct pike_string *str;
  switch(type)
  {
    case REG_RESOURCE_LIST:
    case REG_NONE:
    case REG_LINK:
    case REG_BINARY:
      push_string(make_shared_binary_string(buffer,len));
      break;

    case REG_SZ:
      if (!len) {
	push_empty_string();
      } else {
      push_reg_sz:
        str = begin_wide_shared_string((len>>1) - 1, sixteenbit);
        str->flags |= STRING_CONVERT_SURROGATES;
        memcpy(str->str, buffer, len);
        push_string(end_shared_string(str));
      }
      break;

    case REG_EXPAND_SZ:
      if (!len) {
        push_empty_string();
      } else {
        if (no_expand) {
          goto push_reg_sz;
        }
        str = begin_wide_shared_string(1024, sixteenbit);
        str->flags |= STRING_CONVERT_SURROGATES;
        len =
          ExpandEnvironmentStringsW((p_wchar1 *)buffer, STR1(str), str->len);
        /* NB: len includes the terminating NUL. */
        if ((len >= (DWORD)str->len) || !len) {
          free_string(str);
          Pike_error("Failed to expand data.\n");
        }
        push_string(end_and_resize_shared_string(str, len-1));
      }
      break;

    case REG_MULTI_SZ:
      if (!len) {
        push_empty_array();
      } else {
        str = begin_wide_shared_string((len>>1) - 1, sixteenbit);
        str->flags |= STRING_CONVERT_SURROGATES;
        memcpy(str->str, buffer, len);
        push_string(end_shared_string(str));
	push_string(make_shared_binary_string("\000",1));
	f_divide(2);
      }
      break;

#if PIKE_BYTEORDER == 1234
    case REG_DWORD_LITTLE_ENDIAN:
#else
    case REG_DWORD_BIG_ENDIAN:
#endif
      push_int(*((INT32 *)buffer));
      break;

#if PIKE_BYTEORDER == 1234
    case REG_DWORD_BIG_ENDIAN:
#else
    case REG_DWORD_LITTLE_ENDIAN:
#endif
      {
        char buf2[4] = { buffer[3], buffer[2], buffer[1], buffer[0] };
        push_int(*((INT32 *)buf2));
      }
      break;

#ifdef REG_QWORD_LITTLE_ENDIAN
    case REG_QWORD_LITTLE_ENDIAN:
#if PIKE_BYTEORDER == 1234
      push_int64(*((INT64 *)buffer));
#else
      {
        char buf2[8] = { buffer[7], buffer[6], buffer[5], buffer[4],
                         buffer[3], buffer[2], buffer[1], buffer[0] };
        push_int(*((INT64 *)buf2));
      }
#endif
      break;
#endif

    default:
      Pike_error("Cannot handle this data type.\n");
  }
}

/* Known hkeys.
 *
 * This table is used to avoid passing pointers to the pike level.
 *
 * NOTE: Order must match the values specified with
 * ADD_GLOBAL_INTEGER_CONSTANT() in init_nt_system_calls() below.
 */
static const HKEY hkeys[] = {
  HKEY_CLASSES_ROOT,
  HKEY_LOCAL_MACHINE,
  HKEY_CURRENT_USER,
  HKEY_USERS,
  /* HKEY_CURRENT_CONFIG */
};

/*! @decl constant HKEY_CLASSES_ROOT = 0
 *! @decl constant HKEY_LOCAL_MACHINE = 1
 *! @decl constant HKEY_CURRENT_USER = 2
 *! @decl constant HKEY_USERS = 3
 *!
 *!   Root handles in the Windows registry.
 *!
 *! @note
 *!   These constants are only available on Win32 systems.
 *!
 *! @seealso
 *!   @[RegGetValue()], @[RegGetValues()], @[RegGetKeyNames()]
 */

static void do_regclosekey(HKEY key)
{
  RegCloseKey(key);
}

/*! @decl string|int|array(string) RegGetValue(int hkey, string key, @
 *!                                            string index, @
 *!                                            int(0..1)|void no_expand)
 *!
 *!   Get a single value from the register.
 *!
 *! @param hkey
 *!   One of the following:
 *!   @int
 *!     @value HKEY_CLASSES_ROOT
 *!     @value HKEY_LOCAL_MACHINE
 *!     @value HKEY_CURRENT_USER
 *!     @value HKEY_USERS
 *!   @endint
 *!
 *! @param key
 *!   Registry key.
 *!
 *! @param index
 *!   Value name.
 *!
 *! @param no_expand
 *!   Set this to @expr{1@} to not expand variables in
 *!   @expr{REG_EXPAND_SZ@}-values.
 *!
 *! @returns
 *!   Returns the value stored at the specified location in the register
 *!   if any. Returns @expr{UNDEFINED@} on missing keys, throws errors
 *!   on other failures.
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @note
 *!   Prior to Pike 9.0 @expr{REG_EXPAND_SZ@}-values were always expanded.
 *!
 *! @seealso
 *!   @[RegGetValues()], @[RegGetKeyNames()]
 */
void f_RegGetValue(INT32 args)
{
  long ret;
  INT_TYPE hkey_num, no_expand = 0;
  HKEY new_key;
  struct pike_string *key, *ind;
  ONERROR tmp;
  p_wchar1 *utf16;
  DWORD len,type;
  char buffer[8192];
  len=sizeof(buffer)-1;

  get_all_args(NULL, args, "%i%t%t.%i", &hkey_num, &key, &ind, &no_expand);

  if ((hkey_num < 0) || ((unsigned int)hkey_num >= NELEM(hkeys))) {
    Pike_error("Unknown hkey: %d.\n", hkey_num);
  }

  utf16 = pike_string_to_utf16(key, 1);
  ret = RegOpenKeyExW(hkeys[hkey_num], utf16, 0, KEY_READ, &new_key);
  free(utf16);

  if ((ret == ERROR_FILE_NOT_FOUND) ||
      (ret == ERROR_PATH_NOT_FOUND)) {
    pop_n_elems(args);
    push_undefined();
    return;
  }
  if(ret != ERROR_SUCCESS)
    throw_nt_error(ret);

  SET_ONERROR(tmp, do_regclosekey, new_key);

  utf16 = pike_string_to_utf16(ind, 1);
  ret = RegQueryValueExW(new_key, utf16, 0, &type, buffer, &len);
  free(utf16);

  CALL_AND_UNSET_ONERROR(tmp);

  if(ret==ERROR_SUCCESS)
  {
    pop_n_elems(args);
    push_regvalue(type, buffer, len, no_expand);
  } else if (ret == ERROR_FILE_NOT_FOUND) {
    pop_n_elems(args);
    push_undefined();
  }else{
    throw_nt_error(ret);
  }
}

/*! @decl array(string) RegGetKeyNames(int hkey, string key)
 *!
 *!   Get a list of value key names from the register.
 *!
 *! @param hkey
 *!   One of the following:
 *!   @int
 *!     @value HKEY_CLASSES_ROOT
 *!     @value HKEY_LOCAL_MACHINE
 *!     @value HKEY_CURRENT_USER
 *!     @value HKEY_USERS
 *!   @endint
 *!
 *! @param key
 *!   A registry key.
 *!
 *! @returns
 *!   Returns an array of value keys stored at the specified location if any.
 *!   Returns @expr{UNDEFINED@} on missing @[key].
 *!   Throws errors on other failures.
 *!
 *! @example
 *!   > RegGetKeyNames(HKEY_CURRENT_USER, "Keyboard Layout");
 *!  (1) Result: ({
 *!    "IMEtoggle",
 *!    "Preload",
 *!    "Substitutes",
 *!    "Toggle"
 *!   })
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @seealso
 *!   @[RegGetValue()], @[RegGetValues()]
 */
void f_RegGetKeyNames(INT32 args)
{
  INT_TYPE hkey_num;
  struct pike_string *key;
  p_wchar1 *key_utf16;
  int i,ret;
  HKEY new_key;
  ONERROR tmp;
  get_all_args(NULL, args, "%i%t", &hkey_num, &key);

  if ((hkey_num < 0) || ((unsigned int)hkey_num >= NELEM(hkeys))) {
    Pike_error("Unknown hkey: %d.\n", hkey_num);
  }

  key_utf16 = pike_string_to_utf16(key, 1);
  ret = RegOpenKeyExW(hkeys[hkey_num], key_utf16, 0, KEY_READ, &new_key);
  free(key_utf16);

  if ((ret == ERROR_FILE_NOT_FOUND) ||
      (ret == ERROR_PATH_NOT_FOUND)) {
    pop_n_elems(args);
    push_undefined();
    return;
  }
  if(ret != ERROR_SUCCESS)
    throw_nt_error(ret);

  SET_ONERROR(tmp, do_regclosekey, new_key);

  pop_n_elems(args);

  for(i=0;;i++)
  {
    struct pike_string *buf = begin_wide_shared_string(1024, sixteenbit);
    DWORD len = buf->len;
    FILETIME tmp2;

    buf->flags |= STRING_CONVERT_SURROGATES;

    THREADS_ALLOW();
    ret = RegEnumKeyExW(new_key, i, STR1(buf), &len, 0, 0, 0, &tmp2);
    THREADS_DISALLOW();
    switch(ret)
    {
      case ERROR_SUCCESS:
	check_stack(1);
        push_string(end_and_resize_shared_string(buf, len));
	continue;

      case ERROR_NO_MORE_ITEMS:
	break;

      default:
        free_string(buf);
        throw_nt_error(ret);
    }
    free_string(buf);
    break;
  }
  CALL_AND_UNSET_ONERROR(tmp);
  f_aggregate(i);
}

/*! @decl mapping(string:string|int|array(string)) RegGetValues(int hkey, @
 *!                                                             string key, @
 *!                                                 int(0..1)|void no_expand)
 *!
 *!   Get multiple values from the register.
 *!
 *! @param hkey
 *!   One of the following:
 *!   @int
 *!     @value HKEY_CLASSES_ROOT
 *!     @value HKEY_LOCAL_MACHINE
 *!     @value HKEY_CURRENT_USER
 *!     @value HKEY_USERS
 *!   @endint
 *!
 *! @param key
 *!   Registry key.
 *!
 *! @param no_expand
 *!   Set this to @expr{1@} to not expand variables in
 *!   @expr{REG_EXPAND_SZ@}-values.
 *!
 *! @returns
 *!   Returns a mapping with all the values stored at the specified location
 *!   in the register if any.
 *!   Returns @expr{UNDEFINED@} on missing @[key].
 *!   Throws errors on other failures.
 *!
 *! @example
 *! > RegGetValues(HKEY_CURRENT_USER, "Keyboard Layout\\Preload");
 *!(5) Result: ([
 *!  "1":"0000041d"
 *! ])
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @note
 *!   Prior to Pike 9.0 @expr{REG_EXPAND_SZ@}-values were always expanded.
 *!
 *! @seealso
 *!   @[RegGetValue()], @[RegGetKeyNames()]
 */
void f_RegGetValues(INT32 args)
{
  INT_TYPE hkey_num, no_expand = 0;
  struct pike_string *key;
  p_wchar1 *key_utf16;
  int i,ret;
  HKEY new_key;
  ONERROR tmp;

  get_all_args(NULL, args, "%i%t.%i", &hkey_num, &key, &no_expand);

  if ((hkey_num < 0) || ((unsigned int)hkey_num >= NELEM(hkeys))) {
    Pike_error("Unknown hkey: %d.\n", hkey_num);
  }

  key_utf16 = pike_string_to_utf16(key, 1);
  ret = RegOpenKeyExW(hkeys[hkey_num], key_utf16, 0, KEY_READ, &new_key);
  free(key_utf16);

  if ((ret == ERROR_FILE_NOT_FOUND) ||
      (ret == ERROR_PATH_NOT_FOUND)) {
    pop_n_elems(args);
    push_undefined();
    return;
  }
  if(ret != ERROR_SUCCESS)
    throw_nt_error(ret);

  SET_ONERROR(tmp, do_regclosekey, new_key);
  pop_n_elems(args);

  for(i=0;;i++)
  {
    struct pike_string *buf = begin_wide_shared_string(1024, sixteenbit);
    char buffer[8192];
    DWORD len = buf->len;
    DWORD buflen=sizeof(buffer)-1;
    DWORD type;

    buf->flags |= STRING_CONVERT_SURROGATES;

    THREADS_ALLOW();
    ret = RegEnumValueW(new_key, i, STR1(buf), &len, 0, &type, buffer, &buflen);
    THREADS_DISALLOW();
    switch(ret)
    {
      case ERROR_SUCCESS:
	check_stack(2);
        push_string(end_and_resize_shared_string(buf, len));
        push_regvalue(type, buffer, buflen, no_expand);
	continue;

      case ERROR_NO_MORE_ITEMS:
	break;

      default:
        free_string(buf);
	RegCloseKey(new_key);
        throw_nt_error(ret);
    }
    free_string(buf);
    break;
  }
  CALL_AND_UNSET_ONERROR(tmp);
  f_aggregate_mapping(i*2);
}

/*! @decl int FreeConsole()
 *!
 *! Detaches the calling process from its console.
 *!
 *! @note
 *!  Before calling this function, @[Stdio.stderr], @[Stdio.stdout] and
 *!  @[Stdio.stdin] must be closed.
 *!
 *! @note
 *!  Only available on certain Windows systems.
 *!
 *! @returns
 *!   0 on success, non-zero otherwise.
 */
#ifdef HAVE_FREECONSOLE
void f_freeconsole(INT32 args)
{
  int rv;

  rv = (int)FreeConsole();

  push_int(rv);
}
#endif /* HAVE_FREECONSOLE */

/*! @decl int AllocConsole()
 *!
 *! Allocates a new console for the calling process.
 *!
 *! @note
 *!  Only available on certain Windows systems.
 *!
 *! @returns
 *!   0 on success, non-zero otherwise.
 */
#ifdef HAVE_ALLOCCONSOLE
void f_allocconsole(INT32 args)
{
  int rv;

  rv = (int)AllocConsole();

  push_int(rv);
}
#endif /* HAVE_ALOCCONSOLE */

/*! @decl int AttachConsole(int pid)
 *!
 *! Attaches calling process to a specific console.
 *!
 *! @param pid
 *   The identifier of the process whose console is to be used.
 *!
 *! @note
 *!  Only available on certain Windows systems.
 *!
 *! @returns
 *!   0 on success, non-zero otherwise.
 */
#ifdef HAVE_ATTACHCONSOLE
void f_attachconsole(INT32 args)
{
  int rv;
  int pid;
  get_all_args(NULL, args, "%d", &pid);

  rv = (int)AttachConsole(pid);

  push_int(rv);
}
#endif /* HAVE_ATTACHCONSOLE */

static struct program *token_program;

#define THIS_TOKEN (*(HANDLE *)CURRENT_STORAGE)

typedef BOOL (WINAPI *logonusertype)(LPCWSTR, LPCWSTR, LPCWSTR,
                                     DWORD, DWORD, PHANDLE);
typedef DWORD (WINAPI *getlengthsidtype)(PSID);

static logonusertype logonuser;
static getlengthsidtype getlengthsid;

#define LINKFUNC(RET,NAME,TYPE) \
  typedef RET (WINAPI * PIKE_CONCAT(NAME,type)) TYPE ; \
  static PIKE_CONCAT(NAME,type) NAME

LINKFUNC(BOOL,equalsid, (PSID,PSID) );
LINKFUNC(BOOL,lookupaccountname,
         (LPCWSTR,LPCWSTR,PSID,LPDWORD,LPWSTR,LPDWORD,PSID_NAME_USE) );
LINKFUNC(BOOL,lookupaccountsid,
         (LPCWSTR,PSID,LPWSTR,LPDWORD,LPWSTR,LPDWORD,PSID_NAME_USE) );
LINKFUNC(BOOL,setnamedsecurityinfo,
         (LPWSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID,PSID,PACL,PACL) );
LINKFUNC(DWORD,getnamedsecurityinfo,
         (LPWSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID*,PSID*,PACL*,PACL*,PSECURITY_DESCRIPTOR*) );

LINKFUNC(BOOL,initializeacl, (PACL,DWORD,DWORD) );
LINKFUNC(BOOL,addaccessallowedace, (PACL,DWORD,DWORD,PSID) );
LINKFUNC(BOOL,addaccessdeniedace, (PACL,DWORD,DWORD,PSID) );
LINKFUNC(BOOL,addauditaccessace, (PACL,DWORD,DWORD,PSID,BOOL,BOOL) );

HINSTANCE advapilib;

/*! @class SID
 *!
 *! Security Identifier.
 *!
 *! Objects of this class are returned by @[LookupAccountName()].
 *!
 *! @seealso
 *!   @[LookupAccountName()]
 */
#define THIS_PSID (*(PSID *)CURRENT_STORAGE)
static struct program *sid_program;
static void init_sid(struct object *o)
{
  THIS_PSID=0;
}

static void exit_sid(struct object *o)
{
  if(THIS_PSID)
  {
    free(THIS_PSID);
    THIS_PSID=0;
  }
}

/*! @decl protected int(0..1) `==(mixed x)
 */
static void f_sid_eq(INT32 args)
{
  if(args && TYPEOF(sp[-1]) == T_OBJECT)
  {
    PSID *tmp=(PSID *)get_storage(sp[-1].u.object,sid_program);
    if(tmp)
    {
      if( (!THIS_PSID && !*tmp) ||
	  (THIS_PSID && *tmp && equalsid(THIS_PSID, *tmp) ))
      {
	pop_stack();
	push_int(1);
	return;
      }
    }
  }
  pop_stack();
  push_int(0);
}

/*! @decl array(string|int) account(string|void sys)
 *!
 *! @returns
 *!   Returns an array with the following content:
 *!   @array
 *!     @elem string 0
 *!       Account name.
 *!     @elem string 1
 *!       Domain name.
 *!     @elem int(0..) 2
 *!       Account type.
 *!   @endarray
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 */
static void f_sid_account(INT32 args)
{
  p_wchar1 foo[1];
  DWORD namelen=0;
  DWORD domainlen=0;
  p_wchar1 *sys = 0;
  SID_NAME_USE type;

  check_all_args(NULL, args, BIT_STRING|BIT_VOID, 0);
  if(args && (TYPEOF(sp[-args]) == T_STRING)) {
    ref_push_string(sp[-args].u.string);
    push_int(2);
    f_string_to_unicode(2);
    args++;
    sys = (p_wchar1 *)STR0(sp[-1].u.string);
  }

  if(!THIS_PSID) Pike_error("SID->account on uninitialized SID.\n");
  lookupaccountsid(sys,
		   THIS_PSID,
		   foo,
		   &namelen,
		   foo,
		   &domainlen,
		   &type);

  if(namelen && domainlen)
  {
    struct pike_string *dom = begin_wide_shared_string(domainlen-1, sixteenbit);
    struct pike_string *name = begin_wide_shared_string(namelen-1, sixteenbit);
    dom->flags |= STRING_CONVERT_SURROGATES;
    name->flags |= STRING_CONVERT_SURROGATES;

    if (lookupaccountsid(sys,
			 THIS_PSID,
                         STR1(name),
			 &namelen,
                         STR1(dom),
			 &domainlen,
			 &type))
    {
      push_string(end_shared_string(name));
      push_string(end_shared_string(dom));
      push_int(type);
      f_aggregate(3);
      return;
    }
    free(dom);
    free(name);
  }
  errno=GetLastError();
  pop_n_elems(args);
  push_array(allocate_array(3));

}

/*! @endclass
 */

/*! @class UserToken
 */

/*! @endclass
 */

/*! @decl UserToken LogonUser(string username, string|zero domain, @
 *!                           string password, int|void logon_type, @
 *!                           int|void logon_provider)
 *!
 *!   Logon a user.
 *!
 *! @param username
 *!   User name of the user to login.
 *!
 *! @param domain
 *!   Domain to login on, or zero if local logon.
 *!
 *! @param password
 *!   Password to login with.
 *!
 *! @param logon_type
 *!   One of the following values:
 *!   @int
 *!     @value LOGON32_LOGON_BATCH
 *!     @value LOGON32_LOGON_INTERACTIVE
 *!     @value LOGON32_LOGON_SERVICE
 *!
 *!     @value LOGON32_LOGON_NETWORK
 *!       This is the default.
 *!   @endint
 *!
 *! @param logon_provider
 *!   One of the following values:
 *!   @int
 *!     @value LOGON32_PROVIDER_DEFAULT
 *!       This is the default.
 *!   @endint
 *!
 *! @returns
 *!   Returns a login @[UserToken] object on success, and zero on failure.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 */
void f_LogonUser(INT32 args)
{
  struct pike_string *username_str = NULL, *domain_str = NULL, *pw_str = NULL;
  LPCWSTR username, domain = NULL, pw;
  int logontype = LOGON32_LOGON_NETWORK;
  int logonprovider = LOGON32_PROVIDER_DEFAULT;
  HANDLE x;
  BOOL ret;

  get_all_args(NULL, args, "%t%T%t.%d%d",
               &username_str, &domain_str, &pw_str,
               &logontype, &logonprovider);

  ref_push_string(username_str);
  push_int(2);
  f_string_to_unicode(2);
  username = (LPCWSTR)STR0(sp[-1].u.string);
  args++;

  if (domain_str) {
    ref_push_string(domain_str);
    push_int(2);
    f_string_to_unicode(2);
    domain = (LPCWSTR)STR0(sp[-1].u.string);
    args++;
  }

  ref_push_string(pw_str);
  push_int(2);
  f_string_to_unicode(2);
  pw = (LPCWSTR)STR0(sp[-1].u.string);
  args++;

  THREADS_ALLOW();
  ret=logonuser(username, domain, pw, logontype, logonprovider, &x);
  THREADS_DISALLOW();
  pop_n_elems(args);
  if(ret)
  {
    struct object *o;
    o=fast_clone_object(token_program);
    (*(HANDLE *)(o->storage))=x;
    push_object(o);
  }else{
    errno=GetLastError();
    push_int(0);
  }
}

static void init_token(struct object *o)
{
  THIS_TOKEN = INVALID_HANDLE_VALUE;
}

static void exit_token(struct object *o)
{
  CloseHandle(THIS_TOKEN);
  THIS_TOKEN = INVALID_HANDLE_VALUE;
}

static void low_encode_user_info_0(USER_INFO_0 *tmp)
{
#define SAFE_PUSH_WSTR(X) \
  if(X) \
    push_string(make_shared_string1((p_wchar1 *) X)); \
  else \
    push_int(0)

  SAFE_PUSH_WSTR(tmp->usri0_name);
}

static void low_encode_user_info_1(USER_INFO_1 *tmp)
{
  low_encode_user_info_0((USER_INFO_0 *)tmp);
  SAFE_PUSH_WSTR(tmp->usri1_password);
  push_int(tmp->usri1_password_age);
  push_int(tmp->usri1_priv);
  SAFE_PUSH_WSTR(tmp->usri1_home_dir);
  SAFE_PUSH_WSTR(tmp->usri1_comment);
  push_int(tmp->usri1_flags);
  SAFE_PUSH_WSTR(tmp->usri1_script_path);
  /* 8 entries */
}

static void low_encode_user_info_2(USER_INFO_2 *tmp)
{
  low_encode_user_info_1((USER_INFO_1 *)tmp);
  push_int(tmp->usri2_auth_flags);
  SAFE_PUSH_WSTR(tmp->usri2_full_name);
  SAFE_PUSH_WSTR(tmp->usri2_usr_comment);
  SAFE_PUSH_WSTR(tmp->usri2_parms);
  SAFE_PUSH_WSTR(tmp->usri2_workstations);
  push_int(tmp->usri2_last_logon);
  push_int(tmp->usri2_last_logoff);
  push_int(tmp->usri2_acct_expires);
  push_int(tmp->usri2_max_storage);
  push_int(tmp->usri2_units_per_week);

  if(tmp->usri2_logon_hours)
   push_string(make_shared_binary_string(tmp->usri2_logon_hours,21));
  else
   push_int(0);

  push_int(tmp->usri2_bad_pw_count);
  push_int(tmp->usri2_num_logons);
  SAFE_PUSH_WSTR(tmp->usri2_logon_server);
  push_int(tmp->usri2_country_code);
  push_int(tmp->usri2_code_page);
  /* 24 entries */
}

static void low_encode_user_info_3(USER_INFO_3 *tmp)
{
  low_encode_user_info_2((USER_INFO_2 *)tmp);
  push_int(tmp->usri3_user_id);
  push_int(tmp->usri3_primary_group_id);
  SAFE_PUSH_WSTR(tmp->usri3_profile);
  SAFE_PUSH_WSTR(tmp->usri3_home_dir_drive);
  push_int(tmp->usri3_password_expired);
  /* 29 entries */
}

static void low_encode_user_info_10(USER_INFO_10 *tmp)
{
  SAFE_PUSH_WSTR(tmp->usri10_name);
  SAFE_PUSH_WSTR(tmp->usri10_comment);
  SAFE_PUSH_WSTR(tmp->usri10_usr_comment);
  SAFE_PUSH_WSTR(tmp->usri10_full_name);
  /* 4 entries */
}

static void low_encode_user_info_11(USER_INFO_11 *tmp)
{
  low_encode_user_info_10((USER_INFO_10 *)tmp);
  push_int(tmp->usri11_priv);
  push_int(tmp->usri11_auth_flags);
  push_int(tmp->usri11_password_age);
  SAFE_PUSH_WSTR(tmp->usri11_home_dir);
  SAFE_PUSH_WSTR(tmp->usri11_parms);
  push_int(tmp->usri11_last_logon);
  push_int(tmp->usri11_last_logoff);
  push_int(tmp->usri11_bad_pw_count);
  push_int(tmp->usri11_num_logons);
  SAFE_PUSH_WSTR(tmp->usri11_logon_server);
  push_int(tmp->usri11_country_code);
  SAFE_PUSH_WSTR(tmp->usri11_workstations);
  push_int(tmp->usri11_max_storage);
  push_int(tmp->usri11_units_per_week);

  if(tmp->usri11_logon_hours)
   push_string(make_shared_binary_string(tmp->usri11_logon_hours,21));
  else
   push_int(0);

  push_int(tmp->usri11_code_page);
  /* 20 entries */
}

static void low_encode_user_info_20(USER_INFO_20 *tmp)
{
  SAFE_PUSH_WSTR(tmp->usri20_name);
  SAFE_PUSH_WSTR(tmp->usri20_full_name);
  SAFE_PUSH_WSTR(tmp->usri20_comment);
  push_int(tmp->usri20_flags);
  push_int(tmp->usri20_user_id);
  /* 5 entries */
}

static void encode_user_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_user_info_0 ((USER_INFO_0 *) u);break;
    case 1: low_encode_user_info_1 ((USER_INFO_1 *) u);f_aggregate(8); break;
    case 2: low_encode_user_info_2 ((USER_INFO_2 *) u);f_aggregate(24);break;
    case 3: low_encode_user_info_3 ((USER_INFO_3 *) u);f_aggregate(29);break;
    case 10:low_encode_user_info_10((USER_INFO_10 *)u);f_aggregate(4); break;
    case 11:low_encode_user_info_11((USER_INFO_11 *)u);f_aggregate(20);break;
    case 20:low_encode_user_info_20((USER_INFO_20 *)u);f_aggregate(5); break;
    default:
      Pike_error("Unsupported USERINFO level.\n");
  }
}

static void low_encode_group_info_0(GROUP_INFO_0 *tmp)
{
  SAFE_PUSH_WSTR(tmp->grpi0_name);
}

static void low_encode_group_info_1(GROUP_INFO_1 *tmp)
{
  low_encode_group_info_0((GROUP_INFO_0 *)tmp);
  SAFE_PUSH_WSTR(tmp->grpi1_comment);
  /* 2 entries */
}

static void low_encode_group_info_2(GROUP_INFO_2 *tmp)
{
  low_encode_group_info_1((GROUP_INFO_1 *)tmp);
  push_int(tmp->grpi2_group_id);
  push_int(tmp->grpi2_attributes);
  /* 4 entries */
}

static void encode_group_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_group_info_0 ((GROUP_INFO_0 *) u);break;
    case 1: low_encode_group_info_1 ((GROUP_INFO_1 *) u);f_aggregate(2); break;
    case 2: low_encode_group_info_2 ((GROUP_INFO_2 *) u);f_aggregate(4);break;
    default:
      Pike_error("Unsupported GROUPINFO level.\n");
  }
}

static void low_encode_localgroup_info_0(LOCALGROUP_INFO_0 *tmp)
{
  SAFE_PUSH_WSTR(tmp->lgrpi0_name);
}

static void low_encode_localgroup_info_1(LOCALGROUP_INFO_1 *tmp)
{
  low_encode_localgroup_info_0((LOCALGROUP_INFO_0 *)tmp);
  SAFE_PUSH_WSTR(tmp->lgrpi1_comment);
  /* 2 entries */
}

static void encode_localgroup_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_localgroup_info_0 ((LOCALGROUP_INFO_0 *) u);break;
    case 1: low_encode_localgroup_info_1 ((LOCALGROUP_INFO_1 *) u);f_aggregate(2); break;
    default:
      Pike_error("Unsupported LOCALGROUPINFO level.\n");
  }
}

static void low_encode_group_users_info_0(GROUP_USERS_INFO_0 *tmp)
{
  SAFE_PUSH_WSTR(tmp->grui0_name);
}

static void low_encode_group_users_info_1(GROUP_USERS_INFO_1 *tmp)
{
  low_encode_group_users_info_0((GROUP_USERS_INFO_0 *)tmp);
  push_int(tmp->grui1_attributes);
  /* 2 entries */
}

static void encode_group_users_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_group_users_info_0 ((GROUP_USERS_INFO_0 *) u);break;
    case 1: low_encode_group_users_info_1 ((GROUP_USERS_INFO_1 *) u);f_aggregate(2); break;
    default:
      Pike_error("Unsupported GROUPUSERSINFO level.\n");
  }
}

static void low_encode_localgroup_users_info_0(LOCALGROUP_USERS_INFO_0 *tmp)
{
  SAFE_PUSH_WSTR(tmp->lgrui0_name);
}

static void encode_localgroup_users_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_localgroup_users_info_0 ((LOCALGROUP_USERS_INFO_0 *) u);break;
    default:
      Pike_error("Unsupported LOCALGROUPUSERSINFO level.\n");
  }
}

static void low_encode_localgroup_members_info_0(LOCALGROUP_MEMBERS_INFO_0 *tmp)
{

#define SAFE_PUSH_SID(X) do {                           \
    if(getlengthsid && (X) && sid_program) {            \
      int lentmp=getlengthsid( (X) );                   \
      PSID psidtmp=xalloc(lentmp);                      \
      struct object *o=fast_clone_object(sid_program);	\
      memcpy( psidtmp, (X), lentmp);                    \
      (*(PSID *)(o->storage))=psidtmp;                  \
      push_object(o);                                   \
    } else {                                            \
      push_int(0);                                      \
    } } while(0)

  SAFE_PUSH_SID(tmp->lgrmi0_sid);
}

static void low_encode_localgroup_members_info_1(LOCALGROUP_MEMBERS_INFO_1 *tmp)
{
  low_encode_localgroup_members_info_0((LOCALGROUP_MEMBERS_INFO_0 *)tmp);
  push_int(tmp->lgrmi1_sidusage);
  SAFE_PUSH_WSTR(tmp->lgrmi1_name);
  /* 3 entries */
}

static void low_encode_localgroup_members_info_2(LOCALGROUP_MEMBERS_INFO_2 *tmp)
{
  low_encode_localgroup_members_info_0((LOCALGROUP_MEMBERS_INFO_0 *)tmp);
  push_int(tmp->lgrmi2_sidusage);
  SAFE_PUSH_WSTR(tmp->lgrmi2_domainandname);
  /* 3 entries */
}

static void low_encode_localgroup_members_info_3(LOCALGROUP_MEMBERS_INFO_3 *tmp)
{
  SAFE_PUSH_WSTR(tmp->lgrmi3_domainandname);
}

static void encode_localgroup_members_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_localgroup_members_info_0 ((LOCALGROUP_MEMBERS_INFO_0 *) u);break;
    case 1: low_encode_localgroup_members_info_1 ((LOCALGROUP_MEMBERS_INFO_1 *) u);f_aggregate(3);break;
    case 2: low_encode_localgroup_members_info_2 ((LOCALGROUP_MEMBERS_INFO_2 *) u);f_aggregate(3);break;
    case 3: low_encode_localgroup_members_info_3 ((LOCALGROUP_MEMBERS_INFO_3 *) u);break;
    default:
      Pike_error("Unsupported LOCALGROUPMEMBERSINFO level.\n");
  }
}

static int sizeof_user_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(USER_INFO_0);
    case 1: return sizeof(USER_INFO_1);
    case 2: return sizeof(USER_INFO_2);
    case 3: return sizeof(USER_INFO_3);
    case 10:return sizeof(USER_INFO_10);
    case 11:return sizeof(USER_INFO_11);
    case 20:return sizeof(USER_INFO_20);
    default: return -1;
  }
}

static int sizeof_group_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(GROUP_INFO_0);
    case 1: return sizeof(GROUP_INFO_1);
    case 2: return sizeof(GROUP_INFO_2);
    default: return -1;
  }
}

static int sizeof_localgroup_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(LOCALGROUP_INFO_0);
    case 1: return sizeof(LOCALGROUP_INFO_1);
    default: return -1;
  }
}

static int sizeof_group_users_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(GROUP_USERS_INFO_0);
    case 1: return sizeof(GROUP_USERS_INFO_1);
    default: return -1;
  }
}

static int sizeof_localgroup_users_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(LOCALGROUP_USERS_INFO_0);
    default: return -1;
  }
}

static int sizeof_localgroup_members_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(LOCALGROUP_MEMBERS_INFO_0);
    case 1: return sizeof(LOCALGROUP_MEMBERS_INFO_1);
    case 2: return sizeof(LOCALGROUP_MEMBERS_INFO_2);
    case 3: return sizeof(LOCALGROUP_MEMBERS_INFO_3);
    default: return -1;
  }
}

typedef NET_API_STATUS (WINAPI *netusergetinfotype)(LPWSTR,LPWSTR,DWORD,LPBYTE *);
typedef NET_API_STATUS (WINAPI *netuserenumtype)(LPWSTR,DWORD,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);
typedef NET_API_STATUS (WINAPI *netusergetgroupstype)(LPWSTR,LPWSTR,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD);
typedef NET_API_STATUS (WINAPI *netusergetlocalgroupstype)(LPWSTR,LPWSTR,DWORD,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD);
typedef NET_API_STATUS (WINAPI *netgroupenumtype)(LPWSTR,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);
typedef NET_API_STATUS (WINAPI *netgroupgetuserstype)(LPWSTR,LPWSTR,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);
typedef NET_API_STATUS (WINAPI *netgetdcnametype)(LPWSTR,LPWSTR,LPBYTE*);
typedef NET_API_STATUS (WINAPI *netapibufferfreetype)(LPVOID);

static netusergetinfotype netusergetinfo;
static netuserenumtype netuserenum;
static netusergetgroupstype netusergetgroups;
static netusergetlocalgroupstype netusergetlocalgroups;
static netgroupenumtype netgroupenum, netlocalgroupenum;
static netgroupgetuserstype netgroupgetusers, netlocalgroupgetmembers;
static netgetdcnametype netgetdcname, netgetanydcname;
static netapibufferfreetype netapibufferfree;

HINSTANCE netapilib;


/*! @decl string|array(string|int) NetUserGetInfo(string username, @
 *!                                               string|int(0..0) server, @
 *!                                               int|void level)
 *!
 *!   Get information about a network user.
 *!
 *! @param username
 *!   User name of the user to get information about.
 *!
 *! @param server
 *!   Server the user exists on.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!     @value 2
 *!     @value 3
 *!     @value 10
 *!     @value 11
 *!     @value 20
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserEnum()], @[NetGroupEnum()]
 *!   @[NetLocalGroupEnum()], @[NetUserGetGroups()],
 *!   @[NetUserGetLocalGroups()], @[NetGroupGetUsers()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetUserGetInfo(INT32 args)
{
  char *to_free1=NULL,*to_free2=NULL;
  BYTE *tmp=0;
  DWORD level;
  LPWSTR server, user;
  NET_API_STATUS ret;

  check_all_args(NULL,args,BIT_STRING|BIT_INT, BIT_STRING, BIT_VOID | BIT_INT, 0);

  if(args>2)
    level=sp[2-args].u.integer;
  else
    level=1;

  switch(level)
  {
    case 0: case 1: case 2: case 3: case 10: case 11: case 20:
      break;
    default:
      Pike_error("Unsupported information level in NetUserGetInfo.\n");
  }

  if(TYPEOF(sp[-args]) == T_STRING)
  {
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);
    if(!server)
      Pike_error("Server name string is too wide.\n");
  }else{
    server=NULL;
  }

  user=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);
  if(!user)
  {
    if(to_free1) free(to_free1);
    Pike_error("User name string is too wide.\n");
  }

  THREADS_ALLOW();
  ret=netusergetinfo(server,user,level,&tmp);
  THREADS_DISALLOW();

  pop_n_elems(args);
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);

  if (ret == NERR_Success)
  {
    encode_user_info(tmp,level);
    netapibufferfree(tmp);
  }
  else
    throw_nt_error(ret);
}

/*! @decl array(string|array(string|int)) @
 *!     NetUserEnum(string|int(0..0)|void server, @
 *!                 int|void level, int|void filter)
 *!
 *!   Get information about network users.
 *!
 *! @param server
 *!   Server the users exist on.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!     @value 2
 *!     @value 3
 *!     @value 10
 *!     @value 11
 *!     @value 20
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetGroupEnum()]
 *!   @[NetLocalGroupEnum()], @[NetUserGetGroups()],
 *!   @[NetUserGetLocalGroups()], @[NetGroupGetUsers()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetUserEnum(INT32 args)
{
  char *to_free1=NULL;
  DWORD level=0;
  DWORD filter=0;
  LPWSTR server=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD resume=0;
  NET_API_STATUS ret;

  check_all_args(NULL,args,BIT_STRING|BIT_INT|BIT_VOID, BIT_INT|BIT_VOID,BIT_INT|BIT_VOID,0);

  switch(args)
  {
    default:filter=sp[2-args].u.integer;
    case 2: level=sp[1-args].u.integer;
      switch(level)
      {
	case 0: case 1: case 2: case 3: case 10: case 11: case 20:
	  break;
	default:
          Pike_error("Unsupported information level.\n");
      }

    case 1:
      if(TYPEOF(sp[-args]) == T_STRING)
	server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

    case 0: break;
  }

  pop_n_elems(args);

  ret = ERROR_MORE_DATA;

  do
  {
    DWORD read=0, total=0, e;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netuserenum(server,
		    level,
		    filter,
		    &buf,
		    0x4000,
		    &read,
		    &total,
		    &resume);
    THREADS_DISALLOW();
    if(!a)
      push_array(a=allocate_array(total));

    if (ret == NERR_Success || ret == ERROR_MORE_DATA)
    {
      ptr=buf;
      for(e=0;e<read;e++)
      {
        encode_user_info(ptr,level);
        a->item[pos]=sp[-1];
        sp--;
        dmalloc_touch_svalue(sp);
        pos++;
        if(pos>=a->size) break;
        ptr+=sizeof_user_info(level);
      }
      netapibufferfree(buf);
    }
    else
    { if (to_free1) free(to_free1);
      throw_nt_error(ret);
      return;
    }
  } while (ret == ERROR_MORE_DATA);

  if (to_free1) free(to_free1);
}


/*! @decl array(string|array(string|int)) @
 *!     NetGroupEnum(string|int(0..0)|void server, int|void level)
 *!
 *!   Get information about network groups.
 *!
 *! @param server
 *!   Server the groups exist on.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!     @value 2
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetLocalGroupEnum()], @[NetUserGetGroups()],
 *!   @[NetUserGetLocalGroups()], @[NetGroupGetUsers()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetGroupEnum(INT32 args)
{
  char *to_free1=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD resume=0;
  NET_API_STATUS ret;

  check_all_args(NULL,args,BIT_STRING|BIT_INT|BIT_VOID, BIT_INT|BIT_VOID,0);

  if(args && TYPEOF(sp[-args]) == T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && TYPEOF(sp[1-args]) == T_INT) {
    level = sp[1-args].u.integer;
    switch(level)
    {
      case 0: case 1: case 2:
	break;
      default:
        Pike_error("Unsupported information level.\n");
    }
  }

  pop_n_elems(args);

  do
  {
    DWORD read=0, total=0, e;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netgroupenum(server,
		    level,
		    &buf,
		    0x10000,
		    &read,
		    &total,
		    &resume);
    THREADS_DISALLOW();
    if(!a)
      push_array(a=allocate_array(total));

    if (ret == NERR_Success || ret == ERROR_MORE_DATA)
    {
      ptr=buf;
      for(e=0;e<read;e++)
      {
        encode_group_info(ptr,level);
        a->item[pos]=sp[-1];
        sp--;
        dmalloc_touch_svalue(sp);
        pos++;
        if(pos>=a->size) break;
        ptr+=sizeof_group_info(level);
      }
      netapibufferfree(buf);
    }
    else
    {
      if (to_free1) free(to_free1);
      throw_nt_error(ret);
      return;
    }
  } while (ret == ERROR_MORE_DATA);

  if(to_free1) free(to_free1);
}

/*! @decl array(array(string|int)) @
 *!     NetLocalGroupEnum(string|int(0..0)|void server, int|void level)
 *!
 *!   Get information about local network groups.
 *!
 *! @param server
 *!   Server the groups exist on.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetUserGetGroups()],
 *!   @[NetUserGetLocalGroups()], @[NetGroupGetUsers()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetLocalGroupEnum(INT32 args)
{
  char *to_free1=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD resume=0;
  NET_API_STATUS ret;

  check_all_args(NULL,args,BIT_STRING|BIT_INT|BIT_VOID, BIT_INT|BIT_VOID,0);

  if(args && TYPEOF(sp[-args]) == T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && TYPEOF(sp[1-args]) == T_INT) {
    level = sp[1-args].u.integer;
    switch(level)
    {
      case 0: case 1:
	break;
      default:
        Pike_error("Unsupported information level.\n");
    }
  }

  pop_n_elems(args);

  do
  {
    DWORD read=0, total=0, e;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netlocalgroupenum(server,
		    level,
		    &buf,
		    0x10000,
		    &read,
		    &total,
		    &resume);
    THREADS_DISALLOW();
    if(!a)
      push_array(a=allocate_array(total));

    if (ret == NERR_Success || ret == ERROR_MORE_DATA)
    {
      ptr=buf;
      for(e=0;e<read;e++)
      {
        encode_localgroup_info(ptr,level);
        a->item[pos]=sp[-1];
        sp--;
        dmalloc_touch_svalue(sp);
        pos++;
        if(pos>=a->size) break;
        ptr+=sizeof_localgroup_info(level);
      }
      netapibufferfree(buf);
    }
    else
    {
      if (to_free1) free(to_free1);
      throw_nt_error(ret);
    }
  } while (ret == ERROR_MORE_DATA);

  if(to_free1) free(to_free1);
}

/*! @decl array(array(string|int)) @
 *!     NetUserGetGroups(string|int(0..0) server, string user, int|void level)
 *!
 *!   Get information about group membership for a network user.
 *!
 *! @param server
 *!   Server the groups exist on.
 *!
 *! @param user
 *!   User to retrieve groups for.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetLocalGroupEnum()],
 *!   @[NetUserGetLocalGroups()], @[NetGroupGetUsers()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetUserGetGroups(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  LPWSTR user=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD read=0, total=0, e;
  NET_API_STATUS ret;
  LPBYTE buf=0,ptr;

  check_all_args(NULL,args,BIT_STRING|BIT_INT, BIT_STRING,BIT_INT|BIT_VOID, 0);

  if(args>0 && TYPEOF(sp[-args]) == T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && TYPEOF(sp[-args+1]) == T_STRING)
    user=(LPWSTR)require_wstring1(sp[-args+1].u.string,&to_free2);

  if(args>2 && TYPEOF(sp[2-args]) == T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0: case 1:
	break;
      default:
        Pike_error("Unsupported information level.\n");
    }
  }

  pop_n_elems(args);


  THREADS_ALLOW();
  ret=netusergetgroups(server,
			user,
			level,
			&buf,
			0x100000,
			&read,
			&total);
  THREADS_DISALLOW();
  if(!a)
    push_array(a=allocate_array(total));

  if (ret == NERR_Success)
  {
    ptr=buf;
    for(e=0;e<read;e++)
    {
      encode_group_users_info(ptr,level);
      a->item[pos]=sp[-1];
      sp--;
      dmalloc_touch_svalue(sp);
      pos++;
      if(pos>=a->size) break;
      ptr+=sizeof_group_users_info(level);
    }
    netapibufferfree(buf);
    if(to_free1) free(to_free1);
    if(to_free2) free(to_free2);
  }
  else
  {
    if(to_free1) free(to_free1);
    if(to_free2) free(to_free2);
    throw_nt_error(ret);
  }
}

/*! @decl array(string) @
 *!     NetUserGetLocalGroups(string|int(0..0) server, @
 *!                           string user, int|void level, int|void flags)
 *!
 *!   Get information about group membership for a local network user.
 *!
 *! @param server
 *!   Server the groups exist on.
 *!
 *! @param user
 *!   User to retrieve groups for.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!   @endint
 *!
 *! @param flags
 *!   Zero, of one of the following:
 *!   @int
 *!     @value LG_INCLUDE_INDIRECT
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetLocalGroupEnum()],
 *!   @[NetUserGetGroups()], @[NetGroupGetUsers()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetUserGetLocalGroups(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL;
  DWORD level=0;
  DWORD flags=0;
  LPWSTR server=NULL;
  LPWSTR user=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD read=0, total=0, e;
  NET_API_STATUS ret;
  LPBYTE buf=0,ptr;

  check_all_args(NULL,args,BIT_STRING|BIT_INT, BIT_STRING,BIT_INT|BIT_VOID, BIT_INT|BIT_VOID, 0);

  if(args>0 && TYPEOF(sp[-args]) == T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && TYPEOF(sp[-args+1]) == T_STRING)
    user=(LPWSTR)require_wstring1(sp[-args+1].u.string,&to_free2);

  if(args>2 && TYPEOF(sp[2-args]) == T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0:
	break;
      default:
        Pike_error("Unsupported information level.\n");
    }
  }

  if(args>3 && TYPEOF(sp[3-args]) == T_INT)
    flags = sp[3-args].u.integer;

  pop_n_elems(args);


  THREADS_ALLOW();
  ret=netusergetlocalgroups(server,
			    user,
			    level,
			    flags,
			    &buf,
			    0x100000,
			    &read,
			    &total);
  THREADS_DISALLOW();
  if(!a)
    push_array(a=allocate_array(total));

  switch(ret)
  {
    case NERR_Success:
      ptr=buf;
      for(e=0;e<read;e++)
      {
  	encode_localgroup_users_info(ptr,level);
  	a->item[pos]=sp[-1];
  	sp--;
  	dmalloc_touch_svalue(sp);
  	pos++;
  	if(pos>=a->size) break;
  	ptr+=sizeof_localgroup_users_info(level);
      }
      netapibufferfree(buf);
      break;

    default:
      if(to_free1) free(to_free1);
      if(to_free2) free(to_free2);
      throw_nt_error(ret);
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

/*! @decl array(string|array(int|string)) @
 *!     NetGroupGetUsers(string|int(0..0) server, @
 *!                      string group, int|void level)
 *!
 *!   Get information about group membership for a network group.
 *!
 *! @param server
 *!   Server the groups exist on.
 *!
 *! @param group
 *!   Group to retrieve members for.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetLocalGroupEnum()],
 *!   @[NetUserGetGroups()], @[NetUserGetLocalGroups()],
 *!   @[NetLocalGroupGetMembers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetGroupGetUsers(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  LPWSTR group=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD resume=0;

  check_all_args(NULL,args,BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING, BIT_INT|BIT_VOID,0);

  if(args && TYPEOF(sp[-args]) == T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && TYPEOF(sp[1-args]) == T_STRING)
    group=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);

  if(args>2 && TYPEOF(sp[2-args]) == T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0: case 1:
	break;
      default:
        Pike_error("Unsupported information level.\n");
    }
  }

  pop_n_elems(args);

  while(1)
  {
    DWORD read=0, total=0, e;
    NET_API_STATUS ret;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netgroupgetusers(server,
			 group,
			 level,
			 &buf,
			 0x10000,
			 &read,
			 &total,
			 &resume);
    THREADS_DISALLOW();
    if(!a)
      push_array(a=allocate_array(total));

    switch(ret)
    {
      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_group_users_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  dmalloc_touch_svalue(sp);
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_group_users_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
	break;

      default:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
        throw_nt_error(ret);
    }
    break;
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

/*! @decl array(string|array(int|string)) @
 *!     NetLocalGroupGetMembers(string|int(0..0) server, @
 *!                             string group, int|void level)
 *!
 *!   Get information about group membership for a network group.
 *!
 *! @param server
 *!   Server the groups exist on.
 *!
 *! @param group
 *!   Group to retrieve members for.
 *!
 *! @param level
 *!   Information level. One of:
 *!   @int
 *!     @value 0
 *!     @value 1
 *!     @value 2
 *!     @value 3
 *!   @endint
 *!
 *! @returns
 *!   Returns an array on success. Throws errors on failure.
 *!
 *! @fixme
 *!   Document the return value.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetLocalGroupEnum()],
 *!   @[NetUserGetGroups()], @[NetUserGetLocalGroups()],
 *!   @[NetGroupGetUsers()], @[NetGetDCName()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetLocalGroupGetMembers(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  LPWSTR group=NULL;
  INT32 pos=0;
  struct array *a=0;
  DWORD resume=0;

  check_all_args(NULL,args,BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING, BIT_INT|BIT_VOID,0);

  if(args && TYPEOF(sp[-args]) == T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && TYPEOF(sp[1-args]) == T_STRING)
    group=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);

  if(args>2 && TYPEOF(sp[2-args]) == T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0: case 1: case 2: case 3:
	break;
      default:
        Pike_error("Unsupported information level.\n");
    }
  }

  pop_n_elems(args);

  while(1)
  {
    DWORD read=0, total=0, e;
    NET_API_STATUS ret;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netlocalgroupgetmembers(server,
				group,
				level,
				&buf,
				0x10000,
				&read,
				&total,
				&resume);
    THREADS_DISALLOW();
    if(!a)
      push_array(a=allocate_array(total));

    switch(ret)
    {
      case ERROR_NO_SUCH_ALIAS:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
        Pike_error("No such alias.\n");
	break;

      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_localgroup_members_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  dmalloc_touch_svalue(sp);
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_localgroup_members_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
	break;

      default:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
        throw_nt_error(ret);
        break;
    }
    break;
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

/*! @decl string NetGetDCName(string|int(0..0) server, string domain)
 *!
 *!   Get name of the domain controller from a server.
 *!
 *! @param server
 *!   Server the domain exists on.
 *!
 *! @param domain
 *!   Domain to get the domain controller for.
 *!
 *! @returns
 *!   Returns the name of the domain controller on success.
 *!   Throws errors on failure.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetLocalGroupEnum()],
 *!   @[NetUserGetGroups()], @[NetUserGetLocalGroups()],
 *!   @[NetGroupGetUsers()], @[NetLocalGroupGetMembers()],
 *!   @[NetGetAnyDCName()]
 */
void f_NetGetDCName(INT32 args)
{
  char *to_free1=NULL,*to_free2=NULL;
  BYTE *tmp=0;
  LPWSTR server, domain;
  NET_API_STATUS ret;

  check_all_args(NULL,args,BIT_STRING|BIT_INT, BIT_STRING, 0);

  if(TYPEOF(sp[-args]) == T_STRING)
  {
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);
    if(!server)
      Pike_error("Server name string is too wide.\n");
  }else{
    server=NULL;
  }

  domain=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);
  if(!domain)
  {
    if(to_free1) free(to_free1);
    Pike_error("Domain name string is too wide.\n");
  }

  THREADS_ALLOW();
  ret=netgetdcname(server,domain,&tmp);
  THREADS_DISALLOW();

  pop_n_elems(args);
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);

  switch(ret)
  {
    case NERR_Success:
      SAFE_PUSH_WSTR(tmp);
      netapibufferfree(tmp);
      return;

    default:
      throw_nt_error(ret);
  }
}

/*! @decl string NetGetAnyDCName(string|int(0..0) server, string domain)
 *!
 *!   Get name of a domain controller from a server.
 *!
 *! @param server
 *!   Server the domain exists on.
 *!
 *! @param domain
 *!   Domain to get a domain controller for.
 *!
 *! @returns
 *!   Returns the name of a domain controller on success.
 *!   Throws errors on failure.
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[NetUserGetInfo()], @[NetUserEnum()],
 *!   @[NetGroupEnum()], @[NetLocalGroupEnum()],
 *!   @[NetUserGetGroups()], @[NetUserGetLocalGroups()],
 *!   @[NetGroupGetUsers()], @[NetLocalGroupGetMembers()],
 *!   @[NetGetDCName()]
 */
void f_NetGetAnyDCName(INT32 args)
{
  char *to_free1=NULL,*to_free2=NULL;
  BYTE *tmp=0;
  LPWSTR server, domain;
  NET_API_STATUS ret;

  check_all_args(NULL,args,BIT_STRING|BIT_INT, BIT_STRING, 0);

  if(TYPEOF(sp[-args]) == T_STRING)
  {
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);
    if(!server)
      Pike_error("Server name string is too wide.\n");
  }else{
    server=NULL;
  }

  domain=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);
  if(!domain)
  {
    if(to_free1) free(to_free1);
    Pike_error("Domain name string is too wide.\n");
  }

  THREADS_ALLOW();
  ret=netgetanydcname(server,domain,&tmp);
  THREADS_DISALLOW();

  pop_n_elems(args);
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);

  switch(ret)
  {
    case NERR_Success:
      SAFE_PUSH_WSTR(tmp);
      netapibufferfree(tmp);
      return;

    default:
      throw_nt_error(ret);
  }
}

static LPWSTR get_wstring(struct svalue *s)
{
  if(TYPEOF(*s) != T_STRING) return (LPWSTR)0;
  switch(s->u.string->size_shift)
  {
    case 0:
    {
      struct string_builder x;
      init_string_builder(&x,1);
      string_builder_shared_strcat(&x, s->u.string);
      string_builder_putchar(&x, 0);
      string_builder_putchar(&x, 32767);
      free_string(s->u.string);
      s->u.string=finish_string_builder(&x);
    }
    /* Fall through */
    case 1:
      return STR1(s->u.string);
    default:
      Pike_error("Bad string width.\n");
      /* we never get here, but the "return (LPWSTR)0" makes the compiler
       * stop complaining about our not returning a value here.
       */
      UNREACHABLE();
  }
}


/* Stuff for NetSessionEnum */

LINKFUNC(NET_API_STATUS, netsessionenum,
	 (LPWSTR, LPWSTR, LPWSTR, DWORD, LPBYTE *,
	  DWORD, LPDWORD, LPDWORD, LPDWORD));

LINKFUNC(NET_API_STATUS, netwkstauserenum,
	 (LPWSTR, DWORD, LPBYTE *,
	  DWORD, LPDWORD, LPDWORD, LPDWORD));

static void low_encode_session_info_0(SESSION_INFO_0 *tmp)
{
  SAFE_PUSH_WSTR(tmp->sesi0_cname);
}

static void low_encode_session_info_1(SESSION_INFO_1 *tmp)
{
  SAFE_PUSH_WSTR(tmp->sesi1_cname);
  SAFE_PUSH_WSTR(tmp->sesi1_username);
  push_int(tmp->sesi1_time);
  push_int(tmp->sesi1_idle_time);
  push_int(tmp->sesi1_user_flags);
  f_aggregate(5);
}

static void low_encode_session_info_2(SESSION_INFO_2 *tmp)
{
  SAFE_PUSH_WSTR(tmp->sesi2_cname);
  SAFE_PUSH_WSTR(tmp->sesi2_username);
  push_int(tmp->sesi2_num_opens);
  push_int(tmp->sesi2_time);
  push_int(tmp->sesi2_idle_time);
  push_int(tmp->sesi2_user_flags);
  SAFE_PUSH_WSTR(tmp->sesi2_cltype_name);

  f_aggregate(7);
}

static void low_encode_session_info_10(SESSION_INFO_10 *tmp)
{
  SAFE_PUSH_WSTR(tmp->sesi10_cname);
  SAFE_PUSH_WSTR(tmp->sesi10_username);
  push_int(tmp->sesi10_time);
  push_int(tmp->sesi10_idle_time);

  f_aggregate(4);
}

static void low_encode_session_info_502(SESSION_INFO_502 *tmp)
{
  SAFE_PUSH_WSTR(tmp->sesi502_cname);
  SAFE_PUSH_WSTR(tmp->sesi502_username);
  push_int(tmp->sesi502_num_opens);

  push_int(tmp->sesi502_time);
  push_int(tmp->sesi502_idle_time);
  push_int(tmp->sesi502_user_flags);

  SAFE_PUSH_WSTR(tmp->sesi502_cltype_name);
  SAFE_PUSH_WSTR(tmp->sesi502_transport);

  f_aggregate(8);
}


static void encode_session_info(BYTE *u, int level)
{
  if(!u)
  {
    push_int(0);
    return;
  }
  switch(level)
  {
    case 0: low_encode_session_info_0 ((SESSION_INFO_0 *) u);break;
    case 1: low_encode_session_info_1 ((SESSION_INFO_1 *) u); break;
    case 2: low_encode_session_info_2 ((SESSION_INFO_2 *) u);break;
    case 10:low_encode_session_info_10((SESSION_INFO_10 *)u); break;
    case 502:low_encode_session_info_502((SESSION_INFO_502 *)u); break;
    default:
      Pike_error("Unsupported SESSIONINFO level.\n");
  }
}

static int sizeof_session_info(int level)
{
  switch(level)
  {
    case 0: return sizeof(SESSION_INFO_0);
    case 1: return sizeof(SESSION_INFO_1);
    case 2: return sizeof(SESSION_INFO_2);
    case 10: return sizeof(SESSION_INFO_10);
    case 502: return sizeof(SESSION_INFO_502);
    default: return -1;
  }
}

static void encode_wkstauser_info(BYTE *p, int level)
{
  if (!p)
  { /* This probably shouldn't happen, but the example on the
     * MSDN NetWkstaUserEnum manual page checks for this, so
     * we'll do that, too.
     */
    f_aggregate(0);
  }
  else if (level == 0)
  {
    WKSTA_USER_INFO_0 *wp = (WKSTA_USER_INFO_0 *) p;

    SAFE_PUSH_WSTR(wp->wkui0_username);
    f_aggregate(1);
  }
  else if (level == 1)
  {
    WKSTA_USER_INFO_1 *wp = (WKSTA_USER_INFO_1 *) p;

    SAFE_PUSH_WSTR(wp->wkui1_username);
    SAFE_PUSH_WSTR(wp->wkui1_logon_domain);
    SAFE_PUSH_WSTR(wp->wkui1_oth_domains);
    SAFE_PUSH_WSTR(wp->wkui1_logon_server);
    f_aggregate(4);
  }
  else
  {
    Pike_error("Unsupported WKSTA_USER_INFO level.\n");
  }
}

static int sizeof_wkstauser_info(int level)
{
  switch (level)
  {
    case 0: return sizeof(WKSTA_USER_INFO_0);
    case 1: return sizeof(WKSTA_USER_INFO_1);
    default: return -1;
  }
}

/*! @decl array(int|string) NetSessionEnum(string|int(0..0) server, @
 *!                                        string|int(0..0) client, @
 *!                                        string|int(0..0) user, int level)
 *!
 *!   Get session information.
 *!
 *! @param level
 *!   One of
 *!   @int
 *!     @value 0
 *!     @value 1
 *!     @value 2
 *!     @value 10
 *!     @value 502
 *!   @endint
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 */
static void f_NetSessionEnum(INT32 args)
{
  LPWSTR server, client, user;
  DWORD level;
  DWORD resume = 0;
  struct array *a=0;

  check_all_args(NULL,args,
		 BIT_INT|BIT_STRING,
		 BIT_INT|BIT_STRING,
		 BIT_INT|BIT_STRING,
		 BIT_INT,
		 0);

  server=get_wstring(sp-args);
  client=get_wstring(sp-args+1);
  user=get_wstring(sp-args+2);
  level=sp[3-args].u.integer;

  switch (level)
  {
    case 0: case 1: case 2: case 10: case 502:
      /* valid levels */
      break;
    default:
      Pike_error("Unsupported level: %d.\n", level);
  }


  while(1)
  {
    DWORD read=0, total=0, e, pos = 0;
    NET_API_STATUS ret;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netsessionenum(server,
		       client,
		       user,
		       level,
		       &buf,
		       0x2000,
		       &read,
		       &total,
		       &resume);
    THREADS_DISALLOW();

    if(!a)
      push_array(a=allocate_array(total));

    switch(ret)
    {
      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_session_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  pos++;
	  if((INT32)pos>=a->size) break;
	  ptr+=sizeof_session_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
	if(pos < total) continue;
	break;

      default:
        throw_nt_error(ret);
    }
    break;
  }
}

/* End netsessionenum */

/*! @decl array(mixed) NetWkstaUserEnum(string|int(0..0) server, int level)
 *!
 *! @param level
 *!   One of
 *!   @int
 *!     @value 0
 *!     @value 1
 *!   @endint
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 */
static void f_NetWkstaUserEnum(INT32 args)
{
  LPWSTR server;
  DWORD level;
  DWORD resume = 0;
  struct array *a=0;

  check_all_args(NULL,args,
		 BIT_INT|BIT_STRING,
		 BIT_INT,
		 0);

  server=get_wstring(sp-args);
  level=sp[1-args].u.integer;

  if (level != 0 && level != 1)
      Pike_error("Unsupported level: %d.\n", level);

  while(1)
  {
    DWORD read=0, total=0, e, pos = 0;
    NET_API_STATUS ret;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netwkstauserenum(server,
                         level,
                         &buf,
                         0x2000,
                         &read,
                         &total,
                         &resume);
    THREADS_DISALLOW();

    if(!a)
      push_array(a=allocate_array(total));

    switch(ret)
    {
      case NERR_Success:
      case ERROR_MORE_DATA:
        ptr=buf;
        for(e=0;e<read;e++)
        {
          encode_wkstauser_info(ptr,level);
          a->item[pos]=sp[-1];
          sp--;
          pos++;
          if((INT32)pos>=a->size) break;
          ptr+=sizeof_wkstauser_info(level);
        }
        netapibufferfree(buf);
        if(ret==ERROR_MORE_DATA) continue;
        if(pos < total) continue;
	break;

      default:
        throw_nt_error(ret);
    }
    break;
  }
}

/*! @decl utf8_string normalize_path(string(8bit) path)
 *!
 *!   Normalize an existing Windows file system path.
 *!
 *!   The following transformations are currently done:
 *!   @ul
 *!     @item
 *!       If the @[path] is not valid UTF-8, it will be converted
 *!       into UTF-8.
 *!     @item
 *!       Forward slashes (@expr{'/'@}) are converted to backward
 *!       slashes (@expr{'\'@}).
 *!     @item
 *!       Trailing slashes are removed, except a single slash after a
 *!       drive letter (e.g. @expr{"C:\"@} is returned instead of
 *!       @expr{"C:"@}).
 *!     @item
 *!       Extraneous empty extensions are removed.
 *!     @item
 *!       Short filenames are expanded to their corresponding long
 *!       variants.
 *!     @item
 *!       Relative paths are expanded to absolute paths.
 *!     @item
 *!       Current- and parent-directory path components (@expr{"."@}
 *!       and @expr{".."@}) are followed, similar to @[combine_path].
 *!     @item
 *!       Case-information in directory and file names is restored.
 *!     @item
 *!       Drive letters are returned in uppercase.
 *!     @item
 *!       The host and share parts of UNC paths are returned in
 *!       lowercase.
 *!   @endul
 *!
 *! @returns
 *!   A normalized absolute path without trailing slashes.
 *!
 *!   Throws errors on failure, e.g. if the file or directory doesn't
 *!   exist.
 *!
 *! @note
 *!   File fork information is currently not supported (invalid data).
 *!
 *! @note
 *!   In Pike 7.6 and earlier, this function didn't preserve a single
 *!   slash after drive letters, and it didn't convert the host and
 *!   share parts of an UNC path to lowercase.
 *!
 *! @seealso
 *!   @[combine_path()], @[combine_path_nt()]
 */
static void f_normalize_path(INT32 args)
{
  char *path = NULL;

  get_all_args("normalize_path", args, "%c", &path);

  path = fd_normalize_path(path);
  if (!path) {
    throw_nt_error(errno);
  }

  push_text(path);
  free(path);
}

/*! @decl int GetFileAttributes(string(8bit) filename)
 *!
 *!   Get the file attributes for the specified file.
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @seealso
 *!   @[SetFileAttributes()]
 */
static void f_GetFileAttributes(INT32 args)
{
  char *file;
  p_wchar1 *file_utf16;
  DWORD ret;
  get_all_args(NULL, args, "%c", &file);
  file_utf16 = pike_dwim_utf8_to_utf16(file);
  ret = GetFileAttributesW(file_utf16);
  free(file_utf16);
  pop_stack();
  errno=GetLastError();
  push_int(ret);
}

/*! @decl int SetFileAttributes(string(8bit) filename)
 *!
 *!   Set the file attributes for the specified file.
 *!
 *! @note
 *!   This function is only available on Win32 systems.
 *!
 *! @seealso
 *!   @[GetFileAttributes()]
 */
static void f_SetFileAttributes(INT32 args)
{
  char *file;
  p_wchar1 *file_utf16;
  INT_TYPE attr, ret;
  DWORD tmp;
  get_all_args(NULL, args, "%c%i", &file, &attr);
  file_utf16 = pike_dwim_utf8_to_utf16(file);
  tmp=attr;
  ret = SetFileAttributesW(file_utf16, tmp);
  free(file_utf16);
  pop_stack();
  errno=GetLastError();
  push_int(ret);
}

/*! @decl array(mixed) LookupAccountName(string|int(0..0) sys, string account)
 *!
 *! @returns
 *!   Returns @expr{0@} (zero) if the @[account] was not found,
 *!   and an array containing the following on success:
 *!
 *!   @array
 *!     @elem SID 0
 *!       @[SID] object representing the @[account].
 *!     @elem string 1
 *!       Domain name.
 *!     @elem int 2
 *!       Account type.
 *!   @endarray
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 */
static void f_LookupAccountName(INT32 args)
{
  LPCWSTR sys=0;
  LPCWSTR acc;
  DWORD sidlen, domainlen;
  SID_NAME_USE tmp;
  p_wchar1 buffer[1];

  check_all_args(NULL,args,BIT_INT|BIT_STRING, BIT_STRING,0);
  if(TYPEOF(sp[-args]) == T_STRING)
  {
    ref_push_string(sp[-args].u.string);
    push_int(2);
    f_string_to_unicode(2);
    args++;
    sys = (p_wchar1 *)STR0(sp[-1].u.string);
  }
  ref_push_string(sp[1-args].u.string);
  push_int(2);
  f_string_to_unicode(2);
  args++;
  acc = (p_wchar1 *)STR0(sp[-1].u.string);

  sidlen=0;
  domainlen=0;

  /* Find size required */
  lookupaccountname(sys,
		    acc,
		    (PSID)buffer,
		    &sidlen,
                    (LPWSTR)buffer,
		    &domainlen,
		    &tmp);

  if(sidlen && domainlen)
  {
    PSID sid=xalloc(sidlen);
    struct pike_string *dom = begin_wide_shared_string(domainlen-1, sixteenbit);
    dom->flags |= STRING_CONVERT_SURROGATES;

    if(lookupaccountname(sys,
			 acc,
			 sid,
			 &sidlen,
                         (LPWSTR)(STR1(dom)),
			 &domainlen,
			 &tmp))
    {
      struct object *o;
      pop_n_elems(args);
      o=fast_clone_object(sid_program);
      (*(PSID *)(o->storage))=sid;
      push_object(o);
      push_string(end_shared_string(dom));
      push_int(tmp);
      f_aggregate(3);
      return;
    }
    free(dom);
    free(sid);
  }
  errno=GetLastError();
  pop_n_elems(args);
  push_array(allocate_array(3));
}


static struct array *encode_acl(PACL acl)
{
  ACL_SIZE_INFORMATION tmp;
  if(GetAclInformation(acl, &tmp, sizeof(tmp), AclSizeInformation))
  {
    unsigned int e;
    check_stack(((ptrdiff_t)(tmp.AceCount + 4)));
    for(e=0;e<tmp.AceCount;e++)
    {
      void *ace;
      if(!GetAce(acl, e, &ace)) break;
      switch(((ACE_HEADER *)ace)->AceType)
      {
	case ACCESS_ALLOWED_ACE_TYPE:
	  push_static_text("allow");
	  push_int(((ACE_HEADER *)ace)->AceFlags);
	  push_int( ((ACCESS_ALLOWED_ACE *)ace)->Mask );
	  SAFE_PUSH_SID( & ((ACCESS_ALLOWED_ACE *)ace)->SidStart );
	  f_aggregate(4);
	  break;

	case ACCESS_DENIED_ACE_TYPE:
	  push_static_text("deny");
	  push_int(((ACE_HEADER *)ace)->AceFlags);
	  push_int( ((ACCESS_DENIED_ACE *)ace)->Mask );
	  SAFE_PUSH_SID( & ((ACCESS_DENIED_ACE *)ace)->SidStart );
	  f_aggregate(4);
	  break;

	case SYSTEM_AUDIT_ACE_TYPE:
	  push_static_text("audit");
	  push_int(((ACE_HEADER *)ace)->AceFlags);
	  push_int( ((SYSTEM_AUDIT_ACE *)ace)->Mask );
	  SAFE_PUSH_SID( & ((SYSTEM_AUDIT_ACE *)ace)->SidStart );
	  f_aggregate(4);
	  break;

	default:
	  push_static_text("unknown");
	  f_aggregate(1);
	  break;

      }
    }
    return aggregate_array(e);
  }
  return 0;
}

static PACL decode_acl(struct array *arr)
{
  PACL ret;
  int a;
  int size=sizeof(ACL);
  char *str;
  PSID *sid;

  for(a=0;a<arr->size;a++)
  {
    if(TYPEOF(arr->item[a]) != T_ARRAY)
      Pike_error("Index %d in ACL is not an array.\n",a);

    if(arr->item[a].u.array->size != 4)
      Pike_error("Index %d in ACL is not of size 4.\n",a);

    if(TYPEOF(arr->item[a].u.array->item[0]) != T_STRING)
      Pike_error("ACE[%d][%d] is not a string.\n",a,0);

    if(TYPEOF(arr->item[a].u.array->item[1]) != T_INT)
      Pike_error("ACE[%d][%d] is not an integer.\n",a,1);

    if(TYPEOF(arr->item[a].u.array->item[2]) != T_INT)
      Pike_error("ACE[%d][%d] is not an integer.\n",a,2);

    if(TYPEOF(arr->item[a].u.array->item[3]) != T_OBJECT)
      Pike_error("ACE[%d][%d] is not a SID class.\n",a,3);

    sid=(PSID *)get_storage(arr->item[a].u.array->item[3].u.object,sid_program);
    if(!sid || !*sid)
      Pike_error("ACE[%d][%d] is not a SID class.\n",a,3);

    str=arr->item[a].u.array->item[0].u.string->str;
    switch( ( str[0] << 8 ) + str[1] )
    {
      case ( 'a' << 8 ) + 'c': size += sizeof(ACCESS_ALLOWED_ACE); break;
      case ( 'd' << 8 ) + 'e': size += sizeof(ACCESS_DENIED_ACE); break;
      case ( 'a' << 8 ) + 'u': size += sizeof(SYSTEM_AUDIT_ACE); break;
      default:
	Pike_error("ACE[%d][0] is not a known ACE type.\n");
    }
    size += getlengthsid( *sid ) - sizeof(DWORD);
  }

  ret=xalloc( size );

  if(!initializeacl(ret, size, ACL_REVISION))
    Pike_error("InitializeAcl failed!\n");

  for(a=0;a<arr->size;a++)
  {
    str=arr->item[a].u.array->item[0].u.string->str;
    sid=(PSID *)get_storage(arr->item[a].u.array->item[3].u.object,sid_program);

    switch( ( str[0] << 8 ) + str[1] )
    {
      case ( 'a' << 8 ) + 'c':
	if(!addaccessallowedace(ret, ACL_REVISION,
				arr->item[a].u.array->item[2].u.integer,
				sid))
	  Pike_error("AddAccessAllowedAce failed!\n");
	break;

      case ( 'd' << 8 ) + 'e':
	if(!addaccessdeniedace(ret, ACL_REVISION,
			       arr->item[a].u.array->item[2].u.integer,
			       sid))
	  Pike_error("AddAccessDeniedAce failed!\n");
	break;

      case ( 'a' << 8 ) + 'u':
	/* FIXME, what to do with the last two arguments ?? */
	if(!addauditaccessace(ret, ACL_REVISION,
			      arr->item[a].u.array->item[2].u.integer,
			      sid,1,1))
	  Pike_error("AddAuditAccessAce failed!\n");
	break;
    }
  }
  return ret;
}

/*
 * Note, this function does not use errno!!
 * (Time to learn how to autodoc... /Hubbe)
 */
/*! @decl int SetNamedSecurityInfo(string name, mapping(string:mixed) options)
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[GetNamedSecurityInfo()]
 */
static void f_SetNamedSecurityInfo(INT32 args)
{
  struct svalue *sval;
  struct pike_string *name;
  struct mapping *m;
  SECURITY_INFORMATION flags=0;
  PSID owner=0;
  PSID group=0;
  PACL dacl=0;
  PACL sacl=0;
  DWORD ret;
  SE_OBJECT_TYPE type=SE_FILE_OBJECT;

  get_all_args(NULL, args, "%t%m", &name, &m);

  if((sval=low_mapping_string_lookup(m, literal_type_string)))
  {
    if(TYPEOF(*sval) != T_INT)
      Pike_error("Bad 'type' type.\n");
    type=sval->u.integer;
  }

  if((sval=simple_mapping_string_lookup(m,"owner")))
  {
    if(TYPEOF(*sval) != T_OBJECT ||
       !get_storage(sval->u.object, sid_program))
      Pike_error("Bad 'owner' type.\n");
    owner=*(PSID *)get_storage(sval->u.object, sid_program);
    flags |= OWNER_SECURITY_INFORMATION;
  }

  if((sval=simple_mapping_string_lookup(m,"group")))
  {
    if(TYPEOF(*sval) != T_OBJECT ||
       !get_storage(sval->u.object, sid_program))
      Pike_error("Bad 'group' type.\n");
    group=*(PSID *)get_storage(sval->u.object, sid_program);
    flags |= GROUP_SECURITY_INFORMATION;
  }

  if((sval=simple_mapping_string_lookup(m,"dacl")))
  {
    if(TYPEOF(*sval) != T_ARRAY)
      Pike_error("Bad 'dacl' type.\n");
    dacl=decode_acl(sval->u.array);
    flags |= DACL_SECURITY_INFORMATION;
  }

  if((sval=simple_mapping_string_lookup(m,"sacl")))
  {
    if(TYPEOF(*sval) != T_ARRAY)
      Pike_error("Bad 'sacl' type.\n");
    sacl=decode_acl(sval->u.array);
    flags |= SACL_SECURITY_INFORMATION;
  }

  ref_push_string(name);
  push_int(2);
  f_string_to_unicode(2);
  args++;
  name = Pike_sp[-1].u.string;

  /* FIXME, add dacl and sacl!!!! */

  ret = setnamedsecurityinfo((p_wchar1 *)STR0(name),
			   type,
			   flags,
			   owner,
			   group,
			   dacl,
			   sacl);
  pop_n_elems(args);
  push_int(ret);
}

/*! @decl mapping(mixed:mixed) GetNamedSecurityInfo(string name, @
 *!                                                 int|void type, @
 *!                                                 int|void flags)
 *!
 *! @note
 *!   This function is only available on some Win32 systems.
 *!
 *! @seealso
 *!   @[SetNamedSecurityInfo()]
 */
static void f_GetNamedSecurityInfo(INT32 args)
{
  struct pike_string *name;
  PSID owner=0, group=0;
  PACL dacl=0, sacl=0;
  PSECURITY_DESCRIPTOR desc=0;
  DWORD ret;
  SECURITY_INFORMATION flags=
    OWNER_SECURITY_INFORMATION |
    GROUP_SECURITY_INFORMATION |
    DACL_SECURITY_INFORMATION;

  SE_OBJECT_TYPE type = SE_FILE_OBJECT;
  check_all_args(NULL,args,BIT_STRING, BIT_VOID|BIT_INT, BIT_VOID|BIT_INT, 0);

  switch(args)
  {
    default: flags=sp[2-args].u.integer;
    case 2: type=sp[1-args].u.integer;
    case 1: break;
  }

  name = sp[-args].u.string;
  ref_push_string(name);
  push_int(2);
  f_string_to_unicode(2);
  args++;
  name = sp[-1].u.string;

  if(!(ret=getnamedsecurityinfo((p_wchar1 *)STR0(name),
				type,
				flags,
				&owner,
				&group,
				&dacl,
				&sacl,
				&desc)))
  {
    int tmp=0;
    pop_n_elems(args);

    if(owner)
    {
      push_static_text("owner");
      SAFE_PUSH_SID(owner);
      tmp++;
    }
    if(group)
    {
      push_static_text("group");
      SAFE_PUSH_SID(group);
      tmp++;
    }
    if(sacl)
    {
      push_static_text("sacl");
      push_array( encode_acl( sacl ));
      tmp++;
    }
    if(dacl)
    {
      push_static_text("dacl");
      push_array( encode_acl( dacl ));
      tmp++;
    }
    f_aggregate_mapping(tmp * 2);
  }else{
    pop_n_elems(args);
    push_int(0);
    errno=ret; /* magic */
  }
  if(desc) LocalFree(desc);
}

/* Implementation of uname() for Win32. */
static void f_nt_uname(INT32 args)
{
  /* More info could be added here */

  char buf[1024];
  char *machine = "unknown";
  char *version = "??";
  OSVERSIONINFO osversion;
  SYSTEM_INFO sysinfo;
  int n=0;

  pop_n_elems(args);

  osversion.dwOSVersionInfoSize=sizeof(osversion);
  if(!GetVersionEx(&osversion))
    Pike_error("GetVersionEx failed with errno %d\n",GetLastError());

  GetSystemInfo(&sysinfo);

  n+=2;
  push_static_text("architecture");
  switch(sysinfo.wProcessorArchitecture)
  {
    case PROCESSOR_ARCHITECTURE_INTEL:
      sprintf(buf, "i%ld", sysinfo.dwProcessorType);
      push_text(buf);
      machine = "i86pc";
      break;

    case PROCESSOR_ARCHITECTURE_MIPS:
      push_static_text("mips");
      machine = "mips";
      break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
      push_static_text("alpha");
      machine = "alpha";
      break;

    case PROCESSOR_ARCHITECTURE_PPC:
      push_static_text("ppc");
      machine = "ppc";
      break;

#ifdef PROCESSOR_ARCHITECTURE_SHX
    case PROCESSOR_ARCHITECTURE_SHX:
      machine = "shx";
      switch (sysinfo.dwProcessorType) {
      case PROCESSOR_HITACHI_SH3:
      case PROCESSOR_SHx_SH3:
        push_static_text("sh3");
	break;
      case PROCESSOR_HITACHI_SH3E:
        push_static_text("sh3e");
	break;
      case PROCESSOR_HITACHI_SH4:
      case PROCESSOR_SHx_SH4:
        push_static_text("sh4");
	break;
      default:
	push_static_text("shx");
	break;
      }
      break;
#endif /* PROCESSOR_ARCHITECTURE_SHX */

#ifdef PROCESSOR_ARCHITECTURE_ARM
    case PROCESSOR_ARCHITECTURE_ARM:
      machine = "arm";
      switch (sysinfo.dwProcessorType) {
      case PROCESSOR_STRONGARM:
	push_static_text("strongarm");
	break;
      case PROCESSOR_ARM720:
	push_static_text("arm720");
	break;
      case PROCESSOR_ARM820:
	push_static_text("arm820");
	break;
      case PROCESSOR_ARM920:
	push_static_text("arm920");
	break;
      case PROCESSOR_ARM_7TDMI:
	push_static_text("arm7tdmi");
	break;
      default:
	push_static_text("arm");
	break;
      }
      break;
#endif /* PROCESSOR_ARCHITECTURE_ARM */

#ifdef PROCESSOR_ARCHITECTURE_ALPHA64
    case PROCESSOR_ARCHITECTURE_ALPHA64:
      machine = "alpha64";
      push_static_text("alpha64");
      break;
#endif /* PROCESSOR_ARCHITECTURE_ALPHA64 */

#ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64:
      machine = "amd64";
      push_static_text("amd64");
      break;
#endif

#ifdef PROCESSOR_ARCHITECTURE_MSIL
    case PROCESSOR_ARCHITECTURE_MSIL:
      machine = "msil";
      push_static_text("msil");
      break;
#endif /* PROCESSOR_ARCHITECTURE_MSIL */

    default:
    case PROCESSOR_ARCHITECTURE_UNKNOWN:
      machine = "unknown";
      push_static_text("unknown");
      break;
  }

  n+=2;
  push_static_text("machine");
  push_text(machine);

  n+=2;
  push_static_text("sysname");
  switch(osversion.dwPlatformId)
  {
    case VER_PLATFORM_WIN32s:
      version = "3.1";
      push_static_text("Win32s");
      break;

    case VER_PLATFORM_WIN32_WINDOWS:
      push_static_text("Win32");
      switch(osversion.dwMinorVersion)
      {
      case 0:
	version = "95";
	break;
      case 10:
	version = "98";
	break;
      case 90:
	version = "Me";
	break;
      }
      break;

    case VER_PLATFORM_WIN32_NT:
      push_static_text("Win32");
      switch(osversion.dwMajorVersion)
      {
      case 3:
	version = "NT 3.51";
	break;
      case 4:
	version = "NT 4.0";
	break;
      case 5:
	switch(osversion.dwMinorVersion)
	{
	case 0:
	  version = "2000";
	  break;
	case 1:
	  version = "XP";
	  break;
	case 2:
	  version = "Server 2003";
	  break;
	}
      }
      break;

    default:
      push_static_text("Win32");
      break;
  }

  snprintf(buf, sizeof(buf), "Windows %s %ld.%ld.%ld",
	  version,
	  osversion.dwMajorVersion,
	  osversion.dwMinorVersion,
	  osversion.dwBuildNumber & 0xffff);

  n+=2;
  push_static_text("release");
  push_text(buf);

  n+=2;
  push_static_text("version");
  push_text(osversion.szCSDVersion);

  n+=2;
  push_static_text("nodename");
  gethostname(buf, sizeof(buf));
  push_text(buf);

  f_aggregate_mapping(n);
}

/**************************/
/* start Security context */

typedef SECURITY_STATUS (WINAPI *querysecuritypackageinfotype)(SEC_WCHAR*,PSecPkgInfo*);
typedef SECURITY_STATUS (WINAPI *acquirecredentialshandletype)(SEC_WCHAR*,SEC_WCHAR*,ULONG,PLUID,PVOID,SEC_GET_KEY_FN,PVOID,PCredHandle,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *freecredentialshandletype)(PCredHandle);
typedef SECURITY_STATUS (WINAPI *acceptsecuritycontexttype)(PCredHandle,PCtxtHandle,PSecBufferDesc,ULONG,ULONG,PCtxtHandle,PSecBufferDesc,PULONG,PTimeStamp);
typedef SECURITY_STATUS (WINAPI *completeauthtokentype)(PCtxtHandle,PSecBufferDesc);
typedef SECURITY_STATUS (WINAPI *deletesecuritycontexttype)(PCtxtHandle);
typedef SECURITY_STATUS (WINAPI *freecontextbuffertype)(PVOID);
typedef SECURITY_STATUS (WINAPI *querycontextattributestype)(PCtxtHandle,ULONG,PVOID);

static querysecuritypackageinfotype querysecuritypackageinfo;
static acquirecredentialshandletype acquirecredentialshandle;
static freecredentialshandletype freecredentialshandle;
static acceptsecuritycontexttype acceptsecuritycontext;
static completeauthtokentype completeauthtoken;
static deletesecuritycontexttype deletesecuritycontext;
static freecontextbuffertype freecontextbuffer;
static querycontextattributestype querycontextattributes;

HINSTANCE securlib;

struct sctx_storage {
  CredHandle hcred;
  int        hcred_alloced;
  CtxtHandle hctxt;
  int        hctxt_alloced;
  p_wchar1   *PackageNameUTF16;
  DWORD      cbMaxMessage;
  BYTE *     buf;
  DWORD      cBuf;
  int        done;
  int        lastError;
};

#define THIS_SCTX ((struct sctx_storage *)CURRENT_STORAGE)
static struct program *sctx_program;
static void init_sctx(struct object *o)
{
  struct sctx_storage *sctx = THIS_SCTX;

  memset(sctx, 0, sizeof(struct sctx_storage));
}

static void exit_sctx(struct object *o)
{
  struct sctx_storage *sctx = THIS_SCTX;

  if (sctx->PackageNameUTF16) {
    free(sctx->PackageNameUTF16);
    sctx->PackageNameUTF16 = NULL;
  }
  if (sctx->hctxt_alloced) {
    deletesecuritycontext (&sctx->hctxt);
    sctx->hctxt_alloced = 0;
  }
  if (sctx->hcred_alloced) {
    freecredentialshandle (&sctx->hcred);
    sctx->hcred_alloced = 0;
  }
  if (sctx->buf) {
    free(sctx->buf);
    sctx->buf = 0;
  }
}


static void f_sctx_create(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  SECURITY_STATUS ss;
  PSecPkgInfo     pkgInfo;
  TimeStamp       Lifetime;
  struct pike_string *pkgName;

  get_all_args(NULL, args, "%t", &pkgName);

  if (sctx->PackageNameUTF16) {
    free(sctx->PackageNameUTF16);
    sctx->PackageNameUTF16 = NULL;
  }
  sctx->PackageNameUTF16 = pike_string_to_utf16(pkgName, 1);

  ss = querysecuritypackageinfo(sctx->PackageNameUTF16, &pkgInfo);

  if (!SEC_SUCCESS(ss))
  {
    Pike_error("Could not query package info for %pS, error 0x%08x.\n",
               pkgName, ss);
  }

  sctx->cbMaxMessage = pkgInfo->cbMaxToken;
  if (sctx->buf)
    free(sctx->buf);
  sctx->buf = malloc(sctx->cbMaxMessage);

  freecontextbuffer(pkgInfo);

  if (sctx->hcred_alloced)
    freecredentialshandle (&sctx->hcred);
  ss = acquirecredentialshandle (
                                 NULL,
                                 sctx->PackageNameUTF16,
                                 SECPKG_CRED_INBOUND,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &sctx->hcred,
                                 &Lifetime);

  if (!SEC_SUCCESS (ss))
  {
    Pike_error("AcquireCreds failed: 0x%08x.\n", ss);
  }
  sctx->hcred_alloced = 1;
}


BOOL GenServerContext (BYTE *pIn, DWORD cbIn, BYTE *pOut, DWORD *pcbOut,
                       BOOL *pfDone, BOOL fNewConversation)
{
  struct sctx_storage *sctx = THIS_SCTX;
  SECURITY_STATUS   ss;
  TimeStamp         Lifetime;
  SecBufferDesc     OutBuffDesc;
  SecBuffer         OutSecBuff;
  SecBufferDesc     InBuffDesc;
  SecBuffer         InSecBuff;
  ULONG             Attribs = 0;

  /*----------------------------------------------------------------
   * Prepare output buffers
   */

  OutBuffDesc.ulVersion = 0;
  OutBuffDesc.cBuffers = 1;
  OutBuffDesc.pBuffers = &OutSecBuff;

  OutSecBuff.cbBuffer = *pcbOut;
  OutSecBuff.BufferType = SECBUFFER_TOKEN;
  OutSecBuff.pvBuffer = pOut;

  /*----------------------------------------------------------------
   * Prepare input buffers
   */

  InBuffDesc.ulVersion = 0;
  InBuffDesc.cBuffers = 1;
  InBuffDesc.pBuffers = &InSecBuff;

  InSecBuff.cbBuffer = cbIn;
  InSecBuff.BufferType = SECBUFFER_TOKEN;
  InSecBuff.pvBuffer = pIn;

  *pcbOut = 0;
  *pfDone = 0;

  ss = acceptsecuritycontext (&sctx->hcred,
                              fNewConversation ? NULL : &sctx->hctxt,
                              &InBuffDesc,
                              Attribs,
                              SECURITY_NATIVE_DREP,
                              &sctx->hctxt,
                              &OutBuffDesc,
                              &Attribs,
                              &Lifetime);

  if (!SEC_SUCCESS (ss))
  {
    sctx->lastError = ss;
    return FALSE;
  }
  sctx->hctxt_alloced = 1;

  /*----------------------------------------------------------------
   * Complete token -- if applicable
   */

  if ((SEC_I_COMPLETE_NEEDED == ss)
      || (SEC_I_COMPLETE_AND_CONTINUE == ss))
  {
    ss = completeauthtoken (&sctx->hctxt, &OutBuffDesc);
    if (!SEC_SUCCESS(ss))
    {
      sctx->lastError = ss;
      return FALSE;
    }
  }

  *pcbOut = OutSecBuff.cbBuffer;

  *pfDone = !((SEC_I_CONTINUE_NEEDED == ss)
              || (SEC_I_COMPLETE_AND_CONTINUE == ss));

  return TRUE;

} /* end GenServerContext */


static void f_sctx_gencontext(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  struct pike_string *in;
  BOOL new_conversation = 0;

  check_all_args(NULL, args, BIT_STRING,0);

  in = Pike_sp[-1].u.string;
  if (in->size_shift != 0)
    Pike_error("Wide strings is not allowed.\n");
  sctx->cBuf = sctx->cbMaxMessage;
  if (!GenServerContext (in->str, in->len, sctx->buf, &sctx->cBuf,
                         &sctx->done, !sctx->hctxt_alloced))
  {
    pop_n_elems(args);
    push_int(0);
    return;
  }

  pop_n_elems(args);

  push_int(sctx->done?1:0);
  push_string(make_shared_binary_string(sctx->buf, sctx->cBuf));
  f_aggregate(2);
}


static void f_sctx_getlastcontext(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  pop_n_elems(args);

  if (sctx->lastError)
  {
    push_int(0);
    return;
  }
  push_int(sctx->done?1:0);
  push_string(make_shared_binary_string(sctx->buf, sctx->cBuf));
  f_aggregate(2);
}


static void f_sctx_isdone(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  pop_n_elems(args);
  push_int(sctx->done?1:0);
}


static void f_sctx_type(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  pop_n_elems(args);
  push_utf16_text(sctx->PackageNameUTF16);
}


static void f_sctx_getusername(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  SECURITY_STATUS   ss;
  SecPkgContext_NamesW name;

  pop_n_elems(args);

  if (!sctx->hctxt_alloced)
  {
    push_int(0);
    return;
  }

  ss = querycontextattributes(&sctx->hctxt, SECPKG_ATTR_NAMES, &name);
  if (ss != SEC_E_OK)
  {
    push_int(0);
    return;
  }

  push_utf16_text(name.sUserName);

  freecontextbuffer(name.sUserName);
}


static void f_sctx_getlasterror(INT32 args)
{
  struct sctx_storage *sctx = THIS_SCTX;
  LPVOID lpMsgBuf;
  char buf[100];

  pop_n_elems(args);

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                sctx->lastError,
		/* Default language */
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                NULL
                );
  sprintf(buf, "0x%04x: ", sctx->lastError);
  push_text(buf);
  push_text(lpMsgBuf);
  f_add(2);
  LocalFree(lpMsgBuf);
}

/*! @decl string GetComputerName()
 *!
 *! Retrieves the NetBIOS name of the local computer.
 *!
 *! @note
 *!   This function is Windows specific, and is not available on all systems.
 */
static void f_GetComputerName(INT32 args)
{
  char  name[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD len = sizeof(name);

  pop_n_elems(args);

  if (!GetComputerName(name, &len))
  {
    push_int(0);
    return;
  }

  push_string(make_shared_binary_string(name, len));
}

/*! @decl string GetUserName()
 *!
 *! Retrieves the name of the user associated with the current thread.
 *!
 *! @note
 *!   This function is Windows specific, and is not available on all systems.
 */
#ifdef HAVE_GETUSERNAME
static void f_GetUserName(INT32 args)
{
  char  name[UNLEN + 1];
  DWORD len = sizeof(name);

  pop_n_elems(args);

  if (!GetUserName(name, &len))
  {
    push_int(0);
    return;
  }

  push_string(make_shared_binary_string(name, len-1));
}
#endif /* HAVE_GETUSERNAME */

#define ADD_GLOBAL_INTEGER_CONSTANT(X,Y) \
   push_int((long)(Y)); low_add_constant(X,sp-1); pop_stack();
#define SIMPCONST(X) \
      add_integer_constant(#X,X,0);

void init_nt_system_calls(void)
{
  /* function(string,string:int) */

  ADD_FUNCTION("GetFileAttributes", f_GetFileAttributes,
               tFunc(tStr8, tInt), 0);
  ADD_FUNCTION("SetFileAttributes", f_SetFileAttributes,
               tFunc(tStr8 tInt, tInt), 0);

  SIMPCONST(FILE_ATTRIBUTE_ARCHIVE);
  SIMPCONST(FILE_ATTRIBUTE_HIDDEN);
  SIMPCONST(FILE_ATTRIBUTE_NORMAL);
  SIMPCONST(FILE_ATTRIBUTE_OFFLINE);
  SIMPCONST(FILE_ATTRIBUTE_READONLY);
  SIMPCONST(FILE_ATTRIBUTE_SYSTEM);
  SIMPCONST(FILE_ATTRIBUTE_TEMPORARY);

  SIMPCONST(FILE_ATTRIBUTE_COMPRESSED);
  SIMPCONST(FILE_ATTRIBUTE_DIRECTORY);
#ifdef FILE_ATTRIBUTE_ENCRYPTED
  SIMPCONST(FILE_ATTRIBUTE_ENCRYPTED);
#endif
#ifdef FILE_ATTRIBUTE_REPARSE_POINT
  SIMPCONST(FILE_ATTRIBUTE_REPARSE_POINT);
#endif
#ifdef FILE_ATTRIBUTE_SPARSE_FILE
  SIMPCONST(FILE_ATTRIBUTE_SPARSE_FILE);
#endif

  ADD_FUNCTION("cp",f_cp,tFunc(tStr tStr,tInt), 0);

  /* See array hkeys[] above. */

  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CLASSES_ROOT", 0);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_LOCAL_MACHINE", 1);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CURRENT_USER", 2);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_USERS", 3);

/* function(int,string,string:string|int|string*) */
  ADD_FUNCTION("RegGetValue", f_RegGetValue,
               tFunc(tInt tStr tStr tOr(tVoid, tInt01),
                     tOr3(tStr, tInt, tArr(tStr))),
               OPT_EXTERNAL_DEPEND);

  ADD_FUNCTION("RegGetValues", f_RegGetValues,
               tFunc(tInt tStr tOr(tVoid, tInt01),
                     tMap(tStr, tOr3(tStr, tInt, tArr(tStr)))),
               OPT_EXTERNAL_DEPEND);

  ADD_FUNCTION("RegGetKeyNames", f_RegGetKeyNames,
               tFunc(tInt tStr, tArr(tStr)), OPT_EXTERNAL_DEPEND);

/* function(void:int) */
#ifdef HAVE_FREECONSOLE
  ADD_FUNCTION("FreeConsole", f_freeconsole, tFunc(tNone,tInt), 0);
#endif /* HAVE_FREECONSOLE */

/* function(void:int) */
#ifdef HAVE_ALLOCCONSOLE
  ADD_FUNCTION("AllocConsole", f_allocconsole, tFunc(tNone,tInt), 0);
#endif /* HAVE_ALLOCCONSOLE */

/* function(int:int) */
#ifdef HAVE_ATTACHCONSOLE
  ADD_FUNCTION("AttachConsole", f_attachconsole, tFunc(tInt,tInt), 0);
#endif /* HAVE_ATTACHCONSOLE */

  ADD_EFUN("uname", f_nt_uname,tFunc(tNone,tMapping), OPT_TRY_OPTIMIZE);
  ADD_FUNCTION2("uname", f_nt_uname,tFunc(tNone,tMapping), 0, OPT_TRY_OPTIMIZE);

  ADD_FUNCTION("normalize_path", f_normalize_path, tFunc(tStr8, tUtf8Str), 0);

  /* LogonUser only exists on NT, link it dynamically */
  if( (advapilib=LoadLibrary("advapi32")) )
  {
    FARPROC proc;
    if( (proc=GetProcAddress(advapilib, "LogonUserW")) )
    {
      logonuser=(logonusertype)proc;

      /* function(string,string,string,int|void,void|int:object) */
      ADD_FUNCTION("LogonUser", f_LogonUser,
                   tFunc(tStr tOr(tStr, tZero) tStr tOr(tInt, tVoid)
                         tOr(tVoid, tInt), tObj), 0);

      SIMPCONST(LOGON32_LOGON_BATCH);
      SIMPCONST(LOGON32_LOGON_INTERACTIVE);
      SIMPCONST(LOGON32_LOGON_SERVICE);
      SIMPCONST(LOGON32_LOGON_NETWORK);
      SIMPCONST(LOGON32_PROVIDER_DEFAULT);

      start_new_program();
      ADD_STORAGE(HANDLE);
      set_init_callback(init_token);
      set_exit_callback(exit_token);
      token_program=end_program();
      add_program_constant("UserToken",token_program,0);
      token_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;
    }

    if( (proc=GetProcAddress(advapilib, "LookupAccountNameW")) )
      lookupaccountname=(lookupaccountnametype)proc;

    if( (proc=GetProcAddress(advapilib, "LookupAccountSidW")) )
      lookupaccountsid=(lookupaccountsidtype)proc;

    if( (proc=GetProcAddress(advapilib, "SetNamedSecurityInfoW")) )
      setnamedsecurityinfo=(setnamedsecurityinfotype)proc;

    if( (proc=GetProcAddress(advapilib, "GetNamedSecurityInfoW")) )
      getnamedsecurityinfo=(getnamedsecurityinfotype)proc;

    if( (proc=GetProcAddress(advapilib, "EqualSid")) )
      equalsid=(equalsidtype)proc;

    if( (proc=GetProcAddress(advapilib, "InitializeAcl")) )
      initializeacl=(initializeacltype)proc;

    if( (proc=GetProcAddress(advapilib, "AddAccessAllowedAce")) )
      addaccessallowedace=(addaccessallowedacetype)proc;

    if( (proc=GetProcAddress(advapilib, "AddAccessDeniedAce")) )
      addaccessdeniedace=(addaccessdeniedacetype)proc;

    if( (proc=GetProcAddress(advapilib, "AddAuditAccessAce")) )
      addauditaccessace=(addauditaccessacetype)proc;

    if( (proc=GetProcAddress(advapilib, "GetLengthSid")) )
      getlengthsid=(getlengthsidtype)proc;

    if(lookupaccountname &&
       equalsid &&
       getlengthsid)
    {
      start_new_program();
      set_init_callback(init_sid);
      set_exit_callback(exit_sid);
      ADD_STORAGE(PSID);
      ADD_FUNCTION("`==", f_sid_eq, tFunc(tMix, tInt01), ID_PROTECTED);
      ADD_FUNCTION("account",f_sid_account,
                   tFunc(tOr(tStr, tVoid), tArr(tOr(tStr, tInt))), 0);
      add_program_constant("SID",sid_program=end_program(),0);

      ADD_FUNCTION("LookupAccountName",f_LookupAccountName,
                   tFunc(tOr(tStr, tZero) tStr,tArray),0);

      ADD_FUNCTION("SetNamedSecurityInfo",f_SetNamedSecurityInfo,
                   tFunc(tStr tMap(tStr,tMix), tInt),0);
      ADD_FUNCTION("GetNamedSecurityInfo",f_GetNamedSecurityInfo,
		   tFunc(tStr tOr(tVoid,tInt),tMapping),0);

      /* FIXME: add ACE constants */
    }

  }

  /* NetUserGetInfo only exists on NT, link it dynamically */
  if( (netapilib=LoadLibrary("netapi32")) )
  {
    FARPROC proc;
    if( (proc=GetProcAddress(netapilib, "NetApiBufferFree")) )
    {
      netapibufferfree=(netapibufferfreetype)proc;

      if( (proc=GetProcAddress(netapilib, "NetUserGetInfo")) )
      {
	netusergetinfo=(netusergetinfotype)proc;

	/* function(string,string,int|void:string|array(string|int)) */
	ADD_FUNCTION("NetUserGetInfo",f_NetUserGetInfo,
                     tFunc(tStr tOr(tStr, tZero) tOr(tInt, tVoid),
			   tOr(tStr,tArr(tOr(tStr,tInt)))),0);

	SIMPCONST(USER_PRIV_GUEST);
	SIMPCONST(USER_PRIV_USER);
	SIMPCONST(USER_PRIV_ADMIN);

	SIMPCONST(UF_SCRIPT);
	SIMPCONST(UF_ACCOUNTDISABLE);
	SIMPCONST(UF_HOMEDIR_REQUIRED);
	SIMPCONST(UF_PASSWD_NOTREQD);
	SIMPCONST(UF_PASSWD_CANT_CHANGE);
	SIMPCONST(UF_LOCKOUT);
	SIMPCONST(UF_DONT_EXPIRE_PASSWD);

	SIMPCONST(UF_NORMAL_ACCOUNT);
	SIMPCONST(UF_TEMP_DUPLICATE_ACCOUNT);
	SIMPCONST(UF_WORKSTATION_TRUST_ACCOUNT);
	SIMPCONST(UF_SERVER_TRUST_ACCOUNT);
	SIMPCONST(UF_INTERDOMAIN_TRUST_ACCOUNT);

	SIMPCONST(AF_OP_PRINT);
	SIMPCONST(AF_OP_COMM);
	SIMPCONST(AF_OP_SERVER);
	SIMPCONST(AF_OP_ACCOUNTS);
      }

      if( (proc=GetProcAddress(netapilib, "NetUserEnum")) )
      {
	netuserenum=(netuserenumtype)proc;

	/* function(string|int|void,int|void,int|void:array(string|array(string|int))) */
	ADD_FUNCTION("NetUserEnum",f_NetUserEnum,
                     tFunc(tOr3(tStr, tZero, tVoid)
                           tOr(tInt, tVoid)
                           tOr(tInt, tVoid),
                           tArr(tOr(tStr, tArr(tOr(tStr, tInt))))),0);

	SIMPCONST(FILTER_TEMP_DUPLICATE_ACCOUNT);
	SIMPCONST(FILTER_NORMAL_ACCOUNT);
	SIMPCONST(FILTER_INTERDOMAIN_TRUST_ACCOUNT);
	SIMPCONST(FILTER_WORKSTATION_TRUST_ACCOUNT);
	SIMPCONST(FILTER_SERVER_TRUST_ACCOUNT);
      }

      if( (proc=GetProcAddress(netapilib, "NetGroupEnum")) )
      {
	netgroupenum=(netgroupenumtype)proc;

	/* function(string|int|void,int|void:array(string|array(string|int))) */
  	ADD_FUNCTION("NetGroupEnum",f_NetGroupEnum,
                     tFunc(tOr3(tStr, tZero, tVoid) tOr(tInt, tVoid),
			   tArr(tOr(tStr,tArr(tOr(tStr,tInt))))), 0);

	SIMPCONST(SE_GROUP_ENABLED_BY_DEFAULT);
	SIMPCONST(SE_GROUP_MANDATORY);
	SIMPCONST(SE_GROUP_OWNER);
      }

      if( (proc=GetProcAddress(netapilib, "NetLocalGroupEnum")) )
      {
	netlocalgroupenum=(netgroupenumtype)proc;

	/* function(string|int|void,int|void:array(array(string|int))) */
  	ADD_FUNCTION("NetLocalGroupEnum",f_NetLocalGroupEnum,
                     tFunc(tOr3(tStr, tZero, tVoid) tOr(tInt, tVoid),
			   tArr(tArr(tOr(tStr,tInt)))), 0);
      }

      if( (proc=GetProcAddress(netapilib, "NetUserGetGroups")) )
      {
	netusergetgroups=(netusergetgroupstype)proc;

	/* function(string|int,string,int|void:array(string|array(int|string))) */
 	ADD_FUNCTION("NetUserGetGroups",f_NetUserGetGroups,
                     tFunc(tOr(tStr, tZero) tStr tOr(tInt, tVoid),
			   tArr(tOr(tStr,tArr(tOr(tInt,tStr))))), 0);
      }

      if( (proc=GetProcAddress(netapilib, "NetUserGetLocalGroups")) )
      {
	netusergetlocalgroups=(netusergetlocalgroupstype)proc;

	/* function(string|int,string,int|void,int|void:array(string)) */
 	ADD_FUNCTION("NetUserGetLocalGroups",f_NetUserGetLocalGroups,
                     tFunc(tOr(tStr, tZero) tStr tOr(tInt, tVoid)
                           tOr(tInt, tVoid), tArr(tStr)), 0);

	SIMPCONST(LG_INCLUDE_INDIRECT);
      }

      if( (proc=GetProcAddress(netapilib, "NetGroupGetUsers")) )
      {
	netgroupgetusers=(netgroupgetuserstype)proc;

	/* function(string|int,string,int|void:array(string|array(int|string))) */
 	ADD_FUNCTION("NetGroupGetUsers",f_NetGroupGetUsers,
                     tFunc(tOr(tStr, tZero) tStr tOr(tInt, tVoid),
			   tArr(tOr(tStr,tArr(tOr(tInt,tStr))))), 0);
      }

      if( (proc=GetProcAddress(netapilib, "NetLocalGroupGetMembers")) )
      {
	netlocalgroupgetmembers=(netgroupgetuserstype)proc;

	/* function(string|int,string,int|void:array(string|array(int|string))) */
 	ADD_FUNCTION("NetLocalGroupGetMembers",f_NetLocalGroupGetMembers,
                     tFunc(tOr(tStr, tZero) tStr tOr(tInt, tVoid),
			   tArr(tOr(tStr,tArr(tOr(tInt,tStr))))), 0);

	SIMPCONST(SidTypeUser);
	SIMPCONST(SidTypeGroup);
	SIMPCONST(SidTypeDomain);
	SIMPCONST(SidTypeAlias);
	SIMPCONST(SidTypeWellKnownGroup);
	SIMPCONST(SidTypeDeletedAccount);
	SIMPCONST(SidTypeInvalid);
	SIMPCONST(SidTypeUnknown);
      }

      if( (proc=GetProcAddress(netapilib, "NetGetDCName")) )
      {
	netgetdcname=(netgetdcnametype)proc;

	/* function(string|int,string:string) */
 	ADD_FUNCTION("NetGetDCName",f_NetGetDCName,
                     tFunc(tOr(tStr, tZero) tStr, tStr), 0);
      }

      if( (proc=GetProcAddress(netapilib, "NetGetAnyDCName")) )
      {
	netgetanydcname=(netgetdcnametype)proc;

	/* function(string|int,string:string) */
 	ADD_FUNCTION("NetGetAnyDCName",f_NetGetAnyDCName,
                     tFunc(tOr(tStr, tZero) tStr, tStr), 0);
      }

      /* FIXME: On windows 9x, netsessionenum is located in svrapi.lib */
      if( (proc=GetProcAddress(netapilib, "NetSessionEnum")) )
      {
	netsessionenum=(netsessionenumtype)proc;

	/* function(string,string,string,int:array(int|string)) */
 	ADD_FUNCTION("NetSessionEnum",f_NetSessionEnum,
                     tFunc(tOr(tStr, tZero) tOr(tStr, tZero)
                           tOr(tStr, tZero) tInt, tArr(tOr(tInt, tStr))), 0);

        SIMPCONST(SESS_GUEST);
        SIMPCONST(SESS_NOENCRYPTION);
      }

      if( (proc=GetProcAddress(netapilib, "NetWkstaUserEnum")) )
      {
	netwkstauserenum=(netwkstauserenumtype)proc;

	/* function(string,int:array(mixed)) */
 	ADD_FUNCTION("NetWkstaUserEnum",f_NetWkstaUserEnum,
                     tFunc(tOr(tStr, tZero) tInt, tArray), 0);
      }
    }
  }

  /* secur32.dll is named security.dll on NT4, link it dynamically */
  securlib=LoadLibrary("secur32");
  if (!securlib)
    securlib=LoadLibrary("security");

  if (securlib)
  {
    FARPROC proc;

#define LOAD_SECUR_FN(VAR, FN) do { VAR=0; \
                                    if((proc=GetProcAddress(securlib, #FN))) \
                                      VAR=(PIKE_CONCAT(VAR,type))proc; \
                               } while(0)

    LOAD_SECUR_FN(querysecuritypackageinfo, QuerySecurityPackageInfoW);
    LOAD_SECUR_FN(acquirecredentialshandle, AcquireCredentialsHandleW);
    LOAD_SECUR_FN(freecredentialshandle, FreeCredentialsHandle);
    LOAD_SECUR_FN(acceptsecuritycontext, AcceptSecurityContext);
    LOAD_SECUR_FN(completeauthtoken, CompleteAuthToken);
    LOAD_SECUR_FN(deletesecuritycontext, DeleteSecurityContext);
    LOAD_SECUR_FN(freecontextbuffer, FreeContextBuffer);
    LOAD_SECUR_FN(querycontextattributes, QueryContextAttributesW);

    if( querysecuritypackageinfo && acquirecredentialshandle &&
        freecredentialshandle && acceptsecuritycontext &&
        completeauthtoken && deletesecuritycontext &&
        freecontextbuffer && querycontextattributes )
    {
      start_new_program();
      set_init_callback(init_sctx);
      set_exit_callback(exit_sctx);
      ADD_STORAGE(struct sctx_storage);
      ADD_FUNCTION("create",f_sctx_create,tFunc(tStr,tVoid),0);
      ADD_FUNCTION("gen_context",f_sctx_gencontext,
		   tFunc(tStr,tArr(tOr(tStr,tInt))),0);
      ADD_FUNCTION("get_last_context",f_sctx_getlastcontext,
		   tFunc(tNone,tArr(tOr(tStr,tInt))),0);
      ADD_FUNCTION("is_done",f_sctx_isdone,tFunc(tNone,tInt),0);
      ADD_FUNCTION("type",f_sctx_type,tFunc(tNone,tStr),0);
      ADD_FUNCTION("get_username",f_sctx_getusername,tFunc(tNone,tStr),0);
      ADD_FUNCTION("get_last_error",f_sctx_getlasterror,tFunc(tNone,tInt),0);
      add_program_constant("SecurityContext",sctx_program=end_program(),0);
    }
  }

  ADD_FUNCTION("GetComputerName",f_GetComputerName,tFunc(tVoid, tStr), 0);
#ifdef HAVE_GETUSERNAME
  ADD_FUNCTION("GetUserName",f_GetUserName,tFunc(tVoid, tStr), 0);
#endif /* HAVE_GETUSERNAME */

}

void exit_nt_system_calls(void)
{
  if(token_program)
  {
    free_program(token_program);
    token_program=0;
  }
  if(sid_program)
  {
    free_program(sid_program);
    sid_program=0;
  }
  if(advapilib)
  {
    if(FreeLibrary(advapilib))
    {
      advapilib=0;
      logonuser=0;
      getlengthsid=0;
      initializeacl=0;
      addaccessallowedace=0;
      addaccessdeniedace=0;
      addauditaccessace=0;
      getnamedsecurityinfo=0;
      setnamedsecurityinfo=0;
      lookupaccountname=0;
      equalsid=0;
    }
  }
  if(netapilib)
  {
    if(FreeLibrary(netapilib))
    {
      netapilib=0;
      netusergetinfo=0;
      netuserenum=0;
      netwkstauserenum=0;
      netusergetgroups=0;
      netusergetlocalgroups=0;
      netgroupenum=0;
      netlocalgroupenum=0;
      netapibufferfree=0;
      netgroupgetusers=0;
      netlocalgroupgetmembers=0;
      netgetdcname=0;
      netgetanydcname=0;
    }
  }
  if(sctx_program)
  {
    free_program(sctx_program);
    sctx_program=0;
  }
  if(securlib)
  {
    if(FreeLibrary(securlib))
    {
      querysecuritypackageinfo=0;
      acquirecredentialshandle=0;
      freecredentialshandle=0;
      acceptsecuritycontext=0;
      completeauthtoken=0;
      deletesecuritycontext=0;
      freecontextbuffer=0;
      querycontextattributes=0;
    }
  }
}


#endif

/*! @endmodule
 */
