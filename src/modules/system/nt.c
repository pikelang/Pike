/*
 * $Id: nt.c,v 1.7 1999/02/01 02:47:18 hubbe Exp $
 *
 * NT system calls for Pike
 *
 * Fredrik Hubinette, 1998-08-22
 */

#include "global.h"

#include "system_machine.h"
#include "system.h"

#ifdef __NT__

#include "interpret.h"
#include "object.h"
#include "program.h"
#include "svalue.h"
#include "stralloc.h"
#include "las.h"
#include "threads.h"
#include "module_support.h"
#include "array.h"

#include <winsock.h>
#include <windows.h>
#include <winbase.h>
#include <lm.h>

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

typedef BOOL WINAPI (*logonusertype)(LPSTR,LPSTR,LPSTR,DWORD,DWORD,PHANDLE);

static logonusertype logonuser;
HINSTANCE advapilib;

void f_LogonUser(INT32 args)
{
  LPTSTR username, domain, pw;
  DWORD logontype, logonprovider;
  HANDLE x;
  BOOL ret;
    
  check_all_args("System.LogonUser",args,
		 BIT_STRING, BIT_INT | BIT_STRING, BIT_STRING,
		 BIT_INT, BIT_INT | BIT_VOID,0);

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

typedef NET_API_STATUS WINAPI (*netusergetinfotype)(LPWSTR,LPWSTR,DWORD,LPBYTE *);
typedef NET_API_STATUS WINAPI (*netuserenumtype)(LPWSTR,DWORD,DWORD,LPBYTE*,DWORD,LPDWORD,LPDWORD,LPDWORD);
typedef NET_API_STATUS WINAPI (*netapibufferfreetype)(LPVOID);

static netusergetinfotype netusergetinfo;
static netuserenumtype netuserenum;
static netapibufferfreetype netapibufferfree;
HINSTANCE netapilib;


void f_NetUserGetInfo(INT32 args)
{
  char *to_free1,*to_free2;
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
  char *to_free1;
  DWORD level=0;
  DWORD filter=0;
  LPWSTR server=NULL;
  INT32 pos=0,e;
  struct array *a=0;

  fprintf(stderr,"before: sp=%p args=%d (base=%p)\n",sp,args,sp-args);

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

  fprintf(stderr,"now: sp=%p\n",sp);

  while(1)
  {
    DWORD read=0, total=0, resume=0;
    NET_API_STATUS ret;
    LPBYTE buf=0,ptr;

    fprintf(stderr,"Result: filter=%d level=%d\n",filter,level);

    THREADS_ALLOW();
    ret=netuserenum(server,
		    filter,
		    level,
		    &buf,
		    0x10000,
		    &read,
		    &total,
		    &resume);
    THREADS_DISALLOW();
    if(!a)
      push_array(a=allocate_array(total));

    fprintf(stderr,"Result: %d (%d) %d %d %p\n",ret,ret==NERR_Success,total,read,buf);

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
	  fprintf(stderr,"before: sp=%p\n",sp);
	  encode_user_info(ptr,level);
	  a->item[pos]=sp[-1];
	  sp--;
	  fprintf(stderr,"after: sp=%p\n",sp);
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

void init_nt_system_calls(void)
{
  add_function("cp",f_cp,"function(string,string:int)", 0);
#define ADD_GLOBAL_INTEGER_CONSTANT(X,Y) \
   push_int((long)(Y)); low_add_constant(X,sp-1); pop_stack();

  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_LOCAL_MACHINE",HKEY_LOCAL_MACHINE);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CURRENT_USER",HKEY_CURRENT_USER);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_USERS",HKEY_USERS);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CLASSES_ROOT",HKEY_CLASSES_ROOT);
  add_efun("RegGetValue",f_RegGetValue,"function(int,string,string:string|int|string*)",OPT_EXTERNAL_DEPEND);


  /* LogonUser only exists on NT, link it dynamically */
  if(advapilib=LoadLibrary("advapi32"))
  {
    FARPROC proc;
    if(proc=GetProcAddress(advapilib, "LogonUserA"))
    {
      logonuser=(logonusertype)proc;

      add_function("LogonUser",f_LogonUser,"function(string,string,string,int|void,void|int:object)",0);
#define SIMPCONST(X) \
      add_integer_constant(#X,X,0);
      
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
	
	add_function("NetUserGetInfo",f_NetUserGetInfo,"function(string,string,int|void:string|array(string|int))",0);
	
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
	
	add_function("NetUserEnum",f_NetUserEnum,"function(string|int|void,int|void,int|void:array(string|array(string|int)))",0);
	
	SIMPCONST(FILTER_TEMP_DUPLICATE_ACCOUNT);
	SIMPCONST(FILTER_NORMAL_ACCOUNT);
	SIMPCONST(FILTER_INTERDOMAIN_TRUST_ACCOUNT);
	SIMPCONST(FILTER_WORKSTATION_TRUST_ACCOUNT);
	SIMPCONST(FILTER_SERVER_TRUST_ACCOUNT);
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
    }
  }
  if(netapilib)
  {
    if(FreeLibrary(netapilib))
    {
      netapilib=0;
      netusergetinfo=0;
      netuserenum=0;
      netapibufferfree=0;
    }
  }
}


#endif

