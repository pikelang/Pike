/*
 * $Id: nt.c,v 1.13 1999/08/14 09:00:42 hubbe Exp $
 *
 * NT system calls for Pike
 *
 * Fredrik Hubinette, 1998-08-22
 */

#include "global.h"

#include "system_machine.h"
#include "system.h"

#ifdef __NT__

#include "program.h"
#include "stralloc.h"
#include "threads.h"
#include "module_support.h"
#include "array.h"
#include "mapping.h"
#include "constants.h"
#include "builtin_functions.h"

#include <winsock.h>
#include <windows.h>
#include <winbase.h>
#include <lm.h>
#include <accctrl.h>

static void f_cp(INT32 args)
{
  char *from, *to;
  int ret;
  get_all_args("cp",args,"%s%s",&from,&to);
  ret=CopyFile(from, to, 0);
  if(!ret) errno=GetLastError();
  pop_n_elems(args);
  push_int(ret);
}

void f_RegGetValue(INT32 args)
{
  long ret;
  INT32 hkey;
  HKEY new_key;
  char *key, *ind;
  DWORD len,type;
  char buffer[8192];
  len=sizeof(buffer)-1;
  get_all_args("RegQueryValue",args,"%d%s%s",&hkey,&key,&ind);
  ret=RegOpenKeyEx((HKEY)hkey, (LPCTSTR)key, 0, KEY_READ,  &new_key);
  if(ret != ERROR_SUCCESS)
    error("RegOpenKeyEx failed with error %d\n",ret);

  ret=RegQueryValueEx(new_key,ind, 0, &type, buffer, &len);
  RegCloseKey(new_key);

  if(ret==ERROR_SUCCESS)
  {
    pop_n_elems(args);
    switch(type)
    {
      case REG_RESOURCE_LIST:
      case REG_NONE:
      case REG_LINK:
      case REG_BINARY:
	push_string(make_shared_binary_string(buffer,len));
	break;

      case REG_SZ:
	push_string(make_shared_binary_string(buffer,len-1));
	break;

      case REG_EXPAND_SZ:
	type=ExpandEnvironmentStrings((LPCTSTR)buffer,
				      buffer+len,
				      sizeof(buffer)-len-1);
	if(type>sizeof(buffer)-len-1 || !type)
	  error("RegGetValue: Failed to expand data.\n");
	push_string(make_shared_string(buffer+len));
	break;

      case REG_MULTI_SZ:
	push_string(make_shared_binary_string(buffer,len-1));
	push_string(make_shared_binary_string("\000",1));
	f_divide(2);
	break;

      case REG_DWORD_LITTLE_ENDIAN:
	push_int(EXTRACT_UCHAR(buffer)+
	  (EXTRACT_UCHAR(buffer+1)<<1)+
	  (EXTRACT_UCHAR(buffer+2)<<2)+
	  (EXTRACT_UCHAR(buffer+3)<<3));
	break;

      case REG_DWORD_BIG_ENDIAN:
	push_int(EXTRACT_UCHAR(buffer+3)+
	  (EXTRACT_UCHAR(buffer+2)<<1)+
	  (EXTRACT_UCHAR(buffer+1)<<2)+
	  (EXTRACT_UCHAR(buffer)<<3));
	break;

      default:
	error("RegGetValue: cannot handle this data type.\n");
    }
  }else{
    error("RegQueryValueEx failed with error %d\n",ret);
  }
}

static struct program *token_program;

#define THIS_TOKEN (*(HANDLE *)(fp->current_storage))

typedef BOOL (WINAPI *logonusertype)(LPSTR,LPSTR,LPSTR,DWORD,DWORD,PHANDLE);
typedef DWORD (WINAPI *getlengthsidtype)(PSID);

static logonusertype logonuser;
static getlengthsidtype getlengthsid;

#define LINKFUNC(RET,NAME,TYPE) \
  typedef RET (WINAPI * PIKE_CONCAT(NAME,type)) TYPE ; \
  static PIKE_CONCAT(NAME,type) NAME

LINKFUNC(BOOL,equalsid, (PSID,PSID) );
LINKFUNC(BOOL,lookupaccountname,
         (LPCTSTR,LPCTSTR,PSID,LPDWORD,LPTSTR,LPDWORD,PSID_NAME_USE) );
LINKFUNC(BOOL,lookupaccountsid,
         (LPCTSTR,PSID,LPTSTR,LPDWORD,LPTSTR,LPDWORD,PSID_NAME_USE) );
LINKFUNC(BOOL,setnamedsecurityinfo,
         (LPTSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID,PSID,PACL,PACL) );
LINKFUNC(DWORD,getnamedsecurityinfo,
         (LPTSTR,SE_OBJECT_TYPE,SECURITY_INFORMATION,PSID*,PSID*,PACL*,PACL*,PSECURITY_DESCRIPTOR) );

LINKFUNC(BOOL,initializeacl, (PACL,DWORD,DWORD) );
LINKFUNC(BOOL,addaccessallowedace, (PACL,DWORD,DWORD,PSID) );
LINKFUNC(BOOL,addaccessdeniedace, (PACL,DWORD,DWORD,PSID) );
LINKFUNC(BOOL,addauditaccessace, (PACL,DWORD,DWORD,PSID,BOOL,BOOL) );

