/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: yp.c,v 1.31 2004/07/01 11:35:02 nilsson Exp $
*/

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
#include "pike_error.h"
#include "pike_macros.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "module_support.h"


#define sp Pike_sp

RCSID("$Id: yp.c,v 1.31 2004/07/01 11:35:02 nilsson Exp $");

#ifdef HAVE_YPERR_STRING
#define YPERROR(e) do{ if(err) Pike_error("%s\n", yperr_string(e)); }while(0)
#else /* !HAVE_YPERR_STRING */
#define YPERROR(e) do{ if(e) Pike_error("YP error %d.\n", (e)); }while(0)
#endif /* HAVE_YPERR_STRING */

struct my_yp_domain
{
  char *domain;
  int last_size; /* Optimize some allocations */
};

#define this ((struct my_yp_domain *)Pike_fp->current_storage)

/*! @module Yp
 *!
 *! This module is an interface to the Yellow Pages functions. Yp is also
 *! known as NIS (Network Information System) and is most commonly used to
 *! distribute passwords and similar information within a network.
 */

/*! @decl string default_domain()
 *!
 *! Returns the default yp-domain.
 */
static void f_default_domain(INT32 args)
{
  int err;
  char *ret;

  err = yp_get_default_domain(&ret);

  YPERROR( err );

  pop_n_elems( args );
  push_text( ret );
}

/*! @class Domain
 */

/*! @decl string server(string map)
 *!
 *! Returns the hostname of the server serving the map @[map]. @[map]
 *! is the YP-map to search in. This must be the full map name.
 *! eg @tt{passwd.byname@} instead of just @tt{passwd@}.
 */
static void f_server(INT32 args)
{
  int err;
  char *ret, *map;

  get_all_args("server", args, "%s", &map);
  err = yp_master(this->domain, map, &ret);

  YPERROR( err );

  pop_n_elems( args );
  push_text( ret );
}

/*! @decl void create(string|void domain)
 *! @decl void bind(string domain)
 *!
 *! If @[domain] is not specified , the default domain will be used.
 *! (As returned by @[Yp.default_domain()]).
 *!
 *! If there is no YP server available for the domain, this
 *! function call will block until there is one. If no server appears
 *! in about ten minutes or so, an error will be returned. This timeout
 *! is not configurable.
 *!
 *! @seealso
 *! @[Yp.default_domain()]
 */
static void f_create(INT32 args)
{
  int err;
  if(!args)
  {
    f_default_domain(0);
    args = 1;
  }
  check_all_args("create", args, BIT_STRING,0);

  if(this->domain)
  {
    yp_unbind( this->domain );
    free(this->domain);
  }
  this->domain = strdup( sp[-args].u.string->str );
  err = yp_bind( this->domain );

  YPERROR( err );

  pop_n_elems(args);
}

/*! @decl mapping(string:string) all(string map)
 *!
 *! Returns the whole map as a mapping.
 *!
 *! @[map] is the YP-map to search in. This must be the full map name,
 *! you have to use @tt{passwd.byname@} instead of just @tt{passwd@}.
 */
static void f_all(INT32 args)
{
  int err, num=0;
  char *retval, *retkey;
  int retlen, retkeylen;
  char *map;
  struct mapping *res_map;
  check_all_args("all", args, BIT_STRING, 0);

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
    YPERROR( err );
  }

  this->last_size = num;
  pop_n_elems(args);
  push_mapping( res_map );
}

/*! @decl void map(string map, function(string, string:void) fun)
 *!
 *! For each entry in @[map], call the function specified by @[fun].
 *!
 *! @[fun] will get two arguments, the first being the key, and the
 *! second the value.
 *!
 *! @[map] is the YP-map to search in. This must be the full map name.
 *! eg @tt{passwd.byname@} instead of just @tt{passwd@}.
 */
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
    YPERROR( err );

  pop_n_elems(args);
}

/*! @decl int order(string map)
 *!
 *! Returns the 'order' number for the map @[map].
 *!
 *! This is usually the number of seconds since Jan 1 1970 (see @[time()]).
 *! When the map is changed, this number will change as well.
 *!
 *! @[map] is the YP-map to search in. This must be the full map name.
 *! eg @tt{passwd.byname@} instead of just @tt{passwd@}.
 */
static void f_order(INT32 args)
{
  int err;
  YP_ORDER_TYPE ret;

  check_all_args("order", args, BIT_STRING, 0);
  
  err = yp_order( this->domain, sp[-args].u.string->str, &ret);

  YPERROR( err );

  pop_n_elems( args );
  push_int( (INT32) ret );
}

/*! @decl string match(string map, string key)
 *!
 *! Search for the key @[key] in the Yp-map @[map].
 *!
 *! @returns
 *! If there is no @[key] in the map, 0 (zero) will be returned,
 *! otherwise the string matching the key will be returned.
 *!
 *! @note
 *! @[key] must match exactly, no pattern matching of any kind is done.
 */
static void f_match(INT32 args)
{
  int err;
  char *retval;
  int retlen;
  
  check_all_args("match", args, BIT_STRING, BIT_STRING, 0);

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

  YPERROR( err );

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

/*! @endclass
 */

/*! @endmodule
 */

/******************** PUBLIC FUNCTIONS BELOW THIS LINE */


PIKE_MODULE_INIT
{
  /* function(void:string) */
  ADD_FUNCTION("default_domain", f_default_domain,tFunc(tVoid,tStr),
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

PIKE_MODULE_EXIT
{
  
}
#else

#include "module.h"
PIKE_MODULE_INIT {}
PIKE_MODULE_EXIT {}

#endif
