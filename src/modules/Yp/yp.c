#include "global.h"
#include "config.h"

#if defined(HAVE_RPCSVC_YPCLNT_H) && defined(HAVE_RPCSVC_YP_PROT_H)
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_RPC_TYPES_H
#include <rpc/types.h>
#endif /* HAVE_RPC_TYPES_H */
#ifdef HAVE_RPC_RPC_H
#include <rpc/rpc.h>
#endif /* HAVE_RPC_RPC_H */
#ifdef HAVE_RPC_CLNT_H
#include <rpc/clnt.h>
#endif /* HAVE_RPC_CLNT_H */
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "stralloc.h"
#include "error.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "module_support.h"

RCSID("$Id: yp.c,v 1.17 1999/08/03 00:42:53 hubbe Exp $");

#define YPERROR(fun,err) do{ if(err) error("yp->%s(): %s\n", (fun), \
                                           yperr_string(err)); }while(0)

struct my_yp_domain
{
  char *domain;
  int last_size; /* Optimize some allocations */
};

#define this ((struct my_yp_domain *)fp->current_storage)

static void f_default_yp_domain(INT32 args)
{
  int err;
  char *ret;

  err = yp_get_default_domain(&ret);

  YPERROR( "default_yp_domain", err );

  pop_n_elems( args );
  push_text( ret );
}

static void f_server(INT32 args)
{
  int err;
  char *ret;

  err = yp_master(this->domain, sp[-1].u.string->str, &ret);

  YPERROR( "server", err );

  pop_n_elems( args );
  push_text( ret );
}

static void f_create(INT32 args)
{
  int err;
  if(!args)
  {
    f_default_yp_domain(0);
    args = 1;
  }
  check_all_args("yp->create", args, BIT_STRING,0);

  if(this->domain)
  {
    yp_unbind( this->domain );
    free(this->domain);
  }
  this->domain = strdup( sp[-args].u.string->str );
  err = yp_bind( this->domain );

  YPERROR("create", err);

  pop_n_elems(args);
}

static void f_all(INT32 args)
{
  int err, num=0;
  char *retval, *retkey;
  int retlen, retkeylen;
  char *map;
  struct mapping *res_map;
  check_all_args("yp->all", args, BIT_STRING, 0);

  map = sp[-1].u.string->str;
  res_map = allocate_mapping( (this->last_size?this->last_size+2:40) );

  if(!(err = yp_first(this->domain, map, &retkey,&retkeylen, &retval,&retlen)))
    do {
      push_string(make_shared_binary_string(retkey, retkeylen));
      push_string(make_shared_binary_string(retval, retlen));
      mapping_insert( res_map, sp-2, sp-1 );
      pop_stack(); pop_stack();

      err = yp_next(this->domain, map, retkey, retkeylen,
		    &retkey, &retkeylen, &retval, &retlen);
      num++;
    } while(!err);

  if(err != YPERR_NOMORE)
  {
    free_mapping( res_map );
    YPERROR( "all", err );
  }

  this->last_size = num;
  pop_n_elems(args);
  push_mapping( res_map );
}

static void f_map(INT32 args)
{
  int err;
  char *retval, *retkey;
  int retlen, retkeylen;
  char *map;

  struct svalue *f = &sp[-1];

  check_all_args("map", args, BIT_STRING, BIT_FUNCTION|BIT_ARRAY, 0 );
  
  map = sp[-2].u.string->str;

  if(!(err = yp_first(this->domain,map, &retkey,&retkeylen, &retval, &retlen)))
    do {
      push_string(make_shared_binary_string(retkey, retkeylen));
      push_string(make_shared_binary_string(retval, retlen));
      apply_svalue( f, 2 );

      err = yp_next(this->domain, map, retkey, retkeylen,
		    &retkey, &retkeylen, &retval, &retlen);
    } while(!err);

  if(err != YPERR_NOMORE)
    YPERROR( "all", err );

  pop_n_elems(args);
}

static void f_order(INT32 args)
{
  int err;
  YP_ORDER_TYPE ret;

  check_all_args("yp->order", args, BIT_STRING, 0);
  
  err = yp_order( this->domain, sp[-args].u.string->str, &ret);

  YPERROR("order", err );

  pop_n_elems( args );
  push_int( (INT32) ret );
}

static void f_match(INT32 args)
{
  int err;
  char *retval;
  int retlen;
  
  check_all_args("yp->match", args, BIT_STRING, BIT_STRING, 0);

  err = yp_match( this->domain, sp[-args].u.string->str,
		  sp[-args+1].u.string->str, sp[-args+1].u.string->len,
		  &retval, &retlen );

  if(err == YPERR_KEY)
  {
    pop_n_elems( args );
    push_int(0);
    sp[-1].subtype = NUMBER_UNDEFINED;
    return;
  }

  YPERROR( "match", err );

  pop_n_elems( args );
  push_string(make_shared_binary_string( retval, retlen ));
}

static void init_yp_struct( struct object *o )
{
  this->domain = 0;
  this->last_size = 0;
}

static void exit_yp_struct( struct object *o )
{
  if(this->domain)
  {
    yp_unbind( this->domain );
    free(this->domain);
  }
}


/******************** PUBLIC FUNCTIONS BELOW THIS LINE */


void pike_module_init(void)
{
  
/* function(void:string) */
  ADD_EFUN("default_yp_domain", f_default_yp_domain,tFunc(tVoid,tStr),
	   OPT_EXTERNAL_DEPEND);

  start_new_program();

  ADD_STORAGE(struct my_yp_domain);
  
  set_init_callback( init_yp_struct );
  set_exit_callback( exit_yp_struct );

  /* function(string|void:void) */
  ADD_FUNCTION("create", f_create,tFunc(tOr(tStr,tVoid),tVoid), 0);
  /* function(string:void) */
  ADD_FUNCTION("bind", f_create,tFunc(tStr,tVoid), 0);

  /* function(string,string:string) */
  ADD_FUNCTION("match", f_match,tFunc(tStr tStr,tStr), 0);
  /* function(string:string) */
  ADD_FUNCTION("server", f_server,tFunc(tStr,tStr), 0);
  /* function(string:mapping(string:string)) */
  ADD_FUNCTION("all", f_all,tFunc(tStr,tMap(tStr,tStr)), 0);
  /* function(string,function|array(function):void) */
  ADD_FUNCTION("map", f_map,tFunc(tStr tOr(tFunction,tArr(tFunction)),tVoid), 0);
  /* function(string:int) */
  ADD_FUNCTION("order", f_order,tFunc(tStr,tInt), 0);

  end_class("Domain", 0);
}

void pike_module_exit(void)
{
  
}
#else

void pike_module_init(void) {}
void pike_module_exit(void) {}

#endif
