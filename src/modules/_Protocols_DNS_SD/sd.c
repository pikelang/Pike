/*
|| This file is part of Pike. For copyright information see COPYRIGHT.
|| Pike is distributed under GPL, LGPL and MPL. See the file COPYING
|| for more information.
|| $Id: sd.c,v 1.1 2005/04/09 21:07:21 jonasw Exp $
*/


/* Glue for DNS Service Discovery, which is built on top of e.g. Multicast
   DNS (ZeroConf/Rendezvous). Using this API a Pike program can register a
   service (e.g. a web server) and have other applications on the local
   network detect it without additional configuration.

   The specification can be found at <http://www.dns-sd.org/>.
   
   The implementation requires either the dns_ds.h (found in Mac OS X)
   or howl.h (part of libhowl on Linux).
*/


#include "global.h"
#include "config.h"
#include "pike_macros.h"
#include "stralloc.h"
#include "pike_error.h"
#include "object.h"
#include "constants.h"
#include "interpret.h"
#include "svalue.h"
#include "mapping.h"
#include "builtin_functions.h"
#include "module_support.h"
#include "threads.h"

#include <signal.h>

RCSID("$Id: sd.c,v 1.1 2005/04/09 21:07:21 jonasw Exp $");


#ifdef THIS
#undef THIS
#endif
#define THIS ((struct service *)(Pike_fp->current_storage))
#define sp Pike_sp


#if defined(HAVE_DNS_SD) || defined(HAVE_HOWL)





#ifdef HAVE_DNS_SD

/* Mac OS X interface is defined in <dns_sd.h> */
#ifdef HAVE_DNS_SD_H
#include <dns_sd.h>
#endif

#define IS_ERR(x) ((x) != kDNSServiceErr_NoError)

/* Instance variables for each service registration */
struct service {
  DNSServiceRef  service_ref;
};


/* Workaround for 10.4 header changes */
#if !defined(kDNSServiceErr_BadinterfaceIndex)
#  define kDNSServiceErr_BadinterfaceIndex kDNSServiceErr_BadInterfaceIndex
#endif

static void raise_error(char *msg, DNSServiceErrorType err)
{
  char *reason;

  if (err == kDNSServiceErr_NoError)
    return;
  
  switch (err) {
  case kDNSServiceErr_NoSuchName:
    reason = "No such name";
    break;
  case kDNSServiceErr_NoMemory:
    reason = "No memory";
    break;
  case kDNSServiceErr_BadParam:
    reason = "Bad parameter";
    break;
  case kDNSServiceErr_BadReference:
    reason = "Bad reference";
    break;
  case kDNSServiceErr_BadState:
    reason = "Bad state";
    break;
  case kDNSServiceErr_BadFlags:
    reason = "Bad flags";
    break;
  case kDNSServiceErr_Unsupported:
    reason = "Unsupported";
    break;
  case kDNSServiceErr_AlreadyRegistered:
    reason = "Already registered";
    break;
  case kDNSServiceErr_NameConflict:
    reason = "Name conflict";
    break;
  case kDNSServiceErr_Invalid:
    reason = "Invalid";
    break;
  case kDNSServiceErr_Incompatible:
    reason = "Incompatible";
    break;
  case kDNSServiceErr_BadinterfaceIndex:
    reason = "Bad interface index";
    break;
  default:
    reason = "Unknown error";
    break;
  }
  Pike_error("DNS_SD: %s Reason: %s (%d)\n", msg, reason, err);
}


static void start_service_callback(DNSServiceRef ref,
				   DNSServiceFlags flags,
				   DNSServiceErrorType error,
				   const char *name,
				   const char *regtype,
				   const char *domain,
				   void *context)
{
}


static DNSServiceErrorType start_service(struct service *svc,
					 char *name,
					 char *service,
					 char *domain,
					 int port,
					 char *txt,
					 int txtlen)
{
  DNSServiceErrorType err;
  DNSServiceRef       ref;
  
  /* Empty strings should be passed as NULL in order to get default values */
  if (name && !strlen(name))
    name = NULL;
  if (domain && !strlen(domain))
    domain = NULL;
  if (txt && !txtlen)
    txt = NULL;
  
  svc->service_ref = NULL;
  err = DNSServiceRegister(&ref, 0, 0, name, service, domain, NULL, port,
                           txtlen, txt, start_service_callback, NULL);
  if (err == kDNSServiceErr_NoError)
    svc->service_ref = ref;
  return err;
}


static void stop_service(struct service *svc)
{
  if (svc->service_ref) {
    DNSServiceRefDeallocate(svc->service_ref);
    svc->service_ref = NULL;
  }
}


static DNSServiceErrorType update_txt_record(struct service *svc,
					     char *txt, int txtlen)
{
  if (svc->service_ref) {
    int ttl = 0;
    return DNSServiceUpdateRecord(svc->service_ref, NULL, 0,
				  txtlen, txt, ttl);
  }
  return kDNSServiceErr_Invalid;
}

#endif /* HAVE_DNS_SD */




#if defined(HAVE_HOWL) && !defined(HAVE_DNS_SD)

/* Load <howl.h> to indirectly get <discovery/discovery.h> */
#ifdef HAVE_HOWL_H
#include <howl.h>
#endif

#define IS_ERR(x) ((x) != SW_OKAY)

/* Global session shared for all objects */
static sw_discovery service_session = NULL;
static THREAD_T service_thread;

struct service {
  sw_discovery_oid  service_ref;
};


