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
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>

#include "stralloc.h"
#include "error.h"
#include "global.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "module_support.h"

#define YPERROR(fun,err) do{if(err)error("yp->%s(): %s\n", (fun), yperr_string( err ));}while(0)

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
  pop_n_elems( args );
  err = yp_get_default_domain(&ret);
  YPERROR( "dafult_yp_domain", err );
  push_text( ret );
}

static void f_server(INT32 args)
{
  int err;
  char *ret;
  err = yp_master(this->domain, sp[-1].u.string->str, &ret);
  pop_n_elems( args );
  YPERROR( "server", err );
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

  pop_n_elems(args);
  if(err != YPERR_NOMORE)
  {
    free_mapping( res_map );
    YPERROR( "all", err );
  }
  this->last_size = num;
  push_mapping( res_map );
}

void f_map(INT32 args)
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

  pop_n_elems(args);
  if(err != YPERR_NOMORE)
    YPERROR( "all", err );
}

static void f_order(INT32 args)
{
  int err;
  unsigned int ret;
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

  pop_n_elems( args );
  if(err == YPERR_KEY)
  {
    push_int(0);
    sp[-1].subtype = NUMBER_UNDEFINED;
    return;
  }

  YPERROR( "match", err );
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
  add_efun("default_yp_domain", f_default_yp_domain, "function(void:string)",
	   OPT_EXTERNAL_DEPEND);

  start_new_program();

  add_storage(sizeof(struct my_yp_domain));
  
  set_init_callback( init_yp_struct );
  set_exit_callback( exit_yp_struct );

  add_function("create", f_create, "function(string|void:void)", 0);
  add_function("bind", f_create, "function(string:void)", 0);

  add_function("match", f_match, "function(string,string:string)", 0);
  add_function("server", f_server, "function(string:string)", 0);
  add_function("all", f_all, "function(string:mapping(string:string))", 0);
  add_function("map", f_map, "function(string,function|array(function):void)", 0);
  add_function("order", f_order, "function(string:int)", 0);

  add_program_constant("Domain", end_program(), 0);
}

void pike_module_exit(void)
{
}
#else

void pike_module_init(void) {}
void pike_module_exit(void) {}

#endif
