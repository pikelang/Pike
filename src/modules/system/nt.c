/*
 * $Id: nt.c,v 1.1 1998/07/26 10:29:49 hubbe Exp $
 *
 * NT system calls for Pike
 *
 * Fredrik Hubinette, 1998-08-22
 */

#include #include "global.h"

#include "system_machine.h"
#include "system.h"

#ifdef __NT__

#include <winsock.h>
#include <windows.h>
#include <winbase.h>


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

void f_LogonUser(INT32 args)
{
  LPTSTR username, domain, pw;
  DWORD logontype, logonprovider;
  HANDLE x;
  BOOL ret;
    
  check_all_args("System.LogonUser",args,
		 BIT_STRING, BIT_INT | BIT_STRING, BIT_STRING,
		 BIT_INT, BIT_INT | BIT_VOID);

  username=(LPTSTR)sp[-args].u.string->str;

  if(sp[1-args].type==T_STRING)
    domain=(LPTSTR)sp[1-args].u.string->str;
  else
    domain=0;

  pw=(LPTSTR)sp[2-args].u.string->str;

  logonprovider=LOGON32_PROVIDER_DEFAULT;
  logintype=LOGON32_LOGON_NETWORK;

  switch(args)
  {
    default: logonprovider=sp[4-args].u.integer;
    case 4:  logontype=sp[3-args].u.integer;
    case 3:
    case 2:
    case 1:
    case 0:
  }

  THREADS_ALLOW();
  ret=LogonUser(username, domain, pw, logontype, logonprovider, &x);
  THREADS_DISALLOW();
  if(ret)
  {
    struct object *o;
    pop_n_elems(args);
    o=low_clone(token_program);
    (*(HANDLE *)o->storage)[0]=x;
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

void init_nt_system_calls(void)
{
  start_new_program();
  add_storage(sizeof(HANDLE));
  set_init_callback(init_token);
  set_exit_callback(exit_token);
  token_program=end_program();
  add_program_constant("UserToken",file_program,0);
  toek_program->flags |= PROGRAM_DESTRUCT_IMMEDIATE;

  add_function("cp",f_cp,"function(string,string:int)", 0);
#define ADD_GLOBAL_INTEGER_CONSTANT(X,Y) \
   push_int((long)(Y)); low_add_constant(X,sp-1); pop_stack();

  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_LOCAL_MACHINE",HKEY_LOCAL_MACHINE);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CURRENT_USER",HKEY_CURRENT_USER);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_USERS",HKEY_USERS);
  ADD_GLOBAL_INTEGER_CONSTANT("HKEY_CLASSES_ROOT",HKEY_CLASSES_ROOT);
  add_efun("RegGetValue",f_RegGetValue,"function(int,string,string:string|int|string*)",OPT_EXTERNAL_DEPEND);

  add_function("LogonUser",f_LogonUser,"function(string,string,string,int,void|int:object)",0);
#define SIMPCONST(X) \
  add_integer_constant(#X,X);

  SIMPCONST(LOGON32_LOGON_BATCH);
  SIMPCONST(LOGON32_LOGON_INTERACTIVE);
  SIMPCONST(LOGON32_LOGON_SERVICE);
  SIMPCONST(LOGON32_LOGON_NETWORK);
  SIMPCONST(LOGON32_PROVIDER_DEFAULT);
}

void exit_nt_system_calls(void)
{
}


#endif

