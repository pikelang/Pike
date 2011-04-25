/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id$
*/

/*
 * NT crypto stuff for Pike
 */

#ifdef __NT__

/* We need to do this before including global.h /Hubbe */
/* (because global.h includes windows.h which includes wincrypt.h ) */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include "global.h"
#include "stralloc.h"
#include "interpret.h"
#include "svalue.h"
#include "object.h"
#include "pike_error.h"
#include "las.h"
#include "module_support.h"

#include <wincrypt.h>


#define sp Pike_sp

static struct program *cryptcontext_program = NULL;

struct cryptcontext_storage {
  HCRYPTPROV handle;
};

#define THIS_CRYPTCONTEXT ((struct cryptcontext_storage *)(Pike_fp->current_storage))

/*! @module Crypto
 */

static void init_cryptcontext_struct(struct object *o)
{
  struct cryptcontext_storage *c = THIS_CRYPTCONTEXT;

  c->handle = 0;
}

static void exit_cryptcontext_struct(struct object *o)
{
  struct cryptcontext_storage *c = THIS_CRYPTCONTEXT;

  if(c->handle)
    CryptReleaseContext(c->handle, 0);
}

/*! @class nt
 */

/*! @class CryptoContext
 */

/*! @decl string CryptGenRandom(int size, string|void init)
 */
static void f_CryptGenRandom(INT32 args)
{
  struct cryptcontext_storage *c = THIS_CRYPTCONTEXT;
  struct pike_string *str = NULL, *res;
  INT_TYPE siz;

  get_all_args("CryptGenRandom()", args, (args>1? "%i%S":"%i"), &siz, &str);

  if(siz == 0 && str != NULL)
    siz = DO_NOT_WARN(str->len);

  res = begin_shared_string(siz);
  if(str != NULL && siz > 0)
    memcpy(res->str, str->str, (str->len<siz? str->len : siz));
  if(CryptGenRandom(c->handle, siz, (BYTE*)res->str)) {
    pop_n_elems(args);
    push_string(end_shared_string(res));
  } else {
    pop_n_elems(args);
    free_string(end_shared_string(res));
    push_int(0);
  }
}

/*! @endclass
 */

/*! @decl CryptoContext CryptAcquireContext(string str1, string str2, @
 *!                                         int typ, int flags)
 */
static void f_CryptAcquireContext(INT32 args)
{
  char *str1=NULL, *str2=NULL;
  INT_TYPE typ, flags, fake1, fake2;
  int nullflag=0;
  HCRYPTPROV prov;

  if(args>0 && sp[-args].type == T_INT && sp[-args].u.integer == 0)
    nullflag |= 1;
  if(args>1 && sp[1-args].type == T_INT && sp[1-args].u.integer == 0)
    nullflag |= 2;

  switch(nullflag) {
  case 0:
    get_all_args("Crypto.nt.CryptAcquireContext()", args, "%s%s%i%i",
		 &str1, &str2, &typ, &flags);
    break;
  case 1:
    get_all_args("Crypto.nt.CryptAcquireContext()", args, "%i%s%i%i",
		 &fake1, &str2, &typ, &flags);
    break;
  case 2:
    get_all_args("Crypto.nt.CryptAcquireContext()", args, "%s%i%i%i",
		 &str1, &fake2, &typ, &flags);
    break;
  case 3:
    get_all_args("Crypto.nt.CryptAcquireContext()", args, "%i%i%i%i",
		 &fake1, &fake2, &typ, &flags);
    break;
  }

  if(!CryptAcquireContext(&prov, str1, str2, typ, flags)) {
    pop_n_elems(args);
    push_int(0);
    return;
  }
  
  pop_n_elems(args);
  push_object(clone_object(cryptcontext_program, 0));
  ((struct cryptcontext_storage *)get_storage(sp[-1].u.object,
					      cryptcontext_program))->handle =
    prov;
}

/*! @endclass
 */

/*! @endmodule
 */

#endif /* __NT__ */


void pike_nt_init(void)
{
#ifdef __NT__

#define SIMPCONST(X) \
      add_integer_constant(#X,X,0);

  start_new_program();

  SIMPCONST(PROV_RSA_FULL);
  SIMPCONST(PROV_RSA_SIG);
  SIMPCONST(PROV_DSS);
  SIMPCONST(PROV_FORTEZZA);
  SIMPCONST(PROV_MS_EXCHANGE);
  SIMPCONST(PROV_SSL);

  SIMPCONST(CRYPT_VERIFYCONTEXT);
  SIMPCONST(CRYPT_NEWKEYSET);
  SIMPCONST(CRYPT_DELETEKEYSET);
#ifdef CRYPT_MACHINE_KEYSET
  SIMPCONST(CRYPT_MACHINE_KEYSET);
#endif
#ifdef CRYPT_SILENT
  SIMPCONST(CRYPT_SILENT);
#endif

  add_function_constant("CryptAcquireContext",f_CryptAcquireContext,
			"function(string,string,int,int:object)", 0);

  end_class("nt", 0);

  start_new_program();
  ADD_STORAGE(struct cryptcontext_storage);
  /* function(int,string|void:string) */
  ADD_FUNCTION("CryptGenRandom", f_CryptGenRandom,
	       tFunc(tInt tOr(tString,tVoid),tString), 0);
  set_init_callback(init_cryptcontext_struct);
  set_exit_callback(exit_cryptcontext_struct);
  cryptcontext_program = end_program();

#endif /* __NT__ */
}

void pike_nt_exit(void)
{
#ifdef __NT__
  if(cryptcontext_program) {
    free_program(cryptcontext_program);
    cryptcontext_program=NULL;
  }
#endif /* __NT__ */
}