HINSTANCE advapilib;

#define THIS_PSID (*(PSID *)fp->current_storage)
static struct program *sid_program;
static void init_sid(struct object *o)
{
  THIS_PSID=0;
}

static void exit_sid(struct object *o)
{
  if(THIS_PSID)
  {
    free((char *)THIS_PSID);
    THIS_PSID=0;
  }
}

static void f_sid_eq(INT32 args)
{
  check_all_args("system.SID->`==",args,BIT_MIXED,0);
  if(sp[-1].type == T_OBJECT)
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

static void f_sid_account(INT32 args)
{
  char foo[1];
  DWORD namelen=0;
  DWORD domainlen=0;
  char *sys=0;
  SID_NAME_USE type;

  check_all_args("SID->account",args,BIT_STRING|BIT_VOID, 0);
  if(args) sys=sp[-1].u.string->str;
  
  if(!THIS_PSID) error("SID->account on uninitialized SID.\n");
  lookupaccountsid(sys,
		   THIS_PSID,
		   foo,
		   &namelen,
		   foo,
		   &domainlen,
		   &type);

  
  if(namelen && domainlen)
  {
    struct pike_string *dom=begin_shared_string(domainlen-1);
    struct pike_string *name=begin_shared_string(namelen-1);
    
    if(lookupaccountsid(sys,
			 THIS_PSID,
			 STR0(name),
			 &namelen,
			 STR0(dom),
			 &domainlen,
			 &type))
    {
      push_string(end_shared_string(name));
      push_string(end_shared_string(dom));
      push_int(type);
      f_aggregate(3);
      return;
    }
    free((char *)dom);
    free((char *)name);
  }
  errno=GetLastError();
  pop_n_elems(args);
  push_array(allocate_array(3));
  
}


void f_LogonUser(INT32 args)
{
  LPTSTR username, domain, pw;
  DWORD logontype, logonprovider;
  HANDLE x;
  BOOL ret;
    
  check_all_args("System.LogonUser",args,
		 BIT_STRING, BIT_INT | BIT_STRING, BIT_STRING,
		 BIT_INT | BIT_VOID, BIT_INT | BIT_VOID,0);

  username=(LPTSTR)sp[-args].u.string->str;

  if(sp[1-args].type==T_STRING)
    domain=(LPTSTR)sp[1-args].u.string->str;
  else
    domain=0;

  pw=(LPTSTR)sp[2-args].u.string->str;

  logonprovider=LOGON32_PROVIDER_DEFAULT;
  logontype=LOGON32_LOGON_NETWORK;

  switch(args)
  {
    default: logonprovider=sp[4-args].u.integer;
    case 4:  logontype=sp[3-args].u.integer;
    case 3:
    case 2:
    case 1:
    case 0: break;
  }

  THREADS_ALLOW();
  ret=logonuser(username, domain, pw, logontype, logonprovider, &x);
  THREADS_DISALLOW();
  if(ret)
  {
    struct object *o;
    pop_n_elems(args);
    o=low_clone(token_program);
    (*(HANDLE *)(o->storage))=x;
    push_object(o);
  }else{
    errno=GetLastError();
    pop_n_elems(args);
    push_int(0);
  }
}

static void init_token(struct object *o)
{
  THIS_TOKEN=INVALID_HANDLE_VALUE;
}

static void exit_token(struct object *o)
{
  CloseHandle(THIS_TOKEN);
  THIS_TOKEN=INVALID_HANDLE_VALUE;
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
      error("Unsupported USERINFO level.\n");
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
      error("Unsupported GROUPINFO level.\n");
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
      error("Unsupported LOCALGROUPINFO level.\n");
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
      error("Unsupported GROUPUSERSINFO level.\n");
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
      error("Unsupported LOCALGROUPUSERSINFO level.\n");
  }
}