static void raise_error(char *msg, sw_result err)
{
  char *reason;

  if (err == SW_OKAY)
    return;
  
  switch (err) {
  case SW_DISCOVERY_E_NO_MEM:
    reason = "No memory";
    break;
  case SW_DISCOVERY_E_BAD_PARAM:
    reason = "Bad parameter";
    break;
  case SW_DISCOVERY_E_UNKNOWN:
  default:
    reason = "Unknown error";
    break;
  }
  Pike_error("DNS_SD: %s Reason: %s (%d)\n", msg, reason, err);
}



static sw_result start_reply_fn(sw_discovery                  discovery,
				sw_discovery_publish_status   status,
				sw_discovery_oid              oid,
				sw_opaque                     extra)
{
  //  Do nothing
  return SW_OKAY;
}


static sw_result start_service(struct service *svc,
			       char *name,
			       char *service,
			       char *domain,
			       int port,
			       char *txt,
			       int txtlen)
{
  sw_result err = SW_DISCOVERY_E_UNKNOWN;
  
  /* Empty strings should be passed as NULL in order to get default values */
  if (domain && !strlen(domain))
    domain = NULL;
  if (txt && !txtlen)
    txt = NULL;
  
  err = sw_discovery_publish(service_session,
			     0, name, service, domain, NULL, port,
			     txt, txtlen, start_reply_fn, NULL,
			     &svc->service_ref);
  return err;
}


static void stop_service(struct service *svc)
{
  if (svc->service_ref) {
    /* Abort registration */
    sw_discovery_cancel(service_session, svc->service_ref);
  }
}


static sw_result update_txt_record(struct service *svc, char *txt, int txtlen)
{
  if (svc->service_ref) {
    return sw_discovery_publish_update(service_session,
				       svc->service_ref,
				       txt, txtlen);
  }
  return SW_DISCOVERY_E_UNKNOWN;
}


static void * howl_thread(void *arg)
{
  sw_discovery_run(service_session);
  return NULL;
}


static void init_howl_module()
{
  if (sw_discovery_init(&service_session) == SW_OKAY) {
    th_create_small(&service_thread, howl_thread, NULL);
  }
}


static void exit_howl_module()
{
  /* Close active session */
  if (service_session)
    sw_discovery_fina(service_session);
  
  /* Kill Howl thread if running */
  if (service_thread)
    th_kill(service_thread, SIGCHLD);
}


#endif /* defined(HAVE_HOWL) && !defined(HAVE_DNS_SD) */




static void f_update_txt(INT32 args)
{
  check_all_args("Service->update_txt", args,
		 BIT_STRING,	/* txt */
		 0);
  
  /* Can only be called if we have valid service */
  if (THIS->service_ref) { 
    char *txt = sp[0 - args].u.string->str;
    int  txtlen = sp[0 - args].u.string->len;
    
    int err = update_txt_record(THIS, txt, txtlen);
    if (IS_ERR(err))
      raise_error("Could not update TXT record.", err);
  }
  pop_n_elems(args);
}


static void f_create(INT32 args)
{
  char                *name, *service, *domain, *txt;
  int                 port, txtlen, err;
  
  check_all_args("Service->create", args,
		 BIT_STRING,		/* name */
		 BIT_STRING,		/* service */
		 BIT_STRING,		/* domain */
		 BIT_INT,		/* port */
		 BIT_STRING | BIT_VOID,	/* txt */
		 0);
  
  /* Stop existing service if one is running */
  stop_service(THIS);
  
  /* Get string arguments which should already be UTF-8 and clipped to
     appropriate lengths. */
  name    = sp[0 - args].u.string->str;
  service = sp[1 - args].u.string->str;
  domain  = sp[2 - args].u.string->str;
  port    = sp[3 - args].u.integer;

  /* Optional TXT record may theoretically contain NUL chars so we can't
     trust strlen. */
  txt     = (args == 5) ? sp[4 - args].u.string->str : NULL;
  txtlen  = txt ? sp[4 - args].u.string->len : 0;
  
  /* Register new service */
  err = start_service(THIS, name, service, domain, port, txt, txtlen);
  if (IS_ERR(err))
    raise_error("Could not register service.", err);
  pop_n_elems(args);
}


static void init_service_struct(struct object *o)
{
  THIS->service_ref = 0;
}


static void exit_service_struct(struct object *o)
{
  /* Stop an existing service */
  stop_service(THIS);
}


PIKE_MODULE_INIT
{
  start_new_program();
  
  ADD_STORAGE(struct service);
  
  set_init_callback(init_service_struct);
  set_exit_callback(exit_service_struct);
  
  /* function(string, string, string, int, string|void:void) */
  ADD_FUNCTION("create", f_create,
	       tFunc(tStr tStr tStr tInt tOr(tStr, tVoid), tVoid), 0);
  
  /* function(string:void) */
  ADD_FUNCTION("update_txt", f_update_txt, tFunc(tStr, tVoid), 0);
  
  end_class("Service", 0);

#ifdef HAVE_HOWL
  init_howl_module();
#endif
}


PIKE_MODULE_EXIT
{
#ifdef HAVE_HOWL
  exit_howl_module();
#endif
}


#else /* defined(HAVE_DNS_SD) || defined(HAVE_HOWL) */

PIKE_MODULE_INIT {}
PIKE_MODULE_EXIT {}

#endif /* defined(HAVE_DNS_SD) || defined(HAVE_HOWL) */