static void low_encode_localgroup_members_info_0(LOCALGROUP_MEMBERS_INFO_0 *tmp)
{

#define SAFE_PUSH_SID(X) do {			\
  if(getlengthsid && (X) && sid_program) {	\
    int lentmp=getlengthsid( (X) );		\
    PSID psidtmp=(PSID)xalloc(lentmp);		\
    struct object *o=low_clone(sid_program);	\
    MEMCPY( psidtmp, (X), lentmp);		\
    (*(PSID *)(o->storage))=psidtmp;		\
    push_object(o);				\
  } else {					\
    push_int(0);                                \
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
      error("Unsupported LOCALGROUPMEMBERSINFO level.\n");
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
typedef NET_API_STATUS (WINAPI *netapibufferfreetype)(LPVOID);

static netusergetinfotype netusergetinfo;
static netuserenumtype netuserenum;
static netusergetgroupstype netusergetgroups;
static netusergetlocalgroupstype netusergetlocalgroups;
static netgroupenumtype netgroupenum, netlocalgroupenum;
static netgroupgetuserstype netgroupgetusers, netlocalgroupgetmembers;
static netapibufferfreetype netapibufferfree;
HINSTANCE netapilib;


void f_NetUserGetInfo(INT32 args)
{
  char *to_free1=NULL,*to_free2=NULL;
  BYTE *tmp=0;
  DWORD level;
  LPWSTR server, user;
  NET_API_STATUS ret;

  check_all_args("NetUserGetInfo",args,BIT_STRING|BIT_INT, BIT_STRING, BIT_VOID | BIT_INT, 0);

  if(args>2)
    level=sp[2-args].u.integer;
  else
    level=1;

  switch(level)
  {
    case 0: case 1: case 2: case 3: case 10: case 11: case 20:
      break;
    default:
      error("Unsupported information level in NetUserGetInfo.\n");
  }

  if(sp[-args].type==T_STRING)
  {
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);
    if(!server)
      error("NetUserGetInfo, server name string is too wide.\n");
  }else{
    server=NULL;
  }
  
  user=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);
  if(!user)
  {
    if(to_free1) free(to_free1);
    error("NetUserGetInfo, user name string is too wide.\n");
  }

  THREADS_ALLOW();
  ret=netusergetinfo(server,user,level,&tmp);
  THREADS_DISALLOW();

  pop_n_elems(args);
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);

  switch(ret)
  {
    case ERROR_ACCESS_DENIED:
      error("NetGetUserInfo: Access denied.\n");
      break;

    case NERR_InvalidComputer:
      error("NetGetUserInfo: Invalid computer.\n");
      break;

    case NERR_UserNotFound:
      push_int(0);
      return;

    case NERR_Success:
      encode_user_info(tmp,level);
      netapibufferfree(tmp);
      return;

    default:
      error("NetGetUserInfo: Unknown error %d\n",ret);
  }
}

void f_NetUserEnum(INT32 args)
{
  char *to_free1=NULL;
  DWORD level=0;
  DWORD filter=0;
  LPWSTR server=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD resume=0;

  /*  fprintf(stderr,"before: sp=%p args=%d (base=%p)\n",sp,args,sp-args); */

  check_all_args("NetUserEnum",args,BIT_STRING|BIT_INT|BIT_VOID, BIT_INT|BIT_VOID,BIT_INT|BIT_VOID,0);

  switch(args)
  {
    default:filter=sp[2-args].u.integer;
    case 2: level=sp[1-args].u.integer;
      switch(level)
      {
	case 0: case 1: case 2: case 3: case 10: case 11: case 20:
	  break;
	default:
	  error("Unsupported information level in NetUserGetInfo.\n");
      }

    case 1:
      if(sp[-args].type==T_STRING)
	server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

    case 0: break;
  }

  pop_n_elems(args);

  /*  fprintf(stderr,"now: sp=%p\n",sp); */

  while(1)
  {
    DWORD read=0, total=0;
    NET_API_STATUS ret;
    LPBYTE buf=0,ptr;

    THREADS_ALLOW();
    ret=netuserenum(server,
		    level,
		    filter,
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
      case ERROR_ACCESS_DENIED:
	if(to_free1) free(to_free1);
	error("NetGetUserInfo: Access denied.\n");
	break;
	
      case NERR_InvalidComputer:
	if(to_free1) free(to_free1);
	error("NetGetUserInfo: Invalid computer.\n");
	break;

      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_user_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_user_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
    }
    break;
  }
  if(to_free1) free(to_free1);
}


void f_NetGroupEnum(INT32 args)
{
  char *to_free1=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD resume=0;

  check_all_args("NetGroupEnum",args,BIT_STRING|BIT_INT|BIT_VOID, BIT_INT|BIT_VOID,0);

  if(args && sp[-args].type==T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && sp[1-args].type==T_INT) {
    level = sp[1-args].u.integer;
    switch(level)
    {
      case 0: case 1: case 2:
	break;
      default:
	error("Unsupported information level in NetGroupEnum.\n");
    }
  }

  pop_n_elems(args);

  while(1)
  {
    DWORD read=0, total=0;
    NET_API_STATUS ret;
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

    switch(ret)
    {
      case ERROR_ACCESS_DENIED:
	if(to_free1) free(to_free1);
	error("NetGroupEnum: Access denied.\n");
	break;
	
      case NERR_InvalidComputer:
	if(to_free1) free(to_free1);
	error("NetGroupEnum: Invalid computer.\n");
	break;

      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_group_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_group_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
    }
    break;
  }
  if(to_free1) free(to_free1);
}

void f_NetLocalGroupEnum(INT32 args)
{
  char *to_free1=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD resume=0;

  check_all_args("NetLocalGroupEnum",args,BIT_STRING|BIT_INT|BIT_VOID, BIT_INT|BIT_VOID,0);

  if(args && sp[-args].type==T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && sp[1-args].type==T_INT) {
    level = sp[1-args].u.integer;
    switch(level)
    {
      case 0: case 1:
	break;
      default:
	error("Unsupported information level in NetLocalGroupEnum.\n");
    }
  }

  pop_n_elems(args);

  while(1)
  {
    DWORD read=0, total=0;
    NET_API_STATUS ret;
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

    switch(ret)
    {
      case ERROR_ACCESS_DENIED:
	if(to_free1) free(to_free1);
	error("NetLocalGroupEnum: Access denied.\n");
	break;
	
      case NERR_InvalidComputer:
	if(to_free1) free(to_free1);
	error("NetLocalGroupEnum: Invalid computer.\n");
	break;

      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_localgroup_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_localgroup_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
    }
    break;
  }
  if(to_free1) free(to_free1);
}

void f_NetUserGetGroups(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL, *tmp_user;
  DWORD level=0;
  LPWSTR server=NULL;
  LPWSTR user=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD read=0, total=0;
  NET_API_STATUS ret;
  LPBYTE buf=0,ptr;

  check_all_args("NetUserGetGroups",args,BIT_STRING|BIT_INT, BIT_STRING,BIT_INT|BIT_VOID, 0);

  if(args>0 && sp[-args].type==T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && sp[-args+1].type==T_STRING)
    user=(LPWSTR)require_wstring1(sp[-args+1].u.string,&to_free2);

  if(args>2 && sp[2-args].type==T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0: case 1:
	break;
      default:
	error("Unsupported information level in NetUserGetGroups.\n");
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
  
  switch(ret)
  {
  case ERROR_ACCESS_DENIED:
    if(to_free1) free(to_free1);
    if(to_free2) free(to_free2);
    error("NetUserGetGroups: Access denied.\n");
    break;
    
  case NERR_InvalidComputer:
    if(to_free1) free(to_free1);
    if(to_free2) free(to_free2);
    error("NetUserGetGroups: Invalid computer.\n");
    break;
    
  case NERR_Success:
    ptr=buf;
    for(e=0;e<read;e++)
    {
      encode_group_users_info(ptr,level);
      a->item[pos]=sp[-1];
      sp--;
      pos++;
      if(pos>=a->size) break;
      ptr+=sizeof_group_users_info(level);
    }
    netapibufferfree(buf);
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

void f_NetUserGetLocalGroups(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL, *tmp_user;
  DWORD level=0;
  DWORD flags=0;
  LPWSTR server=NULL;
  LPWSTR user=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD read=0, total=0;
  NET_API_STATUS ret;
  LPBYTE buf=0,ptr;

  check_all_args("NetUserGetLocalGroups",args,BIT_STRING|BIT_INT, BIT_STRING,BIT_INT|BIT_VOID, BIT_INT|BIT_VOID, 0);

  if(args>0 && sp[-args].type==T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && sp[-args+1].type==T_STRING)
    user=(LPWSTR)require_wstring1(sp[-args+1].u.string,&to_free2);

  if(args>2 && sp[2-args].type==T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0:
	break;
      default:
	error("Unsupported information level in NetUserGetLocalGroups.\n");
    }
  }

  if(args>3 && sp[3-args].type==T_INT)
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
  case ERROR_ACCESS_DENIED:
    if(to_free1) free(to_free1);
    if(to_free2) free(to_free2);
    error("NetUserGetLocalGroups: Access denied.\n");
    break;
    
  case NERR_InvalidComputer:
    if(to_free1) free(to_free1);
    if(to_free2) free(to_free2);
    error("NetUserGetLocalGroups: Invalid computer.\n");
    break;
    
  case NERR_Success:
    ptr=buf;
    for(e=0;e<read;e++)
    {
      encode_localgroup_users_info(ptr,level);
      a->item[pos]=sp[-1];
      sp--;
      pos++;
      if(pos>=a->size) break;
      ptr+=sizeof_localgroup_users_info(level);
    }
    netapibufferfree(buf);
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

void f_NetGroupGetUsers(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  LPWSTR group=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD resume=0;

  check_all_args("NetGroupGetUsers",args,BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING, BIT_INT|BIT_VOID,0);

  if(args && sp[-args].type==T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && sp[1-args].type==T_STRING)
    group=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);

  if(args>2 && sp[2-args].type==T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0: case 1:
	break;
      default:
	error("Unsupported information level in NetGroupGetUsers.\n");
    }
  }

  pop_n_elems(args);

  while(1)
  {
    DWORD read=0, total=0;
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
      case ERROR_ACCESS_DENIED:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetGroupGetUsers: Access denied.\n");
	break;
	
      case NERR_InvalidComputer:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetGroupGetUsers: Invalid computer.\n");
	break;

      case NERR_GroupNotFound:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetGroupGetUsers: Group not found.\n");
	break;

      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_group_users_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_group_users_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
    }
    break;
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

void f_NetLocalGroupGetMembers(INT32 args)
{
  char *to_free1=NULL, *to_free2=NULL, *tmp_server=NULL;
  DWORD level=0;
  LPWSTR server=NULL;
  LPWSTR group=NULL;
  INT32 pos=0,e;
  struct array *a=0;
  DWORD resume=0;

  check_all_args("NetLocalGroupGetMembers",args,BIT_STRING|BIT_INT|BIT_VOID, BIT_STRING, BIT_INT|BIT_VOID,0);

  if(args && sp[-args].type==T_STRING)
    server=(LPWSTR)require_wstring1(sp[-args].u.string,&to_free1);

  if(args>1 && sp[1-args].type==T_STRING)
    group=(LPWSTR)require_wstring1(sp[1-args].u.string,&to_free2);

  if(args>2 && sp[2-args].type==T_INT) {
    level = sp[2-args].u.integer;
    switch(level)
    {
      case 0: case 1: case 2: case 3:
	break;
      default:
	error("Unsupported information level in NetLocalGroupGetMembers.\n");
    }
  }

  pop_n_elems(args);

  while(1)
  {
    DWORD read=0, total=0;
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
      case ERROR_ACCESS_DENIED:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetLocalGroupGetMembers: Access denied.\n");
	break;
	
      case NERR_InvalidComputer:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetLocalGroupGetMembers: Invalid computer.\n");
	break;

      case NERR_GroupNotFound:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetLocalGroupGetMembers: Group not found.\n");
	break;

      case ERROR_NO_SUCH_ALIAS:
	if(to_free1) free(to_free1);
	if(to_free2) free(to_free2);
	error("NetLocalGroupGetMembers: No such alias.\n");
	break;

      case NERR_Success:
      case ERROR_MORE_DATA:
	ptr=buf;
	for(e=0;e<read;e++)
	{
	  encode_localgroup_members_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  pos++;
	  if(pos>=a->size) break;
	  ptr+=sizeof_localgroup_members_info(level);
	}
	netapibufferfree(buf);
	if(ret==ERROR_MORE_DATA) continue;
    }
    break;
  }
  if(to_free1) free(to_free1);
  if(to_free2) free(to_free2);
}

static void f_GetFileAttributes(INT32 args)
{
  char *file;
  DWORD ret;
  get_all_args("GetFileAttributes",args,"%s",&file);
  ret=GetFileAttributes( (LPCTSTR) file);
  pop_stack();
  errno=GetLastError();
  push_int(ret);
}

static void f_SetFileAttributes(INT32 args)
{
  char *file;
  INT32 attr,ret;
  DWORD tmp;
  get_all_args("SetFileAttributes",args,"%s%d",&file,&attr);
  tmp=attr;
  ret=SetFileAttributes( (LPCTSTR) file, tmp);
  pop_stack();
  errno=GetLastError();
  push_int(ret);
}

static void f_LookupAccountName(INT32 args)
{
  LPCTSTR sys=0;
  LPCTSTR acc;
  DWORD sidlen, domainlen;
  SID_NAME_USE tmp;
  char buffer[1];

  check_all_args("LookupAccountName",args,BIT_INT|BIT_STRING, BIT_STRING,0);
  if(sp[-args].type == T_STRING)
  {
    if(sp[-args].u.string->size_shift != 0)
       error("LookupAccountName: System name is wide string.\n");
    sys=STR0(sp[-args].u.string);
  }
  if(sp[1-args].u.string->size_shift != 0)
    error("LookupAccountName: Account name is wide string.\n");

  acc=STR0(sp[1-args].u.string);
  
  sidlen=0;
  domainlen=0;

  /* Find size required */
  lookupaccountname(sys,
		    acc,
		    (PSID)buffer,
		    &sidlen,
		    (LPTSTR)buffer,
		    &domainlen,
		    &tmp);

  if(sidlen && domainlen)
  {
    PSID sid=(PSID)xalloc(sidlen);
    struct pike_string *dom=begin_shared_string(domainlen-1);

    if(lookupaccountname(sys,
			 acc,
			 sid,
			 &sidlen,
			 (LPTSTR)(STR0(dom)),
			 &domainlen,
			 &tmp))
    {
      struct object *o;
      pop_n_elems(args);
      o=low_clone(sid_program);
      (*(PSID *)(o->storage))=sid;
      push_object(o);
      push_string(end_shared_string(dom));
      push_int(tmp);
      f_aggregate(3);
      return;
    }
    free((char *)dom);
    free((char *)sid);
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
    int e;
    check_stack(tmp.AceCount + 4);
    for(e=0;e<tmp.AceCount;e++)
    {
      void *ace;
      if(!GetAce(acl, e, &ace)) break;
      switch(((ACE_HEADER *)ace)->AceType)
      {
	case ACCESS_ALLOWED_ACE_TYPE:
	  push_constant_text("allow");
	  push_int(((ACE_HEADER *)ace)->AceFlags);
	  push_int( ((ACCESS_ALLOWED_ACE *)ace)->Mask );
	  SAFE_PUSH_SID( & ((ACCESS_ALLOWED_ACE *)ace)->SidStart );
	  f_aggregate(4);
	  break;

	case ACCESS_DENIED_ACE_TYPE:
	  push_constant_text("deny");
	  push_int(((ACE_HEADER *)ace)->AceFlags);
	  push_int( ((ACCESS_DENIED_ACE *)ace)->Mask );
	  SAFE_PUSH_SID( & ((ACCESS_DENIED_ACE *)ace)->SidStart );
	  f_aggregate(4);
	  break;

	case SYSTEM_AUDIT_ACE_TYPE:
	  push_constant_text("audit");
	  push_int(((ACE_HEADER *)ace)->AceFlags);
	  push_int( ((SYSTEM_AUDIT_ACE *)ace)->Mask );
	  SAFE_PUSH_SID( & ((SYSTEM_AUDIT_ACE *)ace)->SidStart );
	  f_aggregate(4);
	  break;

	default:
	  push_constant_text("unknown");
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
    if(arr->item[a].type != T_ARRAY)
      error("Index %d in ACL is not an array.\n",a);

    if(arr->item[a].u.array->size != 4)
      error("Index %d in ACL is not of size 4.\n",a);

    if(arr->item[a].u.array->item[0].type != T_STRING)
      error("ACE[%d][%d] is not a string.\n",a,0);

    if(arr->item[a].u.array->item[1].type != T_INT)
      error("ACE[%d][%d] is not an integer.\n",a,1);

    if(arr->item[a].u.array->item[2].type != T_INT)
      error("ACE[%d][%d] is not an integer.\n",a,2);

    if(arr->item[a].u.array->item[3].type != T_OBJECT)
      error("ACE[%d][%d] is not a SID class.\n",a,3);

    sid=(PSID *)get_storage(arr->item[a].u.array->item[3].u.object,sid_program);
    if(!sid || !*sid)
      error("ACE[%d][%d] is not a SID class.\n",a,3);

    str=arr->item[a].u.array->item[0].u.string->str;
    switch( ( str[0] << 8 ) + str[1] )
    {
      case ( 'a' << 8 ) + 'c': size += sizeof(ACCESS_ALLOWED_ACE); break;
      case ( 'd' << 8 ) + 'e': size += sizeof(ACCESS_DENIED_ACE); break;
      case ( 'a' << 8 ) + 'u': size += sizeof(SYSTEM_AUDIT_ACE); break;
      default:
	error("ACE[%d][0] is not a known ACE type.\n");
    }
    size += getlengthsid( *sid ) - sizeof(DWORD);
  }

  ret=(PACL)xalloc( size );

  if(!initializeacl(ret, size, ACL_REVISION))
    error("InitializeAcl failed!\n");

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
	  error("AddAccessAllowedAce failed!\n");
	break;

      case ( 'd' << 8 ) + 'e':
	if(!addaccessdeniedace(ret, ACL_REVISION, 
			       arr->item[a].u.array->item[2].u.integer,
			       sid))
	  error("AddAccessDeniedAce failed!\n");
	break;

      case ( 'a' << 8 ) + 'u':
	/* FIXME, what to do with the last two arguments ?? */
	if(!addauditaccessace(ret, ACL_REVISION, 
			      arr->item[a].u.array->item[2].u.integer,
			      sid,1,1))
	  error("AddAuditAccessAce failed!\n");
	break;
    }
  }
  return ret;
}

/*
 * Note, this function does not use errno!!
 * (Time to learn how to autodoc... /Hubbe)
 */
static void f_SetNamedSecurityInfo(INT32 args)
{
  struct svalue *sval;
  char *name;
  struct mapping *m;
  SECURITY_INFORMATION flags=0;
  PSID owner=0;
  PSID group=0;
  PACL dacl=0;
  PACL sacl=0;
  DWORD ret;
  SE_OBJECT_TYPE type=SE_FILE_OBJECT;

  get_all_args("SetNamedSecurityInfo",args,"%s%m",&name,&m);

  if((sval=simple_mapping_string_lookup(m, "type")))
  {
    if(sval->type != T_INT)
      error("Bad 'type' in SetNamedSecurityInfo.\n");
    type=sval->u.integer;
  }

  if((sval=simple_mapping_string_lookup(m,"owner")))
  {
    if(sval->type != T_OBJECT ||
       !get_storage(sval->u.object, sid_program))
      error("Bad 'owner' in SetNamedSecurityInfo.\n");
    owner=*(PSID *)get_storage(sval->u.object, sid_program);
    flags |= OWNER_SECURITY_INFORMATION;
  }

  if((sval=simple_mapping_string_lookup(m,"group")))
  {
    if(sval->type != T_OBJECT ||
       !get_storage(sval->u.object, sid_program))
      error("Bad 'group' in SetNamedSecurityInfo.\n");
    group=*(PSID *)get_storage(sval->u.object, sid_program);
    flags |= GROUP_SECURITY_INFORMATION;
  }

  if((sval=simple_mapping_string_lookup(m,"dacl")))
  {
    if(sval->type != T_ARRAY)
      error("Bad 'dacl' in SetNamedSecurityInfo.\n");
    dacl=decode_acl(sval->u.array);
    flags |= DACL_SECURITY_INFORMATION;
  }

  if((sval=simple_mapping_string_lookup(m,"sacl")))
  {
    if(sval->type != T_ARRAY)
      error("Bad 'sacl' in SetNamedSecurityInfo.\n");
    sacl=decode_acl(sval->u.array);
    flags |= SACL_SECURITY_INFORMATION;
  }
      
  /* FIXME, add dacl and sacl!!!! */

  ret=setnamedsecurityinfo(name,
			   type,
			   flags,
			   owner,
			   group,
			   dacl,
			   sacl);
  pop_n_elems(args);
  push_int(ret);
}

static void f_GetNamedSecurityInfo(INT32 args)
{
  PSID owner=0, group=0;
  PACL dacl=0, sacl=0;
  PSECURITY_DESCRIPTOR desc=0;
  DWORD ret;
  SECURITY_INFORMATION flags=
    OWNER_SECURITY_INFORMATION |
    GROUP_SECURITY_INFORMATION |
    DACL_SECURITY_INFORMATION;

  SE_OBJECT_TYPE type = SE_FILE_OBJECT;
  check_all_args("GetSecurityInfo",args,BIT_STRING, BIT_VOID|BIT_INT, BIT_VOID|BIT_INT, 0);

  switch(args)
  {
    default: flags=sp[2-args].u.integer;
    case 2: type=sp[1-args].u.integer;
    case 1: break;
  }

  if(!(ret=getnamedsecurityinfo(sp[-args].u.string->str,
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
      push_constant_text("owner");
      SAFE_PUSH_SID(owner);
      tmp++;
    }
    if(group)
    {
      push_constant_text("group");
      SAFE_PUSH_SID(group);
      tmp++;
    }
    if(sacl)
    {
      push_constant_text("sacl");
      push_array( encode_acl( sacl ));
      tmp++;
    }
    if(dacl)
    {
      push_constant_text("dacl");
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

#define ADD_GLOBAL_INTEGER_CONSTANT(X,Y) \
   push_int((long)(Y)); low_add_constant(X,sp-1); pop_stack();
#define SIMPCONST(X) \
      add_integer_constant(#X,X,0);

void init_nt_system_calls(void)
{
  /* function(string,string:int) */

  ADD_FUNCTION("GetFileAttributes",f_GetFileAttributes,tFunc(tStr,tInt),0);
  ADD_FUNCTION("SetFileAttributes",f_SetFileAttributes,tFunc(tStr tInt,tInt),0);
  
  SIMPCONST(FILE_ATTRIBUTE_ARCHIVE);
  SIMPCONST(FILE_ATTRIBUTE_HIDDEN);
  SIMPCONST(FILE_ATTRIBUTE_NORMAL);
  SIMPCONST(FILE_ATTRIBUTE_OFFLINE);
  SIMPCONST(FILE_ATTRIBUTE_READONLY);
  SIMPCONST(FILE_ATTRIBUTE_SYSTEM);
  SIMPCONST(FILE_ATTRIBUTE_TEMPORARY);

  SIMPCONST(FILE_ATTRIBUTE_COMPRESSED);
  SIMPCONST(FILE_ATTRIBUTE_DIRECTORY);
  SIMPCONST(FILE_ATTRIBUTE_ENCRYPTED);
  SIMPCONST(FILE_ATTRIBUTE_REPARSE_POINT);
  SIMPCONST(FILE_ATTRIBUTE_SPARSE_FILE);
  
  ADD_FUNCTION("cp",f_cp,tFunc(tStr tStr,tInt), 0);

  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_LOCAL_MACHINE",HKEY_LOCAL_MACHINE);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CURRENT_USER",HKEY_CURRENT_USER);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_USERS",HKEY_USERS);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CLASSES_ROOT",HKEY_CLASSES_ROOT);
  
/* function(int,string,string:string|int|string*) */
  ADD_EFUN("RegGetValue",f_RegGetValue,tFunc(tInt tStr tStr,tOr3(tStr,tInt,tArr(tStr))),OPT_EXTERNAL_DEPEND);


  /* LogonUser only exists on NT, link it dynamically */
  if(advapilib=LoadLibrary("advapi32"))
  {
    FARPROC proc;
    if(proc=GetProcAddress(advapilib, "LogonUserA"))
    {
      logonuser=(logonusertype)proc;

      /* function(string,string,string,int|void,void|int:object) */
  ADD_FUNCTION("LogonUser",f_LogonUser,tFunc(tStr tStr tStr tOr(tInt,tVoid) tOr(tVoid,tInt),tObj),0);
      
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

    if(proc=GetProcAddress(advapilib, "LookupAccountNameA"))
      lookupaccountname=(lookupaccountnametype)proc;

    if(proc=GetProcAddress(advapilib, "LookupAccountSidA"))
      lookupaccountsid=(lookupaccountsidtype)proc;

    if(proc=GetProcAddress(advapilib, "SetNamedSecurityInfoA"))
      setnamedsecurityinfo=(setnamedsecurityinfotype)proc;

    if(proc=GetProcAddress(advapilib, "GetNamedSecurityInfoA"))
      getnamedsecurityinfo=(getnamedsecurityinfotype)proc;

    if(proc=GetProcAddress(advapilib, "EqualSid"))
      equalsid=(equalsidtype)proc;

    if(proc=GetProcAddress(advapilib, "InitializeAcl"))
      initializeacl=(initializeacltype)proc;

    if(proc=GetProcAddress(advapilib, "AddAccessAllowedAce"))
      addaccessallowedace=(addaccessallowedacetype)proc;

    if(proc=GetProcAddress(advapilib, "AddAccessDeniedAce"))
      addaccessdeniedace=(addaccessdeniedacetype)proc;

    if(proc=GetProcAddress(advapilib, "AddAuditAccessAce"))
      addauditaccessace=(addauditaccessacetype)proc;

    if(proc=GetProcAddress(advapilib, "GetLengthSid"))
      getlengthsid=(getlengthsidtype)proc;

    if(lookupaccountname &&
       equalsid &&
       getlengthsid)
    {
      start_new_program();
      set_init_callback(init_sid);
      set_init_callback(exit_sid);
      ADD_STORAGE(PSID);
      add_function("`==",f_sid_eq,"function(mixed:int)",0);
      add_function("account",f_sid_account,"function(:array(string|int))",0);
      add_program_constant("SID",sid_program=end_program(),0);

      add_function("LookupAccountName",f_LookupAccountName,
		   "function(string,string:array)",0);

      add_function("SetNamedSecurityInfo",f_SetNamedSecurityInfo,
		   "function(string,mapping(string:mixed):array)",0);
      add_function("GetNamedSecurityInfo",f_GetNamedSecurityInfo,
		   "function(string,void|int:mapping)",0);

      /* FIXME: add ACE constants */
    }

  }

  /* NetUserGetInfo only exists on NT, link it dynamically */
  if(netapilib=LoadLibrary("netapi32"))
  {
    FARPROC proc;
    if(proc=GetProcAddress(netapilib, "NetApiBufferFree"))
    {
      netapibufferfree=(netapibufferfreetype)proc;

      if(proc=GetProcAddress(netapilib, "NetUserGetInfo"))
      {
	netusergetinfo=(netusergetinfotype)proc;
	
	/* function(string,string,int|void:string|array(string|int)) */
  ADD_FUNCTION("NetUserGetInfo",f_NetUserGetInfo,tFunc(tStr tStr tOr(tInt,tVoid),tOr(tStr,tArr(tOr(tStr,tInt)))),0);
	
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
      
      if(proc=GetProcAddress(netapilib, "NetUserEnum"))
      {
	netuserenum=(netuserenumtype)proc;
	
	/* function(string|int|void,int|void,int|void:array(string|array(string|int))) */
  ADD_FUNCTION("NetUserEnum",f_NetUserEnum,tFunc(tOr3(tStr,tInt,tVoid) tOr(tInt,tVoid) tOr(tInt,tVoid),tArr(tOr(tStr,tArr(tOr(tStr,tInt))))),0);
	
	SIMPCONST(FILTER_TEMP_DUPLICATE_ACCOUNT);
	SIMPCONST(FILTER_NORMAL_ACCOUNT);
	SIMPCONST(FILTER_INTERDOMAIN_TRUST_ACCOUNT);
	SIMPCONST(FILTER_WORKSTATION_TRUST_ACCOUNT);
	SIMPCONST(FILTER_SERVER_TRUST_ACCOUNT);
      }

      if(proc=GetProcAddress(netapilib, "NetGroupEnum"))
      {
	netgroupenum=(netgroupenumtype)proc;
	
  	add_function("NetGroupEnum",f_NetGroupEnum,"function(string|int|void,int|void:array(string|array(string|int)))",0); 

	SIMPCONST(SE_GROUP_ENABLED_BY_DEFAULT);
	SIMPCONST(SE_GROUP_MANDATORY);
	SIMPCONST(SE_GROUP_OWNER);
      }

      if(proc=GetProcAddress(netapilib, "NetLocalGroupEnum"))
      {
	netlocalgroupenum=(netgroupenumtype)proc;
	
  	add_function("NetLocalGroupEnum",f_NetLocalGroupEnum,"function(string|int|void,int|void:array(array(string|int)))",0); 
      }

      if(proc=GetProcAddress(netapilib, "NetUserGetGroups"))
      {
	netusergetgroups=(netusergetgroupstype)proc;
	
 	add_function("NetUserGetGroups",f_NetUserGetGroups,"function(string|int,string,int|void:array(string|array(int|string)))",0); 
      }

      if(proc=GetProcAddress(netapilib, "NetUserGetLocalGroups"))
      {
	netusergetlocalgroups=(netusergetlocalgroupstype)proc;
	
 	add_function("NetUserGetLocalGroups",f_NetUserGetLocalGroups,"function(string|int,string,int|void,int|void:array(string))",0); 

	SIMPCONST(LG_INCLUDE_INDIRECT);
      }

      if(proc=GetProcAddress(netapilib, "NetGroupGetUsers"))
      {
	netgroupgetusers=(netgroupgetuserstype)proc;
	
 	add_function("NetGroupGetUsers",f_NetGroupGetUsers,"function(string|int,string,int|void:array(string|array(int|string)))",0); 
      }

      if(proc=GetProcAddress(netapilib, "NetLocalGroupGetMembers"))
      {
	netlocalgroupgetmembers=(netgroupgetuserstype)proc;
	
 	add_function("NetLocalGroupGetMembers",f_NetLocalGroupGetMembers,"function(string|int,string,int|void:array(string|array(int|string)))",0); 

	SIMPCONST(SidTypeUser);
	SIMPCONST(SidTypeGroup);
	SIMPCONST(SidTypeDomain);
	SIMPCONST(SidTypeAlias);
	SIMPCONST(SidTypeWellKnownGroup);
	SIMPCONST(SidTypeDeletedAccount);
	SIMPCONST(SidTypeInvalid);
	SIMPCONST(SidTypeUnknown);
      }
    }
  }
}

void exit_nt_system_calls(void)
{
  if(token_program)
  {
    free_program(token_program);
    token_program=0;
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
      netusergetgroups=0;
      netusergetlocalgroups=0;
      netgroupenum=0;
      netlocalgroupenum=0;
      netapibufferfree=0;
      netgroupgetusers=0;
      netlocalgroupgetmembers=0;
    }
  }
}


#endif

